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
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>

#define DRIVER_NAME "meson-vcodec"
#define MAX_NUM_PLANES 4


/* BEGIN taken from meson-ee-pwrc.c */

/* AO Offsets */

#define GX_AO_RTI_GEN_PWR_SLEEP0	(0x3a << 2)
#define GX_AO_RTI_GEN_PWR_ISO0		(0x3b << 2)


struct meson_ee_pwrc_top_domain {
	unsigned int sleep_reg;
	unsigned int sleep_mask;
	unsigned int iso_reg;
	unsigned int iso_mask;
};

#define SM1_EE_PD(__bit)				\
{							\
	.sleep_reg = GX_AO_RTI_GEN_PWR_SLEEP0, 		\
	.sleep_mask = BIT(__bit), 			\
	.iso_reg = GX_AO_RTI_GEN_PWR_ISO0, 		\
	.iso_mask = BIT(__bit), 			\
}

/* END taken from meson-ee-pwrc.c */

#define GX_EE_PD(__sleep_mask, __iso_mask)		\
{							\
	.sleep_reg = GX_AO_RTI_GEN_PWR_SLEEP0, 		\
	.sleep_mask = __sleep_mask, 			\
	.iso_reg = GX_AO_RTI_GEN_PWR_ISO0, 		\
	.iso_mask = __iso_mask, 			\
}

enum meson_vcodec_pwrc {
	PWRC_VDEC1,
	PWRC_HEVC,
	PWRC_HCODEC,
	MAX_PWRC
};

static const struct meson_ee_pwrc_top_domain gx_pwrc[MAX_PWRC] = {
	[PWRC_VDEC1] = GX_EE_PD(
			BIT(2) | BIT(3),
			BIT(6) | BIT(7)),
	[PWRC_HEVC] = GX_EE_PD(
			BIT(6) | BIT(7),
			BIT(6) | BIT(7)),
	[PWRC_HCODEC] = GX_EE_PD(
			BIT(3),
			BIT(4) | BIT(5)),
};

static const struct meson_ee_pwrc_top_domain sm1_pwrc[MAX_PWRC] = {
	[PWRC_VDEC1] = SM1_EE_PD(1),
	[PWRC_HEVC] = SM1_EE_PD(2),
	[PWRC_HCODEC] = SM1_EE_PD(1),
};

enum meson_vcodec_regs {
	DOS_BASE,
	PARSER_BASE,
	MAX_REGS
};

