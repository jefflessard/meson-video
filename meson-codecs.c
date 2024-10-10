#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/iopoll.h>
#include <linux/soc/amlogic/meson-canvas.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

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

