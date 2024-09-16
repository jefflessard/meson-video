#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/mfd/syscon.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/soc/amlogic/meson-canvas.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"
#include "meson-vcodec.h"
#include "amlogic.h"

#define DRIVER_NAME "meson-vcodec"

static const char* reg_names[MAX_BUS] = {
	[BUS_DOS] = "dos",
	[BUS_PARSER] = "esparser",
	[BUS_HEVCENC] = "hevcenc",
};

static const char* regmap_names[MAX_BUS] = {
	[BUS_AO] = "amlogic,ao-sysctrl",
	[BUS_HHI] = "amlogic,hhi-sysctrl",
};

static const char* clk_names[MAX_CLKS] = {
	[CLK_DOS] = "dos",
	[CLK_PARSER] = "dos_parser",
	[CLK_VDEC1] = "vdec_1",
	[CLK_HEVC] = "vdec_hevc",
	[CLK_HEVCF] = "vdec_hevcf",
	[CLK_HCODEC] = "hcodec",
};

static const char* reset_names[MAX_RESETS] = {
	[RESET_PARSER] = "esparser",
	[RESET_HCODEC] = "hcodec",
};

static const char* irq_names[MAX_IRQS] = {
	[IRQ_VDEC] = "vdec",
	[IRQ_PARSER] = "esparser",
	[IRQ_HCODEC] = "hcodec",
	[IRQ_HEVCENC] = "hevcenc",
};

static void meson_vcodec_configure_ctrls(struct meson_codec_job *job) {
	struct v4l2_ctrl_handler *handler = &job->session->ctrl_handler;
	const struct v4l2_std_ctrl *controls = job->codec->ctrls;
	size_t num_ctrls = job->codec->num_ctrls;
	struct v4l2_ctrl *ctrl;
	size_t i;

	for (i = 0; i < num_ctrls; i++) {
		ctrl = v4l2_ctrl_new_std(handler, NULL, controls[i].id, controls[i].min, controls[i].max, controls[i].step, controls[i].def);

		if (ctrl == NULL) {
			session_warn(job->session, "Failed to create control for CID %u\n", controls[i].id);
			continue;
		}
	}
}

/* vb2_ops */

static int meson_vcodec_queue_setup(
		struct vb2_queue *vq,
		unsigned int *nbuffers,
		unsigned int *nplanes,
		unsigned int sizes[],
		struct device *alloc_devs[]
){
	struct meson_vcodec_session *session = vb2_get_drv_priv(vq);
	struct v4l2_format *fmt;

	stream_trace(session, vq->type, "n_buffers=%d", *nbuffers);

	if(IS_SRC_STREAM(vq->type)) {
		fmt = &session->src_fmt;
	} else {
		fmt = &session->dst_fmt;
	}

	if (*nplanes) {
		if (*nplanes != fmt->fmt.pix_mp.num_planes) {
			stream_err(session, vq->type, "Invalid num_planes");
			return -EINVAL;
		}

		for (int i = 0; i < *nplanes; i++) {
			if (sizes[i] < fmt->fmt.pix_mp.plane_fmt[i].sizeimage) {
				stream_err(session, vq->type, "Invalid plane size");
				return -EINVAL;
			}
		}

		// TODO set number of buffers depending on the codec

		return 0;
	}

	if (STREAM_STATUS(session, vq->type) != STREAM_STATUS_FMTSET) {
		stream_err(session, vq->type, "invalid status to init");
		return -EINVAL;
	}
		
	*nplanes = fmt->fmt.pix_mp.num_planes;
	stream_trace(session, vq->type, "nplanes=%d", *nplanes);
	for (int i = 0; i < *nplanes; i++) {
		sizes[i] = fmt->fmt.pix_mp.plane_fmt[i].sizeimage;
		stream_trace(session, vq->type, "planes=%d, size=%d", i, sizes[i]);
	}

	// TODO set number of buffers depending on the codec

	SET_STREAM_STATUS(session, vq->type, STREAM_STATUS_QSETUP);
	return 0;
}

static int meson_vcodec_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *v4l2buf = to_vb2_v4l2_buffer(vb);
	struct meson_vcodec_session *session = vb2_get_drv_priv(vb->vb2_queue);
	struct v4l2_format *fmt;
	
//	stream_trace(session, vq->type);

	if(IS_SRC_STREAM(vb->type)) {
		fmt = &session->src_fmt;
	} else {
		fmt = &session->dst_fmt;
	}

	if (vb->num_planes != fmt->fmt.pix_mp.num_planes) {
		stream_err(session, vb->type, "Invalid num_planes");
		return -EINVAL;
	}

	for (int i = 0; i < fmt->fmt.pix_mp.num_planes; i++) {
		if (vb2_plane_size(vb, i) < fmt->fmt.pix_mp.plane_fmt[i].sizeimage) {
			stream_err(session, vb->type, "Invalid plane size");
			return -EINVAL;
		}
	}

	v4l2buf->field = V4L2_FIELD_NONE;

	return 0;
}

static void meson_vcodec_buf_queue(struct vb2_buffer *vb)
{
	struct meson_vcodec_session *session = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

//	stream_trace(session, vq->type);

	if (!(
		STREAM_STATUS(session, vb->type) >= STREAM_STATUS_INIT &&
		STREAM_STATUS(session, vb->type) <= STREAM_STATUS_RUN
	)) {
		stream_err(session, vb->type, "invalid status to buffer queue");
		return;
	}

	v4l2_m2m_buf_queue(session->m2m_ctx, vbuf);

	switch (session->type) {
		case SESSION_TYPE_ENCODE:
			ENCODER_OPS(session, queue, vbuf);
			break;
		case SESSION_TYPE_DECODE:
			DECODER_OPS(session, queue, vbuf);
			break;
		case SESSION_TYPE_TRANSCODE:
			// TODO how to handle intermediate format buffers?
			if(IS_SRC_STREAM(vb->type)) {
				DECODER_OPS(session, queue, vbuf);
			} else {
				ENCODER_OPS(session, queue, vbuf);
			}
			break;
		case SESSION_TYPE_NONE:
		default:
			stream_err(session, vb->type, "Failed to queue buffer: invalid session type");
			v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
	}
}