static const char* reg_names[MAX_REGS] = {
	[DOS_BASE] = "dos",
	[PARSER_BASE] = "esparser",
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

static const char* clk_names[MAX_CLKS] = {
	[CLK_DOS] = "dos",
	[CLK_PARSER] = "dos_parser",
	[CLK_VDEC1] = "vdec_1",
	[CLK_HEVC] = "vdec_hevc",
	[CLK_HEVCF] = "vdec_hevcf",
	[CLK_HCODEC] = "hcodec",
};

enum meson_vcodec_reset {
	RESET_PARSER,
	RESET_HCODEC,
	MAX_RESETS
};

static const char* reset_names[MAX_RESETS] = {
	[RESET_PARSER] = "esparser",
	[RESET_HCODEC] = "hcodec",
};

enum meson_vcodec_irq {
	IRQ_VDEC,
	IRQ_PARSER,
	MAX_IRQS
};

static const char* irq_names[MAX_IRQS] = {
	[IRQ_VDEC] = "vdec",
	[IRQ_PARSER] = "esparser",
};

enum meson_platform_id {
	MESON_PLATFORM_ID_M8B = 0x1B,
	MESON_PLATFORM_ID_GXBB = 0x1F,
	MESON_PLATFORM_ID_GXTVBB,
	MESON_PLATFORM_ID_GXL,
	MESON_PLATFORM_ID_GXM,
	MESON_PLATFORM_ID_TXL,
	MESON_PLATFORM_ID_TXLX,
	MESON_PLATFORM_ID_AXG,
	MESON_PLATFORM_ID_GXLX,
	MESON_PLATFORM_ID_TXHD,
	MESON_PLATFORM_ID_G12A,
	MESON_PLATFORM_ID_G12B,
	MESON_PLATFORM_ID_SM1 = 0x2B,
	MESON_PLATFORM_ID_TL1 = 0x2E,
	MESON_PLATFORM_ID_TM2,
	MESON_PLATFORM_ID_C1,
	MESON_PLATFORM_ID_SC2 = 0x32,
	MESON_PLATFORM_ID_T5 = 0x34,
	MESON_PLATFORM_ID_T5D = 0x35,
	MESON_PLATFORM_ID_T7 = 0x36,
	MESON_PLATFORM_ID_S4 = 0x37,
	MESON_PLATFORM_ID_T3 = 0x38,
	MESON_PLATFORM_ID_S4D = 0x3a,
	MESON_PLATFORM_ID_T5W = 0x3b,
	MESON_PLATFORM_ID_S5 = 0x3e,
	MESON_PLATFORM_ID_UNKNOWN,
};

struct meson_vcodec_session;

struct meson_codec_ops {
	int (*init)(struct meson_vcodec_session *session);
	int (*cleanup)(struct meson_vcodec_session *session);
	int (*configure)(struct meson_vcodec_session *session);
	int (*start)(struct meson_vcodec_session *session);
	int (*stop)(struct meson_vcodec_session *session);
	int (*reset)(struct meson_vcodec_session *session);
	int (*process_frame)(struct meson_vcodec_session *session, struct vb2_buffer *src_buf, struct vb2_buffer *dst_buf);
	irqreturn_t (*isr)(int irq, void *dev_id);
};

struct meson_codec_spec {
	const struct meson_codec_ops *ops;
	const struct v4l2_ctrl_ops *ctrl_ops;
	const struct v4l2_ctrl_config *ctrls;
	const int num_ctrls;
};

struct meson_format {
	const u32 pixelformat;
	const char *description;
	const u32 num_planes;
	const u8 plane_size_denums[MAX_NUM_PLANES];
};

struct meson_codec_formats {
	const struct meson_format *input_format;
	const struct meson_format *output_format;
	const struct meson_format *intermediate_format;
	const struct meson_codec_spec *decoder;
	const struct meson_codec_spec *encoder;
};

struct meson_platform_specs {
	const enum meson_platform_id platform_id;
	const struct meson_ee_pwrc_top_domain *pwrc;
	const struct meson_codec_formats *formats;
	const u32 num_formats;
};

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
};

struct meson_vcodec_session {
	struct meson_vcodec *parent;
	int session_id;

	struct meson_codec_formats *codec;

	struct v4l2_format input_format;
	struct v4l2_format output_format;

	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_fh fh;

	struct vb2_queue in_q;
	struct vb2_queue out_q;

	struct completion frame_completion;

	struct vb2_buffer *last_decoded_buffer;
	spinlock_t buffer_lock;
};



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

