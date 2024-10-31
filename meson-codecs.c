#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/iopoll.h>
#include <linux/soc/amlogic/meson-canvas.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-dma-contig.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-vcodec.h"


int meson_vcodec_request_firmware(struct meson_codec_dev *codec, struct meson_vcodec_buffer *buf) {
	struct meson_vcodec_core *core = codec->core;
	struct device *dev = core->dev;
	const char *path = core->platform_specs->firmwares[codec->spec->type];
	const struct firmware *fw;
	int ret;

	if (!buf || buf->size == 0) {
		codec_err(codec, "No buffer / empty buffer\n");
		return -EINVAL;
	}

	if (!path) {
		codec_err(codec, "No firmware path specified\n");
		return -EINVAL;
	}

	buf->vaddr = dma_alloc_coherent(dev, buf->size, &buf->paddr, GFP_KERNEL);
	if (!buf->vaddr) {
		return -ENOMEM;
	}

	ret = request_firmware_into_buf(&fw, path, dev, buf->vaddr, buf->size);
	if (ret < 0) {
		codec_err(codec, "Failed to request firmware %s\n", path);
		goto free_dma;
	}

	buf->priv = fw;
	codec_info(codec, "Firmware %s loaded\n", path);

	return 0;

free_dma:
	dma_free_coherent(dev, buf->size, buf->vaddr, buf->paddr);
	return ret;
}

void meson_vcodec_release_firmware(struct meson_codec_dev *codec, struct meson_vcodec_buffer *buf) {
	const struct firmware *fw = buf->priv;

	release_firmware(fw);
	dma_free_coherent(codec->core->dev, buf->size, buf->vaddr, buf->paddr);
}

void meson_vcodec_buffers_free(struct meson_codec_dev *codec, struct meson_vcodec_buffer *buffers, int num_buffers) {
	int i;

	for (i = 0; i < num_buffers; i++) {
		if (buffers[i].size) {

			dma_free_coherent(codec->core->dev, buffers[i].size, buffers[i].vaddr, buffers[i].paddr);
		}

		buffers[i].vaddr = NULL;
		buffers[i].paddr = 0;
	}
}

int meson_vcodec_buffers_alloc(struct meson_codec_dev *codec, struct meson_vcodec_buffer *buffers, int num_buffers) {
	int i, ret;

	for (i = 0; i < num_buffers; i++) {
		if (buffers[i].size) {

			buffers[i].vaddr = dma_alloc_coherent(codec->core->dev, buffers[i].size, &buffers[i].paddr, GFP_KERNEL);
			if (!buffers[i].vaddr) {
				ret = -ENOMEM;
				goto free_buffers;
			}
		}
	}

	return 0;

free_buffers:
	while (--i >= 0) {
		if (buffers[i].size) {
			dma_free_coherent(codec->core->dev, buffers[i].size, buffers[i].vaddr, buffers[i].paddr);
		}
		buffers[i].vaddr = NULL;
		buffers[i].paddr = 0;
	}
	return ret;
}

int meson_vcodec_canvas_alloc(struct meson_codec_dev *codec, u8 canvases[], u8 num_canvas) {
	int i, ret;

	for (i = 0; i < num_canvas; i++) {
		ret = meson_canvas_alloc(codec->core->canvas, &canvases[i]);
		if (ret)
			goto free_canvases;
	}

	return 0;

free_canvases:
	while (--i >= 0) {
		meson_canvas_free(codec->core->canvas, canvases[i]);
	}
	return ret;
}

void meson_vcodec_canvas_free(struct meson_codec_dev *codec, u8 canvases[], u8 num_canvas) {
	int i;

	for (i = 0; i < num_canvas; i++) {
		meson_canvas_free(codec->core->canvas, canvases[i]);
	}
}