static int meson_vcodec_prepare_streaming(struct vb2_queue *vq) {
	struct meson_vcodec_session *session = vb2_get_drv_priv(vq);
	int ret = 0;

	stream_trace(session, vq->type);

	if (session != session->core->active_session) {
		stream_err(session, vq->type, "Can't init streaming due to active session %d", session->core->active_session ? session->core->active_session->session_id : -1);
		return -EBUSY;
	}

	if (STREAM_STATUS(session, vq->type) != STREAM_STATUS_QSETUP) {
		stream_err(session, vq->type, "invalid status to init");
		return -EINVAL;
	}

	// Init codec as soon as src or dst queue gets prepared
	if (
		session->src_status < STREAM_STATUS_INIT &&
		session->dst_status < STREAM_STATUS_INIT
	) {
		session_dbg(session, "Initializing codec");
		switch (session->type) {
			case SESSION_TYPE_ENCODE:
				meson_vcodec_configure_ctrls(&session->enc_job);
				ret = ENCODER_OPS(session, init);
				if (ret) {
					session_err(session, "Failed to init encoder: %d", ret);
					return ret;
				}
				break;

			case SESSION_TYPE_DECODE:
				meson_vcodec_configure_ctrls(&session->dec_job);
				ret = DECODER_OPS(session, init);
				if (ret) {
					session_err(session, "Failed to init decoder: %d", ret);
					return ret;
				}
				break;

			case SESSION_TYPE_TRANSCODE:
				meson_vcodec_configure_ctrls(&session->dec_job);
				ret = DECODER_OPS(session, init);
				if (ret) {
					session_err(session, "Failed to init decoder: %d", ret);
					return ret;
				}
				meson_vcodec_configure_ctrls(&session->enc_job);
				ret = ENCODER_OPS(session, init);
				if (ret) {
					session_err(session, "Failed to init encoder: %d", ret);
					DECODER_OPS(session, release);
					return ret;
				}
				break;

			case SESSION_TYPE_NONE:
			default:
				session_err(session, "Failed to init: invalid session type");
				return -EINVAL;
		}
	}

	SET_STREAM_STATUS(session, vq->type, STREAM_STATUS_INIT);

	return 0;
}

static int meson_vcodec_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct meson_vcodec_session *session = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer *vbuf;
	int ret = 0;

	stream_trace(session, vq->type);

	if (STREAM_STATUS(session, vq->type) != STREAM_STATUS_INIT) {
		stream_err(session, vq->type, "invalid status to start streaming");
		ret = -EINVAL;
		goto release_buffers;
	}
	
	switch (session->type) {
		case SESSION_TYPE_ENCODE:
			ret = ENCODER_OPS(session, start, vq, count);
			if (ret) {
				stream_err(session, vq->type, "Failed to start encoder: %d", ret);
				goto release_buffers;
			}
			break;

		case SESSION_TYPE_DECODE:
			ret = DECODER_OPS(session, start, vq, count);
			if (ret) {
				stream_err(session, vq->type, "Failed to start decoder: %d", ret);
				goto release_buffers;
			}
			break;

		case SESSION_TYPE_TRANSCODE:
			// TODO how to handle intermediate format streaming ?
			if(IS_SRC_STREAM(vq->type)) {
				ret = DECODER_OPS(session, start, vq, count);
				if (ret) {
					stream_err(session, vq->type, "Failed to start decoder: %d", ret);
					goto release_buffers;
				}
			} else {
				ret = ENCODER_OPS(session, start, vq, count);
				if (ret) {
					stream_err(session, vq->type, "Failed to start encoder: %d", ret);
					goto release_buffers;
				}
			}
			break;

		case SESSION_TYPE_NONE:
		default:
			stream_err(session, vq->type, "Failed to start streaming: invalid session type");
			ret = -EINVAL;
			goto release_buffers;
	}

	SET_STREAM_STATUS(session, vq->type, STREAM_STATUS_START);

	return 0;

release_buffers:
	if (IS_SRC_STREAM(vq->type)) {
		while ((vbuf = v4l2_m2m_src_buf_remove(session->m2m_ctx)))
			v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_QUEUED);
	} else {
		while ((vbuf = v4l2_m2m_dst_buf_remove(session->m2m_ctx)))
			v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_QUEUED);
	}

	return ret;
}

static void meson_vcodec_stop_streaming(struct vb2_queue *vq)
{
	struct meson_vcodec_session *session = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer *vbuf;

	stream_trace(session, vq->type);

	if (!(STREAM_STATUS(session, vq->type) >= STREAM_STATUS_START &&
		STREAM_STATUS(session, vq->type) <= STREAM_STATUS_ABORT)) {
		stream_warn(session, vq->type, "invalid status to stop streaming");
	}

	switch (session->type) {
		case SESSION_TYPE_ENCODE:
			ENCODER_OPS(session, stop, vq);
			break;

		case SESSION_TYPE_DECODE:
			DECODER_OPS(session, stop, vq);
			break;

		case SESSION_TYPE_TRANSCODE:
			// TODO how to handle intermediate format streaming ?
			if(IS_SRC_STREAM(vq->type)) {
				DECODER_OPS(session, stop, vq);
			} else {
				ENCODER_OPS(session, stop, vq);
			}
			break;

		case SESSION_TYPE_NONE:
		default:
			break;
	}

	// Return all buffers to vb2 in QUEUED state
	if (IS_SRC_STREAM(vq->type)) {
		while ((vbuf = v4l2_m2m_src_buf_remove(session->m2m_ctx)))
			v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
	} else {
		while ((vbuf = v4l2_m2m_dst_buf_remove(session->m2m_ctx)))
			v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
	}

	SET_STREAM_STATUS(session, vq->type, STREAM_STATUS_STOP);
}