static int meson_vcodec_probe(struct platform_device *pdev)
{
	struct meson_vcodec *vcodec;
	const struct meson_platform_specs *platform_specs;
	int ret, i;

	platform_specs = of_device_get_match_data(&pdev->dev);
	if (!platform_specs)
		return -ENODEV;

	vcodec = devm_kzalloc(&pdev->dev, sizeof(*vcodec), GFP_KERNEL);
	if (!vcodec)
		return -ENOMEM;

	vcodec->dev = &pdev->dev;
	vcodec->platform_specs = platform_specs;
	platform_set_drvdata(pdev, vcodec);

	mutex_init(&vcodec->lock);

	vcodec->regmap_ao = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "amlogic,ao-sysctrl");
	if (IS_ERR(vcodec->regmap_ao))
		return PTR_ERR(vcodec->regmap_ao);

	for (i = 0; i < MAX_REGS; i++) {
		vcodec->regs[i] = devm_platform_ioremap_resource_byname(pdev, reg_names[i]);
		if (IS_ERR(vcodec->regs[i]))
			return PTR_ERR(vcodec->regs[i]);
	}

	for (i = 0; i < MAX_CLKS; i++) {
		if (i == CLK_HEVCF && platform_specs->platform_id < MESON_PLATFORM_ID_G12A)
			continue;

		if (i == CLK_HCODEC && platform_specs->platform_id < MESON_PLATFORM_ID_SC2)
			continue;

		vcodec->clks[i] = devm_clk_get(&pdev->dev, clk_names[i]);
		if (IS_ERR(vcodec->clks[i])) {
			if (PTR_ERR(vcodec->clks[i]) != -ENOENT)
				return PTR_ERR(vcodec->clks[i]);
			vcodec->clks[i] = NULL;
		}
	}

	for (i = 0; i < MAX_RESETS; i++) {
		if (i == RESET_HCODEC && platform_specs->platform_id < MESON_PLATFORM_ID_SC2)
			continue;

		vcodec->resets[i] = devm_reset_control_get(&pdev->dev, reset_names[i]);
		if (IS_ERR(vcodec->resets[i])) {
			if (PTR_ERR(vcodec->resets[i]) != -ENOENT)
				return PTR_ERR(vcodec->resets[i]);
			vcodec->resets[i] = NULL;
		}
	}

	for (i = 0; i < MAX_IRQS; i++) {
		vcodec->irqs[i] = platform_get_irq_byname(pdev, irq_names[i]);
		if (vcodec->irqs[i] < 0)
			return vcodec->irqs[i];

		ret = devm_request_irq(&pdev->dev, vcodec->irqs[i], meson_vcodec_isr, IRQF_SHARED, dev_name(&pdev->dev), vcodec);
		if (ret) {
			dev_err(&pdev->dev, "Failed to install irq (%d)\n", ret);
			return ret;
		}
	}

	init_waitqueue_head(&vcodec->queue);

	ret = v4l2_device_register(&pdev->dev, &vcodec->v4l2_dev);
	if (ret)
		return ret;

	// Initialize encoders and decoders
	// Register V4L2 device
	// Set up V4L2 M2M device

	return 0;
}

static int meson_vcodec_remove(struct platform_device *pdev)
{
	struct meson_vcodec *vcodec = platform_get_drvdata(pdev);

	v4l2_device_unregister(&vcodec->v4l2_dev);
	// Clean up V4L2 M2M device
	// Free encoders and decoders

	return 0;
}

static const struct meson_format nv12 = {
	.description="NV12 raw pixel format",
	.pixelformat = V4L2_PIX_FMT_NV12,
	.num_planes = 2,
	.plane_size_denums = {1, 2},
};

static const struct meson_format yuv420m = {
	.description="YUV420M raw pixel format",
	.pixelformat = V4L2_PIX_FMT_YUV420M,
	.num_planes = 3,
	.plane_size_denums = {1, 2, 4},
};

static const struct meson_format h264 = {
	.description="H.264 AVC compressed format",
	.pixelformat = V4L2_PIX_FMT_H264,
	.num_planes = 1,
	.plane_size_denums = {1},
};

static const struct meson_format hevc = {
	.description="H.265 HEVC compressed format",
	.pixelformat = V4L2_PIX_FMT_HEVC,
	.num_planes = 1,
	.plane_size_denums = {1},
};


static const struct meson_codec_ops h264_decoder_ops = {
};

static const struct v4l2_ctrl_config h264_decoder_ctrls[] = {
	// H.264 decoder specific controls
};

static const struct v4l2_ctrl_ops h264_decoder_ctrl_ops = {
};

static const struct meson_codec_ops hevc_decoder_ops = {
};

static const struct v4l2_ctrl_config hevc_decoder_ctrls[] = {
	// HEVC decoder specific controls
};

