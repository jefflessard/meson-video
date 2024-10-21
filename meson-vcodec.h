#ifndef _MESON_VCODEC_H
#define _MESON_VCODEC_H

#include <linux/soc/amlogic/meson-canvas.h>

#include "meson-formats.h"
#include "meson-codecs.h"
#include "meson-platforms.h"

#define DRIVER_NAME "meson-vcodec"

// TODO remove this once vdec adapter is gone
enum meson_vcodec_regs: u8 {
	DOS_BASE,
	PARSER_BASE,
	WAVE420L_BASE,
	MAX_REGS
};

enum meson_vcodec_regmaps: u8 {
	BUS_DOS,
	BUS_PARSER,
	BUS_AO,
	BUS_HHI,
	BUS_WAVE420L,
	MAX_BUS
};

enum meson_vcodec_clk: u8 {
	CLK_DOS,
	CLK_PARSER,
	CLK_VDEC1,
	CLK_HEVC,
	CLK_HEVCF,
	CLK_HCODEC,
	CLK_WAVE420L,
	MAX_CLKS
};

enum meson_vcodec_reset: u8 {
	RESET_PARSER,
	RESET_HCODEC,
	MAX_RESETS
};

enum meson_vcodec_irq: u8 {
	IRQ_PARSER,
	IRQ_MBOX0,
	IRQ_MBOX1,
	IRQ_MBOX2,
	IRQ_WAVE420L,
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

	struct regmap *regmaps[MAX_BUS];
	void __iomem *regs[MAX_REGS];
	struct clk *clks[MAX_CLKS];
	struct reset_control *resets[MAX_RESETS];
	int irqs[MAX_IRQS];

	struct meson_canvas *canvas;

	struct meson_codec_dev codecs[MAX_CODECS];

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
	struct v4l2_format int_fmt;
	struct v4l2_fract timeperframe;

	struct meson_codec_job enc_job;
	struct meson_codec_job dec_job;

	// status members
	enum meson_session_type type;
	enum meson_stream_status src_status;
	enum meson_stream_status dst_status;
};

s32 meson_vcodec_g_ctrl(struct meson_vcodec_session *session, u32 id);
int meson_vcodec_s_ctrl(struct meson_vcodec_session *session, u32 id, s32 val);
void meson_vcodec_event_resolution(struct meson_vcodec_session *session);
void meson_vcodec_event_eos(struct meson_vcodec_session *session);

int meson_vcodec_reset(struct meson_vcodec_core *core, enum meson_vcodec_reset index);

void meson_vcodec_clk_unprepare(struct meson_vcodec_core *core, enum meson_vcodec_clk index);

int meson_vcodec_clk_prepare(struct meson_vcodec_core *core, enum meson_vcodec_clk index, u64 rate);

int meson_vcodec_pwrc_off(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index);

int meson_vcodec_pwrc_on(struct meson_vcodec_core *core, enum meson_vcodec_pwrc index);


/* helper macros */

#define V4L2_FMT_WIDTH(__fmt) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.width : (__fmt)->fmt.pix.width)

#define V4L2_FMT_SET_WIDTH(__fmt, __val) { \
	if (V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type)) (__fmt)->fmt.pix_mp.width = (__val); else (__fmt)->fmt.pix.width = (__val);}

#define V4L2_FMT_HEIGHT(__fmt) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.height : (__fmt)->fmt.pix.height)

#define V4L2_FMT_SET_HEIGHT(__fmt, __val) { \
	if (V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type)) (__fmt)->fmt.pix_mp.height = (__val); else (__fmt)->fmt.pix.height = (__val);}

#define V4L2_FMT_PIXFMT(__fmt) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.pixelformat : (__fmt)->fmt.pix.pixelformat)

#define V4L2_FMT_SET_PIXFMT(__fmt, __val) { \
	if (((__val) & (0xff << 16)) == ('M' << 16)) { \
		 (__fmt)->type = V4L2_TYPE_IS_OUTPUT((__fmt)->type) ? V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; \
	 	(__fmt)->fmt.pix_mp.pixelformat = (__val); \
	 } else { \
		 (__fmt)->type = V4L2_TYPE_IS_OUTPUT((__fmt)->type) ? V4L2_BUF_TYPE_VIDEO_OUTPUT : V4L2_BUF_TYPE_VIDEO_CAPTURE; \
		 (__fmt)->fmt.pix.pixelformat = (__val); \
	 }}