static void meson_vcodec_unprepare_streaming(struct vb2_queue *vq)
{
	struct meson_vcodec_session *session = vb2_get_drv_priv(vq);

	stream_trace(session, vq->type);

	if (STREAM_STATUS(session, vq->type) < STREAM_STATUS_INIT ||
		STREAM_STATUS(session, vq->type) > STREAM_STATUS_STOP
	) {
		stream_warn(session, vq->type, "invalid status to release");
	}
	
	SET_STREAM_STATUS(session, vq->type, STREAM_STATUS_RELEASE);

	// relase codec after both src and dst queues have been unprepared
	if (
		session->src_status == STREAM_STATUS_RELEASE &&
		session->dst_status == STREAM_STATUS_RELEASE
	) {
		session_dbg(session, "Releasing codec");
		switch (session->type) {
			case SESSION_TYPE_ENCODE:
				ENCODER_OPS(session, release);
				break;

			case SESSION_TYPE_DECODE:
				DECODER_OPS(session, release);
				break;

			case SESSION_TYPE_TRANSCODE:
				DECODER_OPS(session, release);
				ENCODER_OPS(session, release);
				break;

			case SESSION_TYPE_NONE:
			default:
				break;
		}
	}
}

static const struct vb2_ops meson_vcodec_vb2_ops = {
	.queue_setup       = meson_vcodec_queue_setup,
	.buf_prepare       = meson_vcodec_buf_prepare,
	.buf_queue         = meson_vcodec_buf_queue,
	.prepare_streaming = meson_vcodec_prepare_streaming,
	.start_streaming   = meson_vcodec_start_streaming,
	.stop_streaming    = meson_vcodec_stop_streaming,
	.unprepare_streaming = meson_vcodec_unprepare_streaming,
	.wait_prepare      = vb2_ops_wait_prepare,
	.wait_finish       = vb2_ops_wait_finish,
};

/* v4l2_m2m_ops */

static void meson_vcodec_m2m_device_run(void *priv)
{
	struct meson_vcodec_session *session = priv;

	session_trace(session);

	SET_STREAM_STATUS(session, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, STREAM_STATUS_RUN);
	SET_STREAM_STATUS(session, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, STREAM_STATUS_RUN);

	switch (session->type) {
		case SESSION_TYPE_ENCODE:
			ENCODER_OPS(session, resume);
			break;

		case SESSION_TYPE_DECODE:
			DECODER_OPS(session, resume);
			break;

		case SESSION_TYPE_TRANSCODE:
			DECODER_OPS(session, resume);
			ENCODER_OPS(session, resume);
			break;

		case SESSION_TYPE_NONE:
		default:
			break;
	}
}

static void meson_vcodec_m2m_job_abort(void *priv)
{
	struct meson_vcodec_session *session = priv;
	
	session_trace(session);

	SET_STREAM_STATUS(session, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, STREAM_STATUS_ABORT);
	SET_STREAM_STATUS(session, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, STREAM_STATUS_ABORT);

	switch (session->type) {
		case SESSION_TYPE_ENCODE:
			ENCODER_OPS(session, abort);
			break;

		case SESSION_TYPE_DECODE:
			DECODER_OPS(session, abort);
			break;

		case SESSION_TYPE_TRANSCODE:
			DECODER_OPS(session, abort);
			ENCODER_OPS(session, abort);
			break;

		case SESSION_TYPE_NONE:
		default:
			break;
	}
}

static const struct v4l2_m2m_ops meson_vcodec_m2m_ops = {
	.device_run = meson_vcodec_m2m_device_run,
	.job_abort = meson_vcodec_m2m_job_abort,
};

/* v4l2_file_operations */

static int meson_vcodec_m2m_queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
	struct meson_vcodec_session *session = priv;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->ops = &meson_vcodec_vb2_ops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->drv_priv = session;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->min_buffers_needed = 1;
	src_vq->dev = session->core->dev;
	src_vq->lock = &session->lock;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->ops = &meson_vcodec_vb2_ops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->drv_priv = session;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->min_buffers_needed = 1;
	dst_vq->dev = session->core->dev;
	dst_vq->lock = &session->lock;

	return vb2_queue_init(src_vq) || vb2_queue_init(dst_vq);
}

static int meson_vcodec_session_open(struct file *file)
{
	struct meson_vcodec_core *core = video_drvdata(file);
	struct meson_vcodec_session *session;
	int ret = 0;

	session = kzalloc(sizeof(*session), GFP_KERNEL);
	if (!session)
		return -ENOMEM;

	session->core = core;
	core->active_session = session;
	session->session_id = atomic_inc_return(&core->session_counter);

	session_trace(session);

	v4l2_fh_init(&session->fh, &core->vfd);
	file->private_data = &session->fh;
	v4l2_fh_add(&session->fh);

	ret = v4l2_ctrl_handler_init(&session->ctrl_handler, 0);
	if (ret) {
		session_err(session, "Failed to initialize control handler");
		goto err_fh_exit;
	}
	session->fh.ctrl_handler = &session->ctrl_handler;

	session->m2m_ctx = v4l2_m2m_ctx_init(core->m2m_dev, session, meson_vcodec_m2m_queue_init);
	if (IS_ERR(session->fh.m2m_ctx)) {
		session_err(session, "Failed to initialize v4l2-m2m context");
		ret = PTR_ERR(session->fh.m2m_ctx);
		goto err_ctrl_handler_free;
	}
	session->fh.m2m_ctx = session->m2m_ctx;

	mutex_init(&session->lock);

	return 0;

err_ctrl_handler_free:
	v4l2_ctrl_handler_free(&session->ctrl_handler);
err_fh_exit:
	v4l2_fh_exit(&session->fh);
	kfree(session);
	return ret;
}