static const struct v4l2_ctrl_ops hevc_decoder_ctrl_ops = {
};

static const struct meson_codec_ops h264_encoder_ops = {
};

static const struct v4l2_ctrl_config h264_encoder_ctrls[] = {
	// H.264 encoder specific controls
};

static const struct v4l2_ctrl_ops h264_encoder_ctrl_ops = {
};

static const struct meson_codec_ops hevc_encoder_ops = {
};

static const struct v4l2_ctrl_config hevc_encoder_ctrls[] = {
	// HEVC encoder specific controls
};

static const struct v4l2_ctrl_ops hevc_encoder_ctrl_ops = {
};

static const struct meson_codec_spec h264_decoder = {
	.ops = &h264_decoder_ops,
	.ctrl_ops = &h264_decoder_ctrl_ops,
	.ctrls = h264_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_decoder_ctrls),
};

static const struct meson_codec_spec hevc_decoder = {
	.ops = &hevc_decoder_ops,
	.ctrl_ops = &hevc_decoder_ctrl_ops,
	.ctrls = hevc_decoder_ctrls,
	.num_ctrls = ARRAY_SIZE(hevc_decoder_ctrls),
};

static const struct meson_codec_spec h264_encoder = {
	.ops = &h264_encoder_ops,
	.ctrl_ops = &h264_encoder_ctrl_ops,
	.ctrls = h264_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(h264_encoder_ctrls),
};

static const struct meson_codec_spec hevc_encoder = {
	.ops = &hevc_encoder_ops,
	.ctrl_ops = &hevc_encoder_ctrl_ops,
	.ctrls = hevc_encoder_ctrls,
	.num_ctrls = ARRAY_SIZE(hevc_encoder_ctrls),
};

static const struct meson_codec_formats gxl_formats[] = {

	// decoding combinations
	{
		.input_format = &h264,
		.output_format = &nv12,
		.intermediate_format = NULL,
		.decoder = &h264_decoder,
		.encoder = NULL,
	},
	{
		.input_format = &hevc,
		.output_format = &nv12,
		.intermediate_format = NULL,
		.decoder = &hevc_decoder,
		.encoder = NULL,
	},

	// encoding combinations
	{
		.input_format = &nv12,
		.output_format = &h264,
		.intermediate_format = NULL,
		.decoder = NULL,
		.encoder = &h264_encoder,
	},
	{
		.input_format = &yuv420m,
		.output_format = &h264,
		.intermediate_format = NULL,
		.decoder = NULL,
		.encoder = &h264_encoder,
	},
	{
		.input_format = &nv12,
		.output_format = &hevc,
		.intermediate_format = NULL,
		.decoder = NULL,
		.encoder = &hevc_encoder,
	},
	{
		.input_format = &yuv420m,
		.output_format = &hevc,
		.intermediate_format = NULL,
		.decoder = NULL,
		.encoder = &hevc_encoder,
	},

	// transcoding combinations
	{
		.input_format = &h264,
		.output_format = &h264,
		.intermediate_format = &nv12,
		.decoder = &h264_decoder,
		.encoder = &h264_encoder,
	},
	{
		.input_format = &h264,
		.output_format = &hevc,
		.intermediate_format = &nv12,
		.decoder = &h264_decoder,
		.encoder = &hevc_encoder,
	},
	{
		.input_format = &hevc,
		.output_format = &h264,
		.intermediate_format = &nv12,
		.decoder = &hevc_decoder,
		.encoder = &h264_encoder,
	},
	{
		.input_format = &hevc,
		.output_format = &hevc,
		.intermediate_format = &nv12,
		.decoder = &hevc_decoder,
		.encoder = &hevc_encoder,
	},
};

static const struct meson_platform_specs gxl_platform_specs = {
	.platform_id = MESON_PLATFORM_ID_GXL,
	.pwrc = gx_pwrc,
	.formats = gxl_formats,
	.num_formats = ARRAY_SIZE(gxl_formats),
};

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