#define V4L2_FMT_FOURCC(__fmt) \
	((char*) (V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? &(__fmt)->fmt.pix_mp.pixelformat : &(__fmt)->fmt.pix.pixelformat))

#define V4L2_FMT_NUMPLANES(__fmt) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.num_planes : 1)

#define V4L2_FMT_SET_NUMPLANES(__fmt, __val) { \
	if (V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type)) (__fmt)->fmt.pix_mp.num_planes = (__val);}

#define V4L2_FMT_BYTESPERLINE(__fmt, __plane) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.plane_fmt[__plane].bytesperline : (__fmt)->fmt.pix.bytesperline)

#define V4L2_FMT_SET_BYTESPERLINE(__fmt, __plane, __val) { \
	if (V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type)) (__fmt)->fmt.pix_mp.plane_fmt[__plane].bytesperline = (__val); else (__fmt)->fmt.pix.bytesperline = (__val);}

#define V4L2_FMT_SIZEIMAGE(__fmt, __plane) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.plane_fmt[__plane].sizeimage : (__fmt)->fmt.pix.sizeimage)

#define V4L2_FMT_SET_SIZEIMAGE(__fmt, __plane, __val) { \
	if (V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type)) (__fmt)->fmt.pix_mp.plane_fmt[__plane].sizeimage = (__val); else (__fmt)->fmt.pix.sizeimage = (__val);}

#define V4L2_FMT_COLORSPACE(__fmt) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.colorspace : (__fmt)->fmt.pix.colorspace)

#define V4L2_FMT_YCBCR(__fmt) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.ycbcr_enc : (__fmt)->fmt.pix.ycbcr_enc)

#define V4L2_FMT_QUANT(__fmt) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.quantization : (__fmt)->fmt.pix.quantization)

#define V4L2_FMT_XFER(__fmt) \
	(V4L2_TYPE_IS_MULTIPLANAR((__fmt)->type) ? (__fmt)->fmt.pix_mp.xfer_func : (__fmt)->fmt.pix.xfer_func)

#define IS_SRC_STREAM(__type) ( \
		__type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE || \
		__type == V4L2_BUF_TYPE_VIDEO_OUTPUT)

#define SET_STREAM_STATUS(__session, __type, __status) { \
	if (IS_SRC_STREAM(__type)) \
	(__session)->src_status = __status; \
	else \
	(__session)->dst_status = __status; \
}

#define STREAM_STATUS(__session, __type) \
	(IS_SRC_STREAM(__type) ? \
	 (__session)->src_status : \
	 (__session)->dst_status)


/* printk macros */

#define HAS_ARGS_(N, ...) N
#define HAS_ARGS(...) HAS_ARGS_(__VA_OPT__(1,) 0)
#define MACRO_FN_N__(FN, N, ...) FN##_##N(__VA_ARGS__)
#define MACRO_FN_N_(FN, N, ...) MACRO_FN_N__(FN, N, __VA_ARGS__)
#define MACRO_FN_N(FN, N, ...) MACRO_FN_N_(FN, N, __VA_ARGS__)