static int meson_vcodec_session_release(struct file *file)
{
	struct meson_vcodec_session *session = container_of(file->private_data, struct meson_vcodec_session, fh);
	struct meson_vcodec_core *core = session->core;

	session_trace(session);

	v4l2_m2m_ctx_release(session->m2m_ctx);
	v4l2_ctrl_handler_free(&session->ctrl_handler);
	v4l2_fh_del(&session->fh);
	v4l2_fh_exit(&session->fh);
	kfree(session);

	core->active_session = NULL;

	return 0;
}

static const struct v4l2_file_operations meson_vcodec_fops = {
	.owner = THIS_MODULE,
	.open = meson_vcodec_session_open,
	.release = meson_vcodec_session_release,
	.unlocked_ioctl = video_ioctl2,
	.poll = v4l2_m2m_fop_poll,
	.mmap = v4l2_m2m_fop_mmap,
};

/* v4l2_ioctl_ops */

static int meson_vcodec_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	struct meson_vcodec_core *core = video_drvdata(file);

	strscpy(cap->driver, DRIVER_NAME, sizeof(cap->driver));
	strscpy(cap->card, "Meson Video Codec", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s", dev_name(core->dev));

	return 0;
}

static int filter_formats(struct meson_vcodec_session *session, u32 fmt_type, const struct meson_format **array)
{
	struct meson_vcodec_core *core = session->core;
	const struct meson_codec_formats *codecs = core->platform_specs->codecs;
	int num_codecs = core->platform_specs->num_codecs;
	const struct meson_format *fmt_list, *fmt_filter;
	u32 pixfmt_filter;
	bool found, list_src;
	int i, j, count = 0;

	list_src = IS_SRC_STREAM(fmt_type);
	if (list_src) {
		pixfmt_filter = session->dst_fmt.fmt.pix_mp.pixelformat;
	} else {
		pixfmt_filter = session->src_fmt.fmt.pix_mp.pixelformat;
	}

	for (i = 0; i < num_codecs; i++) {
		found = false;

		if (list_src) {
			fmt_list = codecs[i].src_fmt;
			fmt_filter = codecs[i].dst_fmt;
		} else {
			fmt_list = codecs[i].dst_fmt;
			fmt_filter = codecs[i].src_fmt;
		}

		if (!pixfmt_filter || pixfmt_filter == fmt_filter->pixelformat) {
			for (j = 0; j < count; j++) {
				if (array[j] == fmt_list) {
					found = true;
					break;
				}
			}

			if (!found) {
				array[count++] = fmt_list;
			}
		}
	}

	return count;
}

static int meson_vcodec_enum_fmt_vid(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	struct meson_vcodec_session *session = container_of(file->private_data, struct meson_vcodec_session, fh);
	const struct meson_format *formats[MAX_FORMATS];
	const struct meson_format *fmt;
	int count;

	stream_trace(session, f->type, "index=%d", f->index);

	count = filter_formats(session, f->type, formats);

	if (f->index < count) {
		fmt = formats[f->index];
		f->pixelformat = fmt->pixelformat;
		strncpy(f->description, fmt->description, sizeof(f->description) - 1);
		f->flags = fmt->flags;
		return 0;
	}

	return -EINVAL;
}

static int meson_vcodec_enum_framesizes(struct file *file, void *priv, struct v4l2_frmsizeenum *fsize)
{
	struct meson_vcodec_core *core = video_drvdata(file);
	struct meson_vcodec_session *session = container_of(file->private_data, struct meson_vcodec_session, fh);
	const struct meson_codec_formats *codecs = core->platform_specs->codecs;
	int num_codecs = core->platform_specs->num_codecs;
	const struct meson_codec_formats *codec;
	u32 pxlfmt_src = session->src_fmt.fmt.pix_mp.pixelformat;
	u32 pxlfmt_dst = session->dst_fmt.fmt.pix_mp.pixelformat;
	u16 max_width = 0, max_height = 0;
	int i;

	if (fsize->index) {
		return -EINVAL;
	}

	session_trace(session, "index=%d, fmt=%.4s", fsize->index, (char*)&fsize->pixel_format);

	for (i = 0; i < num_codecs; i++) {
		codec = &codecs[i];

		if ((	codec->dst_fmt->pixelformat == fsize->pixel_format || codec->src_fmt->pixelformat == fsize->pixel_format
		) && (
				!pxlfmt_src || pxlfmt_src == codec->src_fmt->pixelformat
		) && (
				!pxlfmt_dst || pxlfmt_dst == codec->dst_fmt->pixelformat
		)) {
			session_trace(session, "src_fmt=%.4s, dst_fmt=%.4s", (char*)&codec->src_fmt->pixelformat, (char*)&codec->dst_fmt->pixelformat);

			if (pxlfmt_src && pxlfmt_dst) {
				max_width = codec->max_width;
				max_height = codec->max_height;
				break;
			} else {
				if (codec->max_width > max_width)
					max_width = codec->max_width;
				if (codec->max_height > max_height)
					max_height = codec->max_height;
			}
		}
	}
	
	if (max_width != 0 && max_height != 0) {
		fsize->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
		fsize->stepwise.min_width = MIN_RESOLUTION_WIDTH;
		fsize->stepwise.min_height = MIN_RESOLUTION_HEIGHT;
		fsize->stepwise.max_width = max_width;
		fsize->stepwise.max_height = max_height;

		return 0;
	}

	return -EINVAL;
}

static const struct meson_codec_formats* find_codec(struct meson_vcodec_core *core, u32 pxfmt_src, u32 pxfmt_dst) {
	const struct meson_codec_formats *codecs = core->platform_specs->codecs;
	int num_codecs = core->platform_specs->num_codecs;
	int i;

	for (i = 0; i < num_codecs; i++) {
		if (	codecs[i].src_fmt->pixelformat == pxfmt_src &&
			codecs[i].dst_fmt->pixelformat == pxfmt_dst
		) {
			return &codecs[i];
		}
	}
	return NULL;
}