int meson_vcodec_canvas_config(struct meson_codec_dev *codec, u8 canvas_index, u32 paddr, u32 width, u32 height) {

	return meson_canvas_config(
			codec->core->canvas,
			canvas_index, paddr, width, height,
			MESON_CANVAS_WRAP_NONE, MESON_CANVAS_BLKMODE_LINEAR, 0
		);
}

static int validate_canvas_align(u32 canvas_paddr, u32 canvas_w, u32 canvas_h, u32 width_align, u32 height_align) {
	if ( canvas_w % width_align != 0 ||
		(canvas_w * canvas_h) % height_align != 0) {
		pr_warn(DRIVER_NAME ": " "Canvas width or height is not aligned. Canvas width = %u, Canvas height = %u\n", canvas_w, canvas_h);
		return -EINVAL;
	}

	if (canvas_paddr % width_align != 0) {
		pr_warn(DRIVER_NAME ": " "Canvas physical address is not aligned. Canvas paddr = 0x%x\n", canvas_paddr);
		return -EINVAL;
	}

	return 0;
}

int meson_vcodec_config_vb2_canvas(struct meson_codec_dev *codec, const struct v4l2_format *fmt, struct vb2_buffer *vb2, u8 canvases[MAX_NUM_CANVAS]) {
	const struct meson_format *s;
	u32 width, height;
	u32 y_width, y_height;
	u32 uv_width, uv_height;
	u32 canvas_w, canvas_h;
	dma_addr_t canvas_paddr;
	int i, ret;

	switch (V4L2_FMT_PIXFMT(fmt)) {
		case V4L2_FMT(NV12M):
			s = &nv12;
			break;
		case V4L2_FMT(NV21M):
			s = &nv21;
			break;
		case V4L2_FMT(YUV420M):
			s = &yuv420;
			break;
		default:
			codec_err(codec, "Invalid format: %.4s\n", V4L2_FMT_FOURCC(fmt));
			return -EINVAL;
	}

	if (!V4L2_TYPE_IS_MULTIPLANAR(vb2->type)) {
		codec_err(codec, "Invalid buffer type: %d\n", vb2->type);
		// TODO single plane to multi canvas paddr offset mapping
		return -EINVAL; 
	}

	if (V4L2_TYPE_IS_MULTIPLANAR(vb2->type) && vb2->num_planes != s->num_planes) {
		codec_err(codec, "Invalid num_planes: vb2=%d, fmt=%d\n", vb2->num_planes, s->num_planes);
		return -EINVAL; 
	}

	width = V4L2_FMT_WIDTH(fmt);
	height = V4L2_FMT_HEIGHT(fmt);

	// Y plane
	y_width = ALIGN((width * s->bits_per_px) >> 3, s->align_width);
	y_height = (height * s->bits_per_px) >> 3;

	// UV planes
	uv_width = (y_width * s->uvplane_bppx) >> 3;
	uv_height = (y_height * s->uvplane_bppy) >> 3;

	for (i = 0; i < s->num_planes; i++) {
		canvas_paddr = vb2_dma_contig_plane_dma_addr(vb2, i);
		canvas_w = i == 0 ? y_width : uv_width;
		canvas_h = i == 0 ? y_height : uv_height;

#ifdef DEBUG_CANVAS
		u32 canvas_size = canvas_w * canvas_h;
		codec_trace(codec, "plane %d: bytes=%lu, size=%lu, canvas=%d, canvas_size=%d",
				i,
				vb2_get_plane_payload(vb2, i),
				vb2_plane_size(vb2, i),
				canvases[i], canvas_size);
#endif

		ret = validate_canvas_align(
				canvas_paddr,
				canvas_w, canvas_h,
				s->align_width, s->align_height);

		ret = meson_vcodec_canvas_config(
				codec,
				canvases[i], canvas_paddr,
				canvas_w, canvas_h);
		if (ret) {
			codec_err(codec, "Failed to configure canvas: i=%d, ret=%d\n", i, ret);
			return ret;
		}
	}

	return COMBINE_CANVAS_ARRAY(canvases, s->num_planes);
}
