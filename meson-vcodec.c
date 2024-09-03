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

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"
#include "meson-vcodec.h"

#define DRIVER_NAME "meson-vcodec"

static const char* reg_names[MAX_REGS] = {
	[DOS_BASE] = "dos",
	[PARSER_BASE] = "esparser",
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
};

/* vb2_ops */

static int meson_vcodec_queue_setup(struct vb2_queue *vq,
		unsigned int *nbuffers,
		unsigned int *nplanes,
		unsigned int sizes[],
		struct device *alloc_devs[])
{
	struct meson_vcodec_session *session = vb2_get_drv_priv(vq);
	struct v4l2_format *fmt;

	fmt = V4L2_TYPE_IS_OUTPUT(vq->type) ?
		&session->input_format : &session->output_format;

	if (*nplanes) {
		if (*nplanes != fmt->fmt.pix_mp.num_planes)
			return -EINVAL;
		return 0;
	}

	*nplanes = fmt->fmt.pix_mp.num_planes;
	for (int i = 0; i < *nplanes; i++)
		sizes[i] = fmt->fmt.pix_mp.plane_fmt[i].sizeimage;

	return 0;
}

static int meson_vcodec_buf_prepare(struct vb2_buffer *vb)
{
	struct meson_vcodec_session *session = vb2_get_drv_priv(vb->vb2_queue);
	struct v4l2_format *fmt;

	fmt = V4L2_TYPE_IS_OUTPUT(vb->type) ?
		&session->input_format : &session->output_format;

	for (int i = 0; i < fmt->fmt.pix_mp.num_planes; i++) {
		if (vb2_plane_size(vb, i) < fmt->fmt.pix_mp.plane_fmt[i].sizeimage)
			return -EINVAL;
	}

	return 0;
}

static void meson_vcodec_buf_queue(struct vb2_buffer *vb)
{
	struct meson_vcodec_session *session = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	v4l2_m2m_buf_queue(session->m2m_ctx, vbuf);
}

static int meson_vcodec_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct meson_vcodec_session *session = vb2_get_drv_priv(vq);
	struct meson_vcodec *vcodec = session->parent;
	int ret = 0;

	dev_dbg(vcodec->dev, "Start streaming for queue type %d\n", vq->type);

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		ret = session->codec->decoder->ops->start(session);
	} else {
		ret = session->codec->encoder->ops->start(session);
	}

	if (ret) {
		dev_err(vcodec->dev, "Failed to start %s: %d\n",
				V4L2_TYPE_IS_OUTPUT(vq->type) ? "decoder" : "encoder", ret);
		return ret;
	}

	return 0;
}

static void meson_vcodec_stop_streaming(struct vb2_queue *vq)
{
	struct meson_vcodec_session *session = vb2_get_drv_priv(vq);
	struct meson_vcodec *vcodec = session->parent;
	struct vb2_v4l2_buffer *vbuf;

	dev_dbg(vcodec->dev, "Stop streaming for queue type %d\n", vq->type);

	if (V4L2_TYPE_IS_OUTPUT(vq->type)) {
		session->codec->decoder->ops->stop(session);
	} else {
		session->codec->encoder->ops->stop(session);
	}

	// Return all buffers to vb2 in QUEUED state
	for (;;) {
		if (V4L2_TYPE_IS_OUTPUT(vq->type))
			vbuf = v4l2_m2m_src_buf_remove(session->m2m_ctx);
		else
			vbuf = v4l2_m2m_dst_buf_remove(session->m2m_ctx);

		if (!vbuf)
			break;

		v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
	}
}