static void set_buffer_sizes(struct v4l2_format *f, const struct meson_format *fmt) {
	u32 sizeimage, aligned_width;
	int j;

	sizeimage = ALIGN(f->fmt.pix_mp.width * f->fmt.pix_mp.height, SZ_64K);
	aligned_width = ALIGN(f->fmt.pix_mp.width, 32);

#if 0
	if (fmt == &nv12) {
		f->fmt.pix_mp.num_planes = 1;
		f->fmt.pix_mp.plane_fmt[0].sizeimage = sizeimage;
		f->fmt.pix_mp.plane_fmt[0].bytesperline = 0;
		return;
	}
#endif

	f->fmt.pix_mp.num_planes = fmt->num_planes;
	for (j = 0; j < fmt->num_planes; j++) {
		memset(
			f->fmt.pix_mp.plane_fmt[j].reserved,
			0,
			sizeof(f->fmt.pix_mp.plane_fmt[j].reserved)
		);

		if (fmt->plane_size_denums[j]) {
			f->fmt.pix_mp.plane_fmt[j].sizeimage = sizeimage / fmt->plane_size_denums[j];
		} else {
			f->fmt.pix_mp.plane_fmt[j].sizeimage = sizeimage;
		}

		if (fmt->plane_line_denums[j]) {
			f->fmt.pix_mp.plane_fmt[j].bytesperline = aligned_width / fmt->plane_line_denums[j];
		} else {
			f->fmt.pix_mp.plane_fmt[j].bytesperline = 0;
		}
	}
}

static int common_try_fmt_vid(struct meson_vcodec_core *core, u32 fmt_type, struct v4l2_format *fmt_src, struct v4l2_format *fmt_dst)
{
	const struct meson_codec_formats *codecs = core->platform_specs->codecs;
	int num_codecs = core->platform_specs->num_codecs;
	const struct meson_codec_formats *codec = NULL;
	struct v4l2_format *fmt_set;
	u32 max_width, max_height, pxfmt_match;
	bool is_set_src_fmt;
	int i;

	switch (fmt_type) {
		case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
			fmt_set = fmt_src;
			is_set_src_fmt = true;
			break;
		case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
			fmt_set = fmt_dst;
			is_set_src_fmt = false;
			break;
		default:
			dev_err(core->dev, "Format type not supported: %d", fmt_type);
			return -EINVAL;
	}

	if (num_codecs == 0) {
		dev_err(core->dev, "No codec formats supported");
		return -EINVAL;
	}

	switch(fmt_set->fmt.pix_mp.pixelformat) {
		case V4L2_PIX_FMT_YUV420:
			fmt_set->fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
			break;
		case V4L2_PIX_FMT_NV12:
			fmt_set->fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
			break;
	}

	// Both formats set
	if (fmt_src->fmt.pix_mp.pixelformat &&
		fmt_dst->fmt.pix_mp.pixelformat
	) {
		// Exact match
		codec = find_codec(
			core,
			fmt_src->fmt.pix_mp.pixelformat,
			fmt_dst->fmt.pix_mp.pixelformat
		);
		if (codec) {
			max_width = codec->max_width;
			max_height = codec->max_height;

		// Filter on the other set format to find a compatible format
		} else {
			dev_warn(core->dev, "Codec formats not found");

			for(i = 0; i < num_codecs; i++) {
				if ((is_set_src_fmt &&
					fmt_dst->fmt.pix_mp.pixelformat == codecs[i].dst_fmt->pixelformat ) ||
					fmt_src->fmt.pix_mp.pixelformat == codecs[i].src_fmt->pixelformat
				) {
					codec = &codecs[i];
					max_width = codec->max_width;
					max_height = codec->max_height;
					fmt_set->fmt.pix_mp.pixelformat = 
						is_set_src_fmt ?
						codecs[i].src_fmt->pixelformat :
						codecs[i].dst_fmt->pixelformat;
					break;
				}
			}
		}
	}

	// Otherwise : heurisitc match
	if (!codec) {
		max_width = 0;
		max_height = 0;

		// Match the codecs of the set pixel format
		for(i = 0; i < num_codecs; i++) {

			pxfmt_match = 
				is_set_src_fmt ?
				codecs[i].src_fmt->pixelformat :
				codecs[i].dst_fmt->pixelformat;

			if (fmt_set->fmt.pix_mp.pixelformat == pxfmt_match) {
				if (codecs[i].max_width > max_width) {
					max_width = codecs[i].max_width; 
				}

				if (codecs[i].max_height > max_height) {
					max_height = codecs[i].max_height; 
				}

				codec = &codecs[i];
			}
		}

		// Fallback to the first supported codec format
		if (!codec || max_width == 0 || max_height == 0) {
			dev_warn(core->dev, "No matching pixel format found");
			codec = &codecs[0];
			max_width = codec->max_width;
			max_height = codec->max_height;
			fmt_set->fmt.pix_mp.pixelformat = 
				is_set_src_fmt ?
				codec->src_fmt->pixelformat :
				codec->dst_fmt->pixelformat;
		}
	}

	// Set max resolutions
	fmt_set->fmt.pix_mp.width = clamp(
		fmt_set->fmt.pix_mp.width,
		MIN_RESOLUTION_WIDTH,
		max_width
	);
	fmt_set->fmt.pix_mp.height = clamp(
		fmt_set->fmt.pix_mp.height,
		MIN_RESOLUTION_HEIGHT,
		max_height
	);

	// Set buffer sizes
	if (is_set_src_fmt) {
		set_buffer_sizes(fmt_set, codec->src_fmt);
	} else {
		set_buffer_sizes(fmt_set, codec->dst_fmt);
	}

	return 0;
}

