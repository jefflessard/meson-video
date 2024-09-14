#ifndef _MESON_VCODEC_H
#define _MESON_VCODEC_H

#include <linux/soc/amlogic/meson-canvas.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"

enum meson_vcodec_regs: u8 {
	DOS_BASE,
	PARSER_BASE,
	VENC_HEVC_BASE,
	MAX_REGS
};

enum meson_vcodec_clk: u8 {
	CLK_DOS,
	CLK_PARSER,
	CLK_VDEC1,
	CLK_HEVC,
	CLK_HEVCF,
	CLK_HCODEC,
	MAX_CLKS
};

enum meson_vcodec_reset: u8 {
	RESET_PARSER,
	RESET_HCODEC,
	MAX_RESETS
};

enum meson_vcodec_irq: u8 {
	IRQ_VDEC,
	IRQ_PARSER,
	IRQ_VENC_AVC,
	IRQ_VENC_HEVC,
	MAX_IRQS
};

enum meson_session_type: u8 {
	SESSION_TYPE_NONE,
	SESSION_TYPE_DECODE,
	SESSION_TYPE_ENCODE,
	SESSION_TYPE_TRANSCODE,
};

enum meson_stream_status: u8 {
	STREAM_STATUS_NONE,
	STREAM_STATUS_FMTSET,
	STREAM_STATUS_QSETUP,
	STREAM_STATUS_INIT,
	STREAM_STATUS_START,
	STREAM_STATUS_RUN,
	STREAM_STATUS_ABORT,
	STREAM_STATUS_STOP,
	STREAM_STATUS_RELEASE = STREAM_STATUS_NONE,
};

struct meson_vcodec_session;

struct meson_vcodec_core {
	const struct meson_platform_specs *platform_specs;
	struct device *dev;
	struct mutex lock;

	struct regmap *regmap_ao;
	void __iomem *regs[MAX_REGS];
	struct clk *clks[MAX_CLKS];
	struct reset_control *resets[MAX_RESETS];
	int irqs[MAX_IRQS];

	struct meson_canvas *canvas;

	struct v4l2_device v4l2_dev;
	struct video_device vfd;
	struct v4l2_m2m_dev *m2m_dev;

	struct meson_vcodec_session *active_session;
	atomic_t session_counter;
};

struct meson_vcodec_session {
	struct meson_vcodec_core *core;
	int session_id;

	struct v4l2_fh fh;
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_m2m_ctx *m2m_ctx;

	struct mutex lock;

	struct v4l2_format src_fmt;
	struct v4l2_format dst_fmt;
	struct v4l2_pix_format_mplane int_fmt;

	struct meson_codec_job enc_job;
	struct meson_codec_job dec_job;

	// status members
	enum meson_session_type type;
	enum meson_stream_status src_status;
	enum meson_stream_status dst_status;
};


int meson_vcodec_reset(struct meson_vcodec_core *core, enum meson_vcodec_reset index);

void meson_vcodec_clk_unprepare(struct meson_vcodec_core *core, enum meson_vcodec_clk index);

int meson_vcodec_clk_prepare(struct meson_vcodec_core *core, enum meson_vcodec_clk index, u64 rate);

u32 meson_vcodec_reg_read(struct meson_vcodec_core *core, enum meson_vcodec_regs index, u32 reg);

void meson_vcodec_reg_write(struct meson_vcodec_core *core, enum meson_vcodec_regs index, u32 reg, u32 value);

int meson_vcodec_pwrc_off(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index);

int meson_vcodec_pwrc_on(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index);


/* helper macros */
#define HAS_ARGS_(N, ...) N
#define HAS_ARGS(...) HAS_ARGS_(__VA_OPT__(1,) 0)
#define MACRO_FN_N__(FN, N, ...) FN##_##N(__VA_ARGS__)
#define MACRO_FN_N_(FN, N, ...) MACRO_FN_N__(FN, N, __VA_ARGS__)
#define MACRO_FN_N(FN, N, ...) MACRO_FN_N_(FN, N, __VA_ARGS__)

#define session_printk(__level, __session, __fmt, ...) \
	dev_printk(__level, __session->core->dev, \
			"Session %d type %d: " __fmt, \
			__session->session_id, __session->type, ##__VA_ARGS__)

#define session_err(__session, __fmt, ...) \
	session_printk(KERN_ERR, __session, __fmt, ##__VA_ARGS__)

#define session_warn(__session, __fmt, ...) \
	session_printk(KERN_WARNING, __session, __fmt, ##__VA_ARGS__)

#define session_info(__session, __fmt, ...) \
	session_printk(KERN_INFO, __session, __fmt, ##__VA_ARGS__)

#define session_dbg(__session, __fmt, ...) \
	dev_dbg( __session->core->dev, \
			"Session %d type %d: " __fmt, \
			__session->session_id, __session->type, ##__VA_ARGS__)

#define session_trace_0(__session) \
	session_dbg(__session, "%s", __func__)

#define session_trace_1(__session, __fmt, ...) \
	session_dbg(__session, "%s: " __fmt, __func__, ##__VA_ARGS__)

#define session_trace(__session, ...) \
	MACRO_FN_N(session_trace, HAS_ARGS(__VA_ARGS__), __session, ##__VA_ARGS__)

#define stream_printk(__level, __session, __type, __fmt, ...) \
	session_printk(__level, __session, \
			"Stream type %d, status %d: " __fmt, __type, \
			STREAM_STATUS(__session, __type), ##__VA_ARGS__)

#define stream_err(__session, __type, __fmt, ...) \
	stream_printk(KERN_ERR, __session, __type, __fmt, ##__VA_ARGS__)

#define stream_warn(__session, __type, __fmt, ...) \
	stream_printk(KERN_WARNING, __session, __type, __fmt, ##__VA_ARGS__)

#define stream_info(__session, __type, __fmt, ...) \
	stream_printk(KERN_INFO, __session, __type, __fmt, ##__VA_ARGS__)

#define stream_dbg(__session, __type, __fmt, ...) \
	session_dbg(__session, \
			"Stream type %d, status %d: " __fmt, __type, \
			STREAM_STATUS(__session, __type), ##__VA_ARGS__)

#define stream_trace_0(__session, __type) \
	stream_dbg(__session, __type, "%s", __func__)

#define stream_trace_1(__session, __type, __fmt, ...) \
	stream_dbg(__session, __type, "%s: " __fmt, __func__, ##__VA_ARGS__)

#define stream_trace(__session, __type, ...) \
	MACRO_FN_N(stream_trace, HAS_ARGS(__VA_ARGS__), __session, __type, ##__VA_ARGS__)

#define IS_SRC_STREAM(__type) (__type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)

#define SET_STREAM_STATUS(__session, __type, __status) { \
	if (IS_SRC_STREAM(__type)) \
		__session->src_status = __status; \
	else \
		__session->dst_status = __status; \
}

#define STREAM_STATUS(__session, __type) \
	(IS_SRC_STREAM(__type) ? \
	 __session->src_status : \
	 __session->dst_status)

#endif