#define session_printk(__level, __session, __fmt, ...) \
	printk(__level DRIVER_NAME ": Session %d: " __fmt, \
			(__session)->session_id, ##__VA_ARGS__)

#define session_err(__session, __fmt, ...) \
	session_printk(KERN_ERR, __session, __fmt, ##__VA_ARGS__)

#define session_warn(__session, __fmt, ...) \
	session_printk(KERN_WARNING, __session, __fmt, ##__VA_ARGS__)

#define session_info(__session, __fmt, ...) \
	session_printk(KERN_INFO, __session, __fmt, ##__VA_ARGS__)

#define session_dbg(__session, __fmt, ...) \
	pr_debug(DRIVER_NAME ": Session %d: " __fmt, \
			(__session)->session_id, ##__VA_ARGS__)

#define session_trace_0(__session) \
	session_dbg(__session, "%s", __func__)

#define session_trace_1(__session, __fmt, ...) \
	session_dbg(__session, "%s: " __fmt, __func__, ##__VA_ARGS__)

#define session_trace(__session, ...) \
	MACRO_FN_N(session_trace, HAS_ARGS(__VA_ARGS__), __session, ##__VA_ARGS__)

#define stream_printk(__level, __session, __type, __fmt, ...) \
	session_printk(__level, __session, \
			"Stream type %s (%d), status %d: " __fmt, \
			IS_SRC_STREAM(__type) ? "src" : "dst", __type, \
			STREAM_STATUS(__session, __type), ##__VA_ARGS__)

#define stream_err(__session, __type, __fmt, ...) \
	stream_printk(KERN_ERR, __session, __type, __fmt, ##__VA_ARGS__)

#define stream_warn(__session, __type, __fmt, ...) \
	stream_printk(KERN_WARNING, __session, __type, __fmt, ##__VA_ARGS__)

#define stream_info(__session, __type, __fmt, ...) \
	stream_printk(KERN_INFO, __session, __type, __fmt, ##__VA_ARGS__)

#define stream_dbg(__session, __type, __fmt, ...) \
	session_dbg(__session, \
			"Stream type %s (%d), status %d: " __fmt, \
			IS_SRC_STREAM(__type) ? "src" : "dst", __type, \
			STREAM_STATUS(__session, __type), ##__VA_ARGS__)

#define stream_trace_0(__session, __type) \
	stream_dbg(__session, __type, "%s", __func__)

#define stream_trace_1(__session, __type, __fmt, ...) \
	stream_dbg(__session, __type, "%s: " __fmt, __func__, ##__VA_ARGS__)

#define stream_trace(__session, __type, ...) \
	MACRO_FN_N(stream_trace, HAS_ARGS(__VA_ARGS__), __session, __type, ##__VA_ARGS__)

#define job_printk(__level, __job, __fmt, ...) \
	session_printk(__level, (__job)->session, \
			"%s: " __fmt, (__job)->codec->spec->name, ##__VA_ARGS__)

#define job_err(__job, __fmt, ...) \
	job_printk(KERN_ERR, __job, __fmt, ##__VA_ARGS__)

#define job_warn(__job, __fmt, ...) \
	job_printk(KERN_WARNING, __job, __fmt, ##__VA_ARGS__)

#define job_info(__job, __fmt, ...) \
	job_printk(KERN_INFO, __job, __fmt, ##__VA_ARGS__)

#define job_dbg(__job, __fmt, ...) \
	session_dbg((__job)->session, \
		"%s: " __fmt, (__job)->codec->spec->name, ##__VA_ARGS__)

#define job_trace_0(__job) \
	job_dbg(__job, "%s", __func__)

#define job_trace_1(__job, __fmt, ...) \
	job_dbg(__job, "%s: " __fmt, __func__, ##__VA_ARGS__)

#define job_trace(__job, ...) \
	MACRO_FN_N(job_trace, HAS_ARGS(__VA_ARGS__), __job, ##__VA_ARGS__)

#define codec_printk(__level, __codec, __fmt, ...) \
	dev_printk(__level, (__codec)->core->dev, \
			"%s: " __fmt, (__codec)->spec->name, ##__VA_ARGS__)

#define codec_err(__codec, __fmt, ...) \
	codec_printk(KERN_ERR, __codec, __fmt, ##__VA_ARGS__)

#define codec_warn(__codec, __fmt, ...) \
	codec_printk(KERN_WARNING, __codec, __fmt, ##__VA_ARGS__)

#define codec_info(__codec, __fmt, ...) \
	codec_printk(KERN_INFO, __codec, __fmt, ##__VA_ARGS__)

#define codec_dbg(__codec, __fmt, ...) \
	dev_dbg((__codec)->core->dev, \
		"%s: " __fmt, (__codec)->spec->name, ##__VA_ARGS__)

#define codec_trace_0(__codec) \
	codec_dbg(__codec, "%s", __func__)

#define codec_trace_1(__codec, __fmt, ...) \
	codec_dbg(__codec, "%s: " __fmt, __func__, ##__VA_ARGS__)

#define codec_trace(__codec, ...) \
	MACRO_FN_N(codec_trace, HAS_ARGS(__VA_ARGS__), __codec, ##__VA_ARGS__)

#endif