static int meson_vcodec_try_fmt_vid(struct file *file, void *priv, struct v4l2_format *f)
{
	struct meson_vcodec_core *core = video_drvdata(file);
	struct meson_vcodec_session *session = container_of(file->private_data, struct meson_vcodec_session, fh);
	struct v4l2_format *fmt_src, *fmt_dst;

	stream_trace(session, f->type, "fmt=%.4s", (char*)&f->fmt.pix_mp.pixelformat);

	if (IS_SRC_STREAM(f->type)) {
		fmt_src = f;
		fmt_dst = &session->dst_fmt;
	} else {
		fmt_src = &session->src_fmt;
		fmt_dst = f;
	}

	return common_try_fmt_vid(core, f->type, fmt_src, fmt_dst);
}

static int meson_vcodec_s_fmt_vid(struct file *file, void *priv, struct v4l2_format *f)
{
	struct meson_vcodec_core *core = video_drvdata(file);
	struct meson_vcodec_session *session = container_of(file->private_data, struct meson_vcodec_session, fh);
	const struct meson_codec_formats *codec;
	struct v4l2_format *fmt_src, *fmt_dst;
	int ret;

	stream_trace(session, f->type, "fmt=%.4s", (char*)&f->fmt.pix_mp.pixelformat);

	if (IS_SRC_STREAM(f->type)) {
		fmt_src = f;
		fmt_dst = &session->dst_fmt;
	} else {
		fmt_src = &session->src_fmt;
		fmt_dst = f;
	}

	ret = common_try_fmt_vid(core, f->type, fmt_src, fmt_dst);
	if (ret)
		return ret;

	if (IS_SRC_STREAM(f->type)) {
		session->src_fmt = *f;
	} else {
		session->dst_fmt = *f;
	}

	session->type = SESSION_TYPE_NONE;
	session->src_status = STREAM_STATUS_NONE;
	session->dst_status = STREAM_STATUS_NONE;
	session->enc_job.codec = NULL;
	session->dec_job.codec = NULL;
	session->enc_job.src_fmt = NULL;
	session->enc_job.dst_fmt = NULL;
	session->dec_job.src_fmt = NULL;
	session->dec_job.dst_fmt = NULL;

	// Once both source and destination formats are set
	if (session->dst_fmt.fmt.pix_mp.pixelformat &&
		session->dst_fmt.fmt.pix_mp.width &&
		session->dst_fmt.fmt.pix_mp.height &&
		session->src_fmt.fmt.pix_mp.pixelformat &&
		session->src_fmt.fmt.pix_mp.width &&
		session->src_fmt.fmt.pix_mp.height
	) {
		session_dbg(session, "Formats set: src_fmt=%.4s, dst_fmt=%.4s", (char*)&session->src_fmt.fmt.pix_mp.pixelformat, (char*)&session->dst_fmt.fmt.pix_mp.pixelformat);

		codec = find_codec(
			core,
			fmt_src->fmt.pix_mp.pixelformat,
			fmt_dst->fmt.pix_mp.pixelformat
		);
		if (!codec) {
			session_err(session, "Codec formats not found");
			return -EINVAL;
		}

		// Set max resolutions
		fmt_src->fmt.pix_mp.width = clamp(
			fmt_src->fmt.pix_mp.width,
			MIN_RESOLUTION_WIDTH,
			codec->max_width
		);
		fmt_src->fmt.pix_mp.height = clamp(
			fmt_src->fmt.pix_mp.height,
			MIN_RESOLUTION_HEIGHT,
			codec->max_height
		);
		fmt_dst->fmt.pix_mp.width = fmt_src->fmt.pix_mp.width;
		fmt_dst->fmt.pix_mp.height = fmt_src->fmt.pix_mp.height;

		// Set buffer sizes
		set_buffer_sizes(fmt_src, codec->src_fmt);
		set_buffer_sizes(fmt_dst, codec->dst_fmt);

		// Set session type and codec jobs
		session->enc_job.session = session;
		session->dec_job.session = session;
		session->enc_job.codec = codec->encoder;
		session->dec_job.codec = codec->decoder;

		if (codec->decoder && codec->encoder) {
			session->type = SESSION_TYPE_TRANSCODE;
			session->dec_job.src_fmt = &session->src_fmt.fmt.pix_mp;
			session->enc_job.dst_fmt = &session->dst_fmt.fmt.pix_mp;

			session->int_fmt.pixelformat = codec->int_fmt->pixelformat;
			session->int_fmt.width = session->dec_job.src_fmt->width;
			session->int_fmt.height = session->dec_job.src_fmt->height;
			session->int_fmt.num_planes = 0;
			session->dec_job.dst_fmt = &session->int_fmt;
			session->enc_job.src_fmt = &session->int_fmt;

		} else if (codec->encoder) {
			session->type = SESSION_TYPE_ENCODE;
			session->enc_job.src_fmt = &session->src_fmt.fmt.pix_mp;
			session->enc_job.dst_fmt = &session->dst_fmt.fmt.pix_mp;
			session->dec_job.src_fmt = NULL;
			session->dec_job.dst_fmt = NULL;

		} else if (codec->decoder) {
			session->type = SESSION_TYPE_DECODE;
			session->enc_job.src_fmt = NULL;
			session->enc_job.dst_fmt = NULL;
			session->dec_job.src_fmt = &session->src_fmt.fmt.pix_mp;
			session->dec_job.dst_fmt = &session->dst_fmt.fmt.pix_mp;

		} else {
			session_err(session, "Codec formats has no encoder or decoder set");
			return -EINVAL;
		}

		// Set stream status
		session->src_status = STREAM_STATUS_FMTSET;
		session->dst_status = STREAM_STATUS_FMTSET;
	}

	return 0;
}

static int meson_vcodec_g_fmt_vid(struct file *file, void *priv, struct v4l2_format *f)
{
	struct meson_vcodec_session *session = container_of(file->private_data, struct meson_vcodec_session, fh);

	if (IS_SRC_STREAM(f->type))
		*f = session->src_fmt;
	else
		*f = session->dst_fmt;

	stream_trace(session, f->type, "fmt=%.4s", (char*)&f->fmt.pix_mp.pixelformat);

	return 0;
}