static const struct vb2_ops meson_vcodec_vb2_ops = {
	.queue_setup     = meson_vcodec_queue_setup,
	.buf_prepare     = meson_vcodec_buf_prepare,
	.buf_queue       = meson_vcodec_buf_queue,
	.start_streaming = meson_vcodec_start_streaming,
	.stop_streaming  = meson_vcodec_stop_streaming,
	.wait_prepare    = vb2_ops_wait_prepare,
	.wait_finish     = vb2_ops_wait_finish,
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
	src_vq->dev = session->parent->dev;
	src_vq->lock = &session->lock;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->ops = &meson_vcodec_vb2_ops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->drv_priv = session;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->min_buffers_needed = 1;
	dst_vq->dev = session->parent->dev;
	dst_vq->lock = &session->lock;

	return vb2_queue_init(src_vq) || vb2_queue_init(dst_vq);
}

static int meson_vcodec_session_open(struct file *file)
{
	struct meson_vcodec *vcodec = video_drvdata(file);
	struct meson_vcodec_session *session;
	int ret = 0;

	if (vcodec->current_session) {
		dev_err(vcodec->dev, 
				"Existing session %d active, can't open a new session\n",
				vcodec->current_session->session_id);
		return -EBUSY;
	}

	dev_dbg(vcodec->dev, "Opening new session\n");

	session = kzalloc(sizeof(*session), GFP_KERNEL);
	if (!session)
		return -ENOMEM;

	session->parent = vcodec;
	session->session_id = atomic_inc_return(&vcodec->session_counter);

	v4l2_fh_init(&session->fh, &vcodec->vfd);
	file->private_data = &session->fh;
	v4l2_fh_add(&session->fh);

	ret = v4l2_ctrl_handler_init(&session->ctrl_handler, 0);
	if (ret) {
		dev_err(vcodec->dev, "Failed to initialize control handler\n");
		goto err_fh_exit;
	}
	session->fh.ctrl_handler = &session->ctrl_handler;

	session->m2m_ctx = v4l2_m2m_ctx_init(vcodec->m2m_dev, session, meson_vcodec_m2m_queue_init);
	if (IS_ERR(session->fh.m2m_ctx)) {
		dev_err(vcodec->dev, "Failed to initialize v4l2-m2m context\n");
		ret = PTR_ERR(session->fh.m2m_ctx);
		goto err_ctrl_handler_free;
	}
	session->fh.m2m_ctx = session->m2m_ctx;

	init_completion(&session->frame_completion);
	mutex_init(&session->lock);

	dev_dbg(vcodec->dev, "Session opened successfully\n");

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
	struct meson_vcodec *vcodec = session->parent;

	dev_dbg(vcodec->dev, "Releasing session %d\n", session->session_id);

	v4l2_m2m_ctx_release(session->m2m_ctx);
	v4l2_ctrl_handler_free(&session->ctrl_handler);
	v4l2_fh_del(&session->fh);
	v4l2_fh_exit(&session->fh);
	kfree(session);

	vcodec->current_session = NULL;

	dev_dbg(vcodec->dev, "Session released successfully\n");

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
	struct meson_vcodec *vcodec = video_drvdata(file);

	strscpy(cap->driver, DRIVER_NAME, sizeof(cap->driver));
	strscpy(cap->card, "Meson Video Codec", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s", dev_name(vcodec->dev));

	return 0;
}

static int meson_vcodec_enum_fmt_vid(struct file *file, void *priv,
		struct v4l2_fmtdesc *f)
{
	struct meson_vcodec *vcodec = video_drvdata(file);
	const struct meson_codec_formats *formats = vcodec->platform_specs->formats;
	int num_formats = vcodec->platform_specs->num_formats;
	int i, count = 0;

	for (i = 0; i < num_formats; i++) {
		if (V4L2_TYPE_IS_OUTPUT(f->type)) {
			if (count == f->index) {
				f->pixelformat = formats[i].input_format->pixelformat;
				strscpy(f->description, formats[i].input_format->description,
						sizeof(f->description));
				return 0;
			}
			count++;
		} else {
			if (count == f->index) {
				f->pixelformat = formats[i].output_format->pixelformat;
				strscpy(f->description, formats[i].output_format->description,
						sizeof(f->description));
				return 0;
			}
			count++;
		}
	}

	return -EINVAL;
}

static int meson_vcodec_try_fmt_vid(struct file *file, void *priv,
		struct v4l2_format *f)
{
	struct meson_vcodec *vcodec = video_drvdata(file);
	const struct meson_codec_formats *formats = vcodec->platform_specs->formats;
	int num_formats = vcodec->platform_specs->num_formats;
	int i;

	for (i = 0; i < num_formats; i++) {
		const struct meson_format *fmt = V4L2_TYPE_IS_OUTPUT(f->type) ?
			formats[i].input_format :
			formats[i].output_format;

		if (fmt->pixelformat == f->fmt.pix_mp.pixelformat) {
			int j;

			f->fmt.pix_mp.num_planes = fmt->num_planes;
			for (j = 0; j < fmt->num_planes; j++) {
				f->fmt.pix_mp.plane_fmt[j].sizeimage =
					(f->fmt.pix_mp.width * f->fmt.pix_mp.height) /
					fmt->plane_size_denums[j];
			}

			return 0;
		}
	}

	return -EINVAL;
}

static int meson_vcodec_s_fmt_vid(struct file *file, void *priv, struct v4l2_format *f)
{
	struct meson_vcodec_session *session = container_of(file->private_data, struct meson_vcodec_session, fh);
	int ret;

	ret = meson_vcodec_try_fmt_vid(file, priv, f);
	if (ret)
		return ret;

	if (V4L2_TYPE_IS_OUTPUT(f->type))
		session->input_format = *f;
	else
		session->output_format = *f;

	return 0;
}

static int meson_vcodec_g_fmt_vid(struct file *file, void *priv, struct v4l2_format *f)
{
	struct meson_vcodec_session *session = container_of(file->private_data, struct meson_vcodec_session, fh);

	if (V4L2_TYPE_IS_OUTPUT(f->type))
		f = &session->input_format;
	else
		f = &session->output_format;

	return 0;
}

static const struct v4l2_ioctl_ops meson_vcodec_ioctl_ops = {
	.vidioc_querycap = meson_vcodec_querycap,
	.vidioc_enum_fmt_vid_cap = meson_vcodec_enum_fmt_vid,
	.vidioc_enum_fmt_vid_out = meson_vcodec_enum_fmt_vid,
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
	.vidioc_enum_framesizes = meson_vcodec_enum_framesizes,
	.vidioc_g_pixelaspect = meson_vcodec_g_pixelaspect,
	.vidioc_subscribe_event = meson_vcodec_subscribe_event,
	..
	.vidioc_decoder_cmd = meson_vcodec_decoder_cmd,
*/
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
	.vidioc_try_decoder_cmd = v4l2_m2m_ioctl_try_decoder_cmd,
	.vidioc_decoder_cmd = v4l2_m2m_ioctl_decoder_cmd,
};

/* v4l2_m2m_ops */

static void meson_vcodec_m2m_device_run(void *priv)
{
	struct meson_vcodec_session *session = priv;

	// TODO schedule_work parser_queue_work
}

static void meson_vcodec_m2m_job_abort(void *priv)
{
	struct meson_vcodec_session *session = priv;

	v4l2_m2m_job_finish(session->parent->m2m_dev, session->m2m_ctx);
}

static const struct v4l2_m2m_ops meson_vcodec_m2m_ops = {
	.device_run = meson_vcodec_m2m_device_run,
	.job_abort = meson_vcodec_m2m_job_abort,
};

/* ressource helpers */

int meson_vcodec_reset(struct meson_vcodec *vcodec, enum meson_vcodec_reset index) {
	int ret;

	ret = reset_control_reset(vcodec->resets[index]);
	if (ret < 0)
		return ret;

	return 0;
}

void meson_vcodec_clk_unprepare(struct meson_vcodec *vcodec, enum meson_vcodec_clk index) {
	clk_disable_unprepare(vcodec->clks[index]);
}


int meson_vcodec_clk_prepare(struct meson_vcodec *vcodec, enum meson_vcodec_clk index, u64 rate) {
	int ret;

	if (rate) {
		ret = clk_set_rate(vcodec->clks[index], rate);
		if (ret < 0)
			return ret;
	}

	ret = clk_prepare_enable(vcodec->clks[index]);
	if (ret < 0)
		return ret;

	return 0;
}

u32 meson_vcodec_reg_read(struct meson_vcodec *vcodec, enum meson_vcodec_regs index, u32 reg) {
	return readl_relaxed(vcodec->regs[index] + reg);
}


void meson_vcodec_reg_write(struct meson_vcodec *vcodec, enum meson_vcodec_regs index, u32 reg, u32 value) {
	writel_relaxed(value, vcodec->regs[index] + reg);
}

int meson_vcodec_pwrc_off(struct meson_vcodec *vcodec, enum meson_vcodec_pwrc index) {
	int ret;

	ret = regmap_update_bits(vcodec->regmap_ao,
			vcodec->platform_specs->pwrc[index].sleep_reg,
			vcodec->platform_specs->pwrc[index].sleep_mask,
			vcodec->platform_specs->pwrc[index].sleep_mask);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(vcodec->regmap_ao,
			vcodec->platform_specs->pwrc[index].iso_reg,
			vcodec->platform_specs->pwrc[index].iso_mask,
			vcodec->platform_specs->pwrc[index].iso_mask);
	if (ret < 0)
		return ret;

	return 0;
}

int meson_vcodec_pwrc_on(struct meson_vcodec *vcodec, enum meson_vcodec_pwrc index) {
	int ret;

	ret = regmap_update_bits(vcodec->regmap_ao,
			vcodec->platform_specs->pwrc[index].sleep_reg,
			vcodec->platform_specs->pwrc[index].sleep_mask, 0);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(vcodec->regmap_ao,
			vcodec->platform_specs->pwrc[index].iso_reg,
			vcodec->platform_specs->pwrc[index].iso_mask, 0);
	if (ret < 0)
		return ret;

	return 0;
}

static irqreturn_t meson_vcodec_isr(int irq, void *dev_id)
{
	struct meson_vcodec *vcodec = dev_id;
	struct meson_vcodec_session *session = vcodec->current_session;
	const struct meson_codec_ops *ops;

	if (!session)
		return IRQ_NONE;

	ops = (session->codec->decoder) ? session->codec->decoder->ops : session->codec->encoder->ops;

	return ops->isr(irq, dev_id);
}

/* platform_driver */

static int meson_vcodec_probe(struct platform_device *pdev)
{
	struct meson_vcodec *vcodec;
	const struct meson_platform_specs *platform_specs;
	struct video_device *vfd;
	int ret, i;

	platform_specs = of_device_get_match_data(&pdev->dev);
	if (!platform_specs) {
		dev_err(&pdev->dev, "Failed to get platform data\n");
		return -ENODEV;
	}

	vcodec = devm_kzalloc(&pdev->dev, sizeof(*vcodec), GFP_KERNEL);
	if (!vcodec)
		return -ENOMEM;

	atomic_set(&vcodec->session_counter, 0);
	vcodec->dev = &pdev->dev;
	vcodec->platform_specs = platform_specs;
	platform_set_drvdata(pdev, vcodec);

	mutex_init(&vcodec->lock);

	vcodec->regmap_ao = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "amlogic,ao-sysctrl");
	if (IS_ERR(vcodec->regmap_ao)) {
		dev_err(&pdev->dev, "Failed to get ao-sysctrl regmap\n");
		ret = PTR_ERR(vcodec->regmap_ao);
		goto err_mutex_destroy;
	}

	for (i = 0; i < MAX_REGS; i++) {
		vcodec->regs[i] = devm_platform_ioremap_resource_byname(pdev, reg_names[i]);
		if (IS_ERR(vcodec->regs[i])) {
			dev_err(&pdev->dev, "Failed to map %s registers\n", reg_names[i]);
			ret = PTR_ERR(vcodec->regs[i]);
			goto err_mutex_destroy;
		}
	}

	for (i = 0; i < MAX_CLKS; i++) {
		if (i == CLK_HEVCF && platform_specs->platform_id < MESON_MAJOR_ID_G12A)
			continue;

		if (i == CLK_HCODEC && platform_specs->platform_id < MESON_MAJOR_ID_SC2)
			continue;

		vcodec->clks[i] = devm_clk_get(&pdev->dev, clk_names[i]);
		if (IS_ERR(vcodec->clks[i])) {
			if (PTR_ERR(vcodec->clks[i]) != -ENOENT) {
				dev_err(&pdev->dev, "Failed to get %s clock\n", clk_names[i]);
				ret = PTR_ERR(vcodec->clks[i]);
				goto err_mutex_destroy;
			}
			vcodec->clks[i] = NULL;
		}
	}

	for (i = 0; i < MAX_RESETS; i++) {
		if (i == RESET_HCODEC && platform_specs->platform_id < MESON_MAJOR_ID_SC2)
			continue;

		vcodec->resets[i] = devm_reset_control_get(&pdev->dev, reset_names[i]);
		if (IS_ERR(vcodec->resets[i])) {
			if (PTR_ERR(vcodec->resets[i]) != -ENOENT) {
				dev_err(&pdev->dev, "Failed to get %s reset\n", reset_names[i]);
				ret = PTR_ERR(vcodec->resets[i]);
				goto err_mutex_destroy;
			}
			vcodec->resets[i] = NULL;
		}
	}

	for (i = 0; i < MAX_IRQS; i++) {
		vcodec->irqs[i] = platform_get_irq_byname(pdev, irq_names[i]);
		if (vcodec->irqs[i] < 0) {
			dev_err(&pdev->dev, "Failed to get %s IRQ\n", irq_names[i]);
			ret = vcodec->irqs[i];
			goto err_mutex_destroy;
		}

		ret = devm_request_irq(&pdev->dev, vcodec->irqs[i], meson_vcodec_isr,
				IRQF_SHARED, dev_name(&pdev->dev), vcodec);
		if (ret) {
			dev_err(&pdev->dev, "Failed to install %s IRQ handler\n", irq_names[i]);
			goto err_mutex_destroy;
		}
	}

	init_waitqueue_head(&vcodec->queue);

	ret = v4l2_device_register(&pdev->dev, &vcodec->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register V4L2 device\n");
		goto err_mutex_destroy;
	}

	vcodec->m2m_dev = v4l2_m2m_init(&meson_vcodec_m2m_ops);
	if (IS_ERR(vcodec->m2m_dev)) {
		dev_err(&pdev->dev, "Failed to initialize V4L2 M2M device\n");
		ret = PTR_ERR(vcodec->m2m_dev);
		goto err_v4l2_unregister;
	}

	vfd = &vcodec->vfd;
	strscpy(vfd->name, "meson-video-codec", sizeof(vfd->name));
	vfd->lock = &vcodec->lock;
	vfd->v4l2_dev = &vcodec->v4l2_dev;
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

	video_set_drvdata(vfd, vcodec);
	dev_dbg(&pdev->dev, "Device registered as %s\n", video_device_node_name(vfd));

	return 0;

err_m2m_release:
	v4l2_m2m_release(vcodec->m2m_dev);
err_v4l2_unregister:
	v4l2_device_unregister(&vcodec->v4l2_dev);
err_mutex_destroy:
	mutex_destroy(&vcodec->lock);
	return ret;
}

static int meson_vcodec_remove(struct platform_device *pdev)
{
	struct meson_vcodec *vcodec = platform_get_drvdata(pdev);

	video_unregister_device(&vcodec->vfd);
	v4l2_m2m_release(vcodec->m2m_dev);
	v4l2_device_unregister(&vcodec->v4l2_dev);
	mutex_destroy(&vcodec->lock);

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
