#ifndef _MESON_VCODEC_H
#define _MESON_VCODEC_H

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"

enum meson_vcodec_regs {
	DOS_BASE,
	PARSER_BASE,
	MAX_REGS
};

enum meson_vcodec_clk {
	CLK_DOS,
	CLK_PARSER,
	CLK_VDEC1,
	CLK_HEVC,
	CLK_HEVCF,
	CLK_HCODEC,
	MAX_CLKS
};

enum meson_vcodec_reset {
	RESET_PARSER,
	RESET_HCODEC,
	MAX_RESETS
};

enum meson_vcodec_irq {
	IRQ_VDEC,
	IRQ_PARSER,
	MAX_IRQS
};

struct meson_vcodec_session;

struct meson_vcodec {
	const struct meson_platform_specs *platform_specs;
	struct device *dev;
	struct mutex lock;

	struct regmap *regmap_ao;
	void __iomem *regs[MAX_REGS];
	struct clk *clks[MAX_CLKS];
	struct reset_control *resets[MAX_RESETS];
	int irqs[MAX_IRQS];

	struct v4l2_device v4l2_dev;
	struct video_device vfd;
	struct v4l2_m2m_dev *m2m_dev;

	wait_queue_head_t queue;

	struct meson_vcodec_session *current_session;
	atomic_t session_counter;
};

struct meson_vcodec_session {
	struct meson_vcodec *parent;
	int session_id;

	const struct meson_codec_formats *codec;

	struct v4l2_format input_format;
	struct v4l2_format output_format;

	struct v4l2_fh fh;
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_m2m_ctx *m2m_ctx;

	struct completion frame_completion;

	struct vb2_buffer *last_decoded_buffer;
	struct mutex lock;
};


int meson_vcodec_reset(struct meson_vcodec *vcodec, enum meson_vcodec_reset index);

void meson_vcodec_clk_unprepare(struct meson_vcodec *vcodec, enum meson_vcodec_clk index);

int meson_vcodec_clk_prepare(struct meson_vcodec *vcodec, enum meson_vcodec_clk index, u64 rate);

u32 meson_vcodec_reg_read(struct meson_vcodec *vcodec, enum meson_vcodec_regs index, u32 reg);

void meson_vcodec_reg_write(struct meson_vcodec *vcodec, enum meson_vcodec_regs index, u32 reg, u32 value);

int meson_vcodec_pwrc_off(struct meson_vcodec *vcodec, enum meson_vcodec_pwrc index);

int meson_vcodec_pwrc_on(struct meson_vcodec *vcodec, enum meson_vcodec_pwrc index);


#endif