static int meson_vcodec_subscribe_event(struct v4l2_fh *fh, const struct v4l2_event_subscription *sub)
{
	struct meson_vcodec_session *session = container_of(fh, struct meson_vcodec_session, fh);

	session_trace(session, "type=%d", sub->type);

	switch (sub->type) {
		case V4L2_EVENT_EOS:
		case V4L2_EVENT_SOURCE_CHANGE:
			return v4l2_event_subscribe(fh, sub, 0, NULL);
		case V4L2_EVENT_CTRL:
			return v4l2_ctrl_subscribe_event(fh, sub);
		default:
			return -EINVAL;
	}
}

static int meson_vcodec_decoder_cmd(struct file *file, void *fh, struct v4l2_decoder_cmd *cmd)
{
	struct meson_vcodec_session *session = container_of(fh, struct meson_vcodec_session, fh);

	session_trace(session);

	return v4l2_m2m_ioctl_decoder_cmd(file, fh, cmd);
}

static int meson_vcodec_encoder_cmd(struct file *file, void *fh, struct v4l2_encoder_cmd *cmd)
{
	struct meson_vcodec_session *session = container_of(fh, struct meson_vcodec_session, fh);

	session_trace(session);

	return v4l2_m2m_ioctl_encoder_cmd(file, fh, cmd);
}

static const struct v4l2_ioctl_ops meson_vcodec_ioctl_ops = {
	.vidioc_querycap = meson_vcodec_querycap,
	.vidioc_enum_fmt_vid_cap = meson_vcodec_enum_fmt_vid,
	.vidioc_enum_fmt_vid_out = meson_vcodec_enum_fmt_vid,
	.vidioc_enum_framesizes = meson_vcodec_enum_framesizes,
	.vidioc_try_fmt_vid_cap_mplane = meson_vcodec_try_fmt_vid,
	.vidioc_try_fmt_vid_out_mplane = meson_vcodec_try_fmt_vid,
	.vidioc_s_fmt_vid_cap_mplane = meson_vcodec_s_fmt_vid,
	.vidioc_s_fmt_vid_out_mplane = meson_vcodec_s_fmt_vid,
	.vidioc_g_fmt_vid_cap_mplane = meson_vcodec_g_fmt_vid,
	.vidioc_g_fmt_vid_out_mplane = meson_vcodec_g_fmt_vid,
	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,
/* TODO
	.vidioc_g_pixelaspect = meson_vcodec_g_pixelaspect,
*/
	.vidioc_subscribe_event = meson_vcodec_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
	.vidioc_try_encoder_cmd = v4l2_m2m_ioctl_try_encoder_cmd,
	.vidioc_encoder_cmd = meson_vcodec_encoder_cmd,
	.vidioc_try_decoder_cmd = v4l2_m2m_ioctl_try_decoder_cmd,
	.vidioc_decoder_cmd = meson_vcodec_decoder_cmd,
};

/* ressource helpers */

s32 meson_vcodec_g_ctrl(struct meson_vcodec_session *session, u32 id) {
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(&session->ctrl_handler, id);
	if (ctrl) {
		return v4l2_ctrl_g_ctrl(ctrl);
	} else {
		session_warn(session, "ctrl %d not found", id);
		return 0;
	}
}

int meson_vcodec_s_ctrl(struct meson_vcodec_session *session, u32 id, s32 val) {
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(&session->ctrl_handler, id);
	if (ctrl) {
		return v4l2_ctrl_s_ctrl(ctrl, val);
	} else {
		session_warn(session, "ctrl %d not found", id);
		return -EINVAL;
	}
}

int meson_vcodec_reset(struct meson_vcodec_core *core, enum meson_vcodec_reset index) {
	int ret;

	ret = reset_control_reset(core->resets[index]);
	if (ret < 0)
		return ret;

	return 0;
}

void meson_vcodec_clk_unprepare(struct meson_vcodec_core *core, enum meson_vcodec_clk index) {
	clk_disable_unprepare(core->clks[index]);
}


int meson_vcodec_clk_prepare(struct meson_vcodec_core *core, enum meson_vcodec_clk index, u64 rate) {
	int ret;

	if (rate) {
		ret = clk_set_rate(core->clks[index], rate);
		if (ret < 0)
			return ret;
	}

	ret = clk_prepare_enable(core->clks[index]);
	if (ret < 0)
		return ret;

	return 0;
}

int meson_vcodec_pwrc_off(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index) {
	int ret;

	ret = regmap_update_bits(core->regmaps[BUS_AO],
			core->platform_specs->pwrc[index].sleep_reg,
			core->platform_specs->pwrc[index].sleep_mask,
			core->platform_specs->pwrc[index].sleep_mask);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(core->regmaps[BUS_AO],
			core->platform_specs->pwrc[index].iso_reg,
			core->platform_specs->pwrc[index].iso_mask,
			core->platform_specs->pwrc[index].iso_mask);
	if (ret < 0)
		return ret;

	return 0;
}

int meson_vcodec_pwrc_on(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index) {
	int ret;

	ret = regmap_update_bits(core->regmaps[BUS_AO],
			core->platform_specs->pwrc[index].sleep_reg,
			core->platform_specs->pwrc[index].sleep_mask, 0);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(core->regmaps[BUS_AO],
			core->platform_specs->pwrc[index].iso_reg,
			core->platform_specs->pwrc[index].iso_mask, 0);
	if (ret < 0)
		return ret;

	return 0;
}

/* platform_driver */

static int meson_vcodec_probe(struct platform_device *pdev)
{
	struct meson_vcodec_core *core;
	const struct meson_platform_specs *platform_specs;
	struct video_device *vfd;
	struct resource *res;
	resource_size_t reg_size;
	void __iomem *base_addr;
	int ret, i;

	platform_specs = of_device_get_match_data(&pdev->dev);
	if (!platform_specs) {
		dev_err(&pdev->dev, "Failed to get platform data\n");
		return -ENODEV;
	}

	core = devm_kzalloc(&pdev->dev, sizeof(*core), GFP_KERNEL);
	if (!core)
		return -ENOMEM;

	atomic_set(&core->session_counter, 0);
	core->dev = &pdev->dev;
	core->platform_specs = platform_specs;
	platform_set_drvdata(pdev, core);
	MESON_VCODEC_CORE = core;

	for (i = 0; i < MAX_BUS; i++) {
		if (!regmap_names[i])
			continue;

		core->regmaps[i] = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, regmap_names[i]);
		if (IS_ERR(core->regmaps[i])) {
			dev_err(&pdev->dev, "Failed to get %s regmap\n", regmap_names[i]);
			return PTR_ERR(core->regmaps[i]);
		}
	}

	for (i = 0; i < MAX_BUS; i++) {
		if (!reg_names[i])
			continue;

		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, reg_names[i]);
		if (!res) {
			dev_err(&pdev->dev, "Failed to get register %s\n", reg_names[i]);
			return -ENODEV;
		}

		reg_size = resource_size(res);
		base_addr = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(base_addr)) {
			dev_err(&pdev->dev, "Failed to map %s register\n", reg_names[i]);
			return PTR_ERR(base_addr);
		}

		core->regs[i] = base_addr;

		static struct regmap_config regmap_config = {
			.reg_bits = 32,
			.val_bits = 32,
			.reg_stride = 4,
			//.reg_base = -0x1000,
			//.reg_shift = -2
			.use_relaxed_mmio = true,
		};
		regmap_config.max_register = ((reg_size - 1) - regmap_config.reg_base) << regmap_config.reg_shift;

		core->regmaps[i] = devm_regmap_init_mmio(&pdev->dev, base_addr, &regmap_config);
		if (IS_ERR(core->regmaps[i])) {
			dev_err(&pdev->dev, "Failed to init %s regmap\n", regmap_names[i]);
			return PTR_ERR(core->regmaps[i]);
		}
	}

	ret = meson_platform_register_clks(core);
	if (ret) {
		dev_err(&pdev->dev, "Failed to registers platform clocks\n");
		return ret;
	}

	for (i = 0; i < MAX_CLKS; i++) {
		if (i == CLK_HEVCF && platform_specs->platform_id < AM_MESON_CPU_MAJOR_ID_G12A)
			continue;

		core->clks[i] = devm_clk_get(&pdev->dev, clk_names[i]);
		if (IS_ERR(core->clks[i])) {
			dev_err(&pdev->dev, "Failed to get %s clock\n", clk_names[i]);
			return PTR_ERR(core->clks[i]);
		}
	}

	for (i = 0; i < MAX_RESETS; i++) {
		if (i == RESET_HCODEC && platform_specs->platform_id < AM_MESON_CPU_MAJOR_ID_SC2)
			continue;

		core->resets[i] = devm_reset_control_get(&pdev->dev, reset_names[i]);
		if (IS_ERR(core->resets[i])) {
			dev_err(&pdev->dev, "Failed to get %s reset\n", reset_names[i]);
			return PTR_ERR(core->resets[i]);
		}
	}

	for (i = 0; i < MAX_IRQS; i++) {
		core->irqs[i] = platform_get_irq_byname(pdev, irq_names[i]);
		if (core->irqs[i] < 0) {
			dev_err(&pdev->dev, "Failed to get %s IRQ\n", irq_names[i]);
			return core->irqs[i];
		}
	}

	core->canvas = meson_canvas_get(&pdev->dev);
	if (IS_ERR(core->canvas)) {
		dev_err(&pdev->dev, "Failed to get canvas\n");
		return PTR_ERR(core->canvas);
	}

	ret = v4l2_device_register(&pdev->dev, &core->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register V4L2 device\n");
		return ret;
	}

	core->m2m_dev = v4l2_m2m_init(&meson_vcodec_m2m_ops);
	if (IS_ERR(core->m2m_dev)) {
		dev_err(&pdev->dev, "Failed to initialize V4L2 M2M device\n");
		ret = PTR_ERR(core->m2m_dev);
		goto err_v4l2_unregister;
	}

	vfd = &core->vfd;
	strscpy(vfd->name, "meson-video-codec", sizeof(vfd->name));
	vfd->lock = &core->lock;
	vfd->v4l2_dev = &core->v4l2_dev;
	vfd->fops = &meson_vcodec_fops;
	vfd->ioctl_ops = &meson_vcodec_ioctl_ops;
	vfd->release = video_device_release_empty;
	vfd->vfl_dir = VFL_DIR_M2M;
	vfd->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;

	ret = video_register_device(vfd, VFL_TYPE_VIDEO, 0);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register video device\n");
		goto err_m2m_release;
	}

	video_set_drvdata(vfd, core);
	dev_dbg(&pdev->dev, "Device registered as %s\n", video_device_node_name(vfd));

	mutex_init(&core->lock);

	return 0;

err_m2m_release:
	v4l2_m2m_release(core->m2m_dev);
err_v4l2_unregister:
	v4l2_device_unregister(&core->v4l2_dev);
	return ret;
}

static int meson_vcodec_remove(struct platform_device *pdev)
{
	struct meson_vcodec_core *core = platform_get_drvdata(pdev);

	MESON_VCODEC_CORE = NULL;
	video_unregister_device(&core->vfd);
	v4l2_m2m_release(core->m2m_dev);
	v4l2_device_unregister(&core->v4l2_dev);
	mutex_destroy(&core->lock);

	return 0;
}

static const struct of_device_id meson_vcodec_match[] = {
	{ .compatible = "amlogic,meson-gxl-vcodec", .data = &gxl_platform_specs },
	// add other supported platforms 
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, meson_vcodec_match);

static struct platform_driver meson_vcodec_driver = {
	.probe = meson_vcodec_probe,
	.remove = meson_vcodec_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = meson_vcodec_match,
	},
};

module_platform_driver(meson_vcodec_driver);

MODULE_DESCRIPTION("Amlogic Meson Video Codec Driver");
MODULE_LICENSE("GPL v2");
