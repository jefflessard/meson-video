#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <linux/cma.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/soc/amlogic/meson-canvas.h>

#define DRIVER_NAME "meson-h264-encoder"
#define MIN_SIZE amvenc_buffspec[0].min_buffsize

#define HCODEC_BASE = 0xff; //To be replaced with the device tree node register
#define WRITE_HREG(reg,val) writel((val), (HCODEC_BASE + reg))
#define READ_HREG(reg) readl(ctx->HCODEC_BASE + (reg))

struct encode_request_s {
    u32 cmd;
    u32 ucode_mode;
    u32 quant;
    u32 flush_flag;
    u32 timeout;
    u32 src;
    u32 framesize;
    u32 fmt;
    u32 type;
};

struct encode_wq_s {
    struct list_head list;
    u32 hw_status;
    u32 output_size;
    struct {
        u32 buf_start;
        u32 buf_size;
    } mem;
    struct {
        u32 encoder_width;
        u32 encoder_height;
        u32 idr_pic_id;
        u32 frame_number;
        u32 pic_order_cnt_lsb;
        u32 log2_max_pic_order_cnt_lsb;
        u32 log2_max_frame_num;
        u32 init_qppicture;
    } pic;
    u32 ucode_index;
    u32 sps_size;
    u32 pps_size;
    u32 me_weight;
    u32 i4_weight;
    u32 i16_weight;
};

struct aml_vcodec_ctx {
    struct v4l2_fh fh;
    struct v4l2_ctrl_handler ctrl_handler;
    struct v4l2_pix_format_mplane src_fmt;
    struct v4l2_pix_format_mplane dst_fmt;
    struct vb2_queue src_queue;
    struct vb2_queue dst_queue;
    struct device *dev;
    struct mutex lock;
    enum v4l2_colorspace colorspace;
    bool streaming;
    struct encode_wq_s *wq;
    struct encode_request_s request;
    struct meson_canvas *canvas;
    u8 canvas_index;
};

struct aml_vcodec_dev {
    struct v4l2_device v4l2_dev;
    struct video_device *vfd;
    struct device *dev;
    struct v4l2_m2m_dev *m2m_dev;
    void __iomem *enc_base;
    struct meson_canvas *canvas;
};

static void avc_canvas_init(struct encode_wq_s *wq)
{
    u32 canvas_width, canvas_height;
    u32 start_addr = wq->mem.buf_start;
    struct aml_vcodec_ctx *ctx = container_of(wq, struct aml_vcodec_ctx, wq);

    canvas_width = ((wq->pic.encoder_width + 31) >> 5) << 5;
    canvas_height = ((wq->pic.encoder_height + 15) >> 4) << 4;

    meson_canvas_config(ctx->canvas, ctx->canvas_index,
        start_addr + wq->mem.buf_start,
        canvas_width, canvas_height,
        MESON_CANVAS_WRAP_NONE, MESON_CANVAS_BLKMODE_LINEAR, 0);
    meson_canvas_config(ctx->canvas, ctx->canvas_index + 1,
        start_addr + wq->mem.buf_start + canvas_width * canvas_height,
        canvas_width, canvas_height / 2,
        MESON_CANVAS_WRAP_NONE, MESON_CANVAS_BLKMODE_LINEAR, 0);
}

static void avc_init_encoder(struct encode_wq_s *wq, bool idr)
{
    struct aml_vcodec_ctx *ctx = container_of(wq, struct aml_vcodec_ctx, wq);
    
    WRITE_HREG(HCODEC_VLC_TOTAL_BYTES, 0);
    WRITE_HREG(HCODEC_VLC_CONFIG, 0x07);
    WRITE_HREG(HCODEC_VLC_INT_CONTROL, 0);

    WRITE_HREG(HCODEC_ASSIST_AMR1_INT0, 0x15);
    WRITE_HREG(HCODEC_ASSIST_AMR1_INT1, 0x8);
    WRITE_HREG(HCODEC_ASSIST_AMR1_INT3, 0x14);

    WRITE_HREG(IDR_PIC_ID, wq->pic.idr_pic_id);
    WRITE_HREG(FRAME_NUMBER,
        (idr == true) ? 0 : wq->pic.frame_number);
    WRITE_HREG(PIC_ORDER_CNT_LSB,
        (idr == true) ? 0 : wq->pic.pic_order_cnt_lsb);

    WRITE_HREG(LOG2_MAX_PIC_ORDER_CNT_LSB,
        wq->pic.log2_max_pic_order_cnt_lsb);
    WRITE_HREG(LOG2_MAX_FRAME_NUM,
        wq->pic.log2_max_frame_num);
    WRITE_HREG(ANC0_BUFFER_ID, 0);
    WRITE_HREG(QPPICTURE, wq->pic.init_qppicture);

    avc_init_encoder_ie(wq, ctx->request.quant);

    WRITE_HREG(ENCODER_STATUS, 0);
    WRITE_HREG(HCODEC_ASSIST_MMC_CTRL1, 0x32);

    WRITE_HREG(QDCT_MB_CONTROL, 
        (1 << 9) | /* mb_info_soft_reset */
        (1 << 0)); /* mb read buffer soft reset */

    WRITE_HREG(QDCT_MB_CONTROL,
        (1 << 28) | /* ignore_t_p8x8 */
        (0 << 27) | /* zero_mc_out_null_non_skipped_mb */
        (0 << 26) | /* no_mc_out_null_non_skipped_mb */
        (0 << 25) | /* mc_out_even_skipped_mb */
        (0 << 24) | /* mc_out_wait_cbp_ready */
        (0 << 23) | /* mc_out_wait_mb_type_ready */
        (1 << 29) | /* ie_start_int_enable */
        (1 << 19) | /* i_pred_enable */
        (1 << 20) | /* ie_sub_enable */
        (1 << 18) | /* iq_enable */
        (1 << 17) | /* idct_enable */
        (1 << 14) | /* mb_pause_enable */
        (1 << 13) | /* q_enable */
        (1 << 12) | /* dct_enable */
        (1 << 10) | /* mb_info_en */
        (0 << 3) | /* endian */
        (0 << 1) | /* mb_read_en */
        (0 << 0)); /* soft reset */

    WRITE_HREG(HCODEC_CURR_CANVAS_CTRL, 0);
    WRITE_HREG(HCODEC_VLC_CONFIG, READ_HREG(HCODEC_VLC_CONFIG) | (1 << 0)); // set pop_coeff_even_all_zero

    WRITE_HREG(HCODEC_IGNORE_CONFIG,
        (1 << 31) | /* ignore_lac_coeff_en */
        (1 << 26) | /* ignore_lac_coeff_else (<1) */
        (1 << 21) | /* ignore_lac_coeff_2 (<1) */
        (2 << 16) | /* ignore_lac_coeff_1 (<2) */
        (1 << 15) | /* ignore_cac_coeff_en */
        (1 << 10) | /* ignore_cac_coeff_else (<1) */
        (1 << 5)  | /* ignore_cac_coeff_2 (<1) */
        (3 << 0));  /* ignore_cac_coeff_1 (<2) */

    if (get_cpu_type() >= MESON_CPU_MAJOR_ID_GXTVBB)
        WRITE_HREG(HCODEC_IGNORE_CONFIG_2,
            (1 << 31) | /* ignore_t_lac_coeff_en */
            (1 << 26) | /* ignore_t_lac_coeff_else (<1) */
            (2 << 21) | /* ignore_t_lac_coeff_2 (<2) */
            (6 << 16) | /* ignore_t_lac_coeff_1 (<6) */
            (1 << 15) | /* ignore_cdc_coeff_en */
            (0 << 14) | /* ignore_t_lac_coeff_else_le_3 */
            (1 << 13) | /* ignore_t_lac_coeff_else_le_4 */
            (1 << 12) | /* ignore_cdc_only_when_empty_cac_inter */
            (1 << 11) | /* ignore_cdc_only_when_one_empty_inter */
            (2 << 9) |  /* ignore_cdc_range_max_inter 0-0, 1-1, 2-2, 3-3 */
            (0 << 7) |  /* ignore_cdc_abs_max_inter 0-1, 1-2, 2-3, 3-4 */
            (1 << 5) |  /* ignore_cdc_only_when_empty_cac_intra */
            (1 << 4) |  /* ignore_cdc_only_when_one_empty_intra */
            (1 << 2) |  /* ignore_cdc_range_max_intra 0-0, 1-1, 2-2, 3-3 */
            (0 << 0));  /* ignore_cdc_abs_max_intra 0-1, 1-2, 2-3, 3-4 */
    else
        WRITE_HREG(HCODEC_IGNORE_CONFIG_2,
            (1 << 31) | /* ignore_t_lac_coeff_en */
            (1 << 26) | /* ignore_t_lac_coeff_else (<1) */
            (1 << 21) | /* ignore_t_lac_coeff_2 (<1) */
            (5 << 16) | /* ignore_t_lac_coeff_1 (<5) */
            (0 << 0));

    WRITE_HREG(HCODEC_QDCT_MB_CONTROL, 1 << 8); // mb_info_state_reset
}

static void avc_init_encoder_ie(struct encode_wq_s *wq, u32 quant)
{
    WRITE_HREG(HCODEC_IE_CONTROL,
        (1 << 30) | /* active_ul_block */
        (0 << 1) | /* ie_enable */
        (1 << 0)); /* ie soft reset */

    WRITE_HREG(HCODEC_IE_CONTROL,
        (1 << 30) | /* active_ul_block */
        (0 << 1) | /* ie_enable */
        (0 << 0)); /* ie soft reset */

    WRITE_HREG(HCODEC_IE_WEIGHT,
        (wq->i16_weight << 16) |
        (wq->i4_weight << 0));
    WRITE_HREG(HCODEC_ME_WEIGHT,
        (wq->me_weight << 0));
    WRITE_HREG(HCODEC_SAD_CONTROL_0,
        (wq->i16_weight << 16) |
        (wq->i4_weight << 0));
    WRITE_HREG(HCODEC_SAD_CONTROL_1,
        (IE_SAD_SHIFT_I16 << 24) |
        (IE_SAD_SHIFT_I4 << 20) |
        (ME_SAD_SHIFT_INTER << 16) |
        (wq->me_weight << 0));

    WRITE_HREG(HCODEC_IE_DATA_FEED_BUFF_INFO, 0);

    WRITE_HREG(HCODEC_QDCT_Q_QUANT_I,
        (quant << 22) |
        (quant << 16) |
        ((quant % 6) << 12) |
        ((quant / 6) << 8) |
        ((quant % 6) << 4) |
        ((quant / 6) << 0));

    WRITE_HREG(HCODEC_QDCT_Q_QUANT_P,
        (quant << 22) |
        (quant << 16) |
        ((quant % 6) << 12) |
        ((quant / 6) << 8) |
        ((quant % 6) << 4) |
        ((quant / 6) << 0));

    WRITE_HREG(HCODEC_SAD_CONTROL_0,
        (wq->i16_weight << 16) |
        (wq->i4_weight << 0));
    WRITE_HREG(HCODEC_SAD_CONTROL_1,
        (IE_SAD_SHIFT_I16 << 24) |
        (IE_SAD_SHIFT_I4 << 20) |
        (ME_SAD_SHIFT_INTER << 16) |
        (wq->me_weight << 0));

    WRITE_HREG(HCODEC_ADV_MV_CTL0,
        (ADV_MV_LARGE_16x8 << 31) |
        (ADV_MV_LARGE_8x16 << 30) |
        (ADV_MV_8x8_WEIGHT << 16) |
        (ADV_MV_4x4x4_WEIGHT << 0));
    WRITE_HREG(HCODEC_ADV_MV_CTL1,
        (ADV_MV_16x16_WEIGHT << 16) |
        (ADV_MV_LARGE_16x16 << 15) |
        (ADV_MV_16_8_WEIGHT << 0));
}

static int aml_vcodec_queue_setup(struct vb2_queue *vq,
                                  unsigned int *nbuffers,
                                  unsigned int *nplanes,
                                  unsigned int sizes[],
                                  struct device *alloc_devs[])
{
    struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vq);
    unsigned int size;

    if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        size = ctx->src_fmt.plane_fmt[0].sizeimage;
    } else if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        size = ctx->dst_fmt.plane_fmt[0].sizeimage;
    } else {
        return -EINVAL;
    }

    if (*nplanes) {
        if (*nplanes != 1 || sizes[0] < size)
            return -EINVAL;
        size = sizes[0];
    } else {
        *nplanes = 1;
        sizes[0] = size;
    }

    return 0;
}

static int aml_vcodec_buf_init(struct vb2_buffer *vb)
{
    struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
    struct v4l2_m2m_buffer *mb = container_of(vbuf, struct v4l2_m2m_buffer, vb);
    struct aml_video_buf *buf = container_of(mb, struct aml_video_buf, m2m_buf);

    buf->size = vb2_plane_size(vb, 0);
    buf->addr = vb2_dma_contig_plane_dma_addr(vb, 0);

    if (vb->vb2_queue->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        // Configure canvas for output buffer
        return meson_canvas_config(ctx->canvas, ctx->canvas_index,
                                   buf->addr, vb->planes[0].bytesused,
                                   ctx->dst_fmt.height,
                                   MESON_CANVAS_WRAP_NONE,
                                   MESON_CANVAS_BLKMODE_LINEAR, 0);
    }

    return 0;
}


static int aml_vcodec_buf_prepare(struct vb2_buffer *vb)
{
    struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

    if (vb->vb2_queue->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE &&
        vb2_get_plane_payload(vb, 0) == 0) {
        vb2_set_plane_payload(vb, 0, ctx->src_fmt.plane_fmt[0].sizeimage);
    }

    return 0;
}

static void aml_vcodec_buf_queue(struct vb2_buffer *vb)
{
    struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

    v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
}

static int aml_vcodec_start_streaming(struct vb2_queue *q, unsigned int count)
{
    struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(q);

    if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        ctx->wq->pic.init_qppicture = 26;  // Default QP, can be made configurable
        avc_init_encoder(ctx->wq, true);
    }

    return 0;
}

static void aml_vcodec_stop_streaming(struct vb2_queue *q)
{
    struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(q);

    if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        amvenc_avc_stop();
    }
}

static const struct vb2_ops aml_vcodec_vb2_ops = {
    .queue_setup     = aml_vcodec_queue_setup,
    .buf_init        = aml_vcodec_buf_init,
    .buf_prepare     = aml_vcodec_buf_prepare,
    .buf_queue       = aml_vcodec_buf_queue,
    .start_streaming = aml_vcodec_start_streaming,
    .stop_streaming  = aml_vcodec_stop_streaming,
};

static int aml_vcodec_job_ready(void *priv)
{
    struct aml_vcodec_ctx *ctx = priv;
    
    if (v4l2_m2m_num_src_bufs_ready(ctx->fh.m2m_ctx) > 0 &&
        v4l2_m2m_num_dst_bufs_ready(ctx->fh.m2m_ctx) > 0)
        return 1;
    
    return 0;
}

static void aml_vcodec_job_abort(void *priv)
{
    struct aml_vcodec_ctx *ctx = priv;
    
    // Stop encoding process
    amvenc_avc_stop();
    
    // Clear any pending buffers
    v4l2_m2m_job_finish(ctx->dev->m2m_dev, ctx->fh.m2m_ctx);
}

static int aml_vcodec_queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
    struct aml_vcodec_ctx *ctx = priv;
    int ret;

    src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
    src_vq->drv_priv = ctx;
    src_vq->buf_struct_size = sizeof(struct aml_video_buf);
    src_vq->ops = &aml_vcodec_vb2_ops;
    src_vq->mem_ops = &vb2_dma_contig_memops;
    src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
    src_vq->lock = &ctx->dev->dev_mutex;
    src_vq->dev = ctx->dev->v4l2_dev.dev;

    ret = vb2_queue_init(src_vq);
    if (ret)
        return ret;

    dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
    dst_vq->drv_priv = ctx;
    dst_vq->buf_struct_size = sizeof(struct aml_video_buf);
    dst_vq->ops = &aml_vcodec_vb2_ops;
    dst_vq->mem_ops = &vb2_dma_contig_memops;
    dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
    dst_vq->lock = &ctx->dev->dev_mutex;
    dst_vq->dev = ctx->dev->v4l2_dev.dev;

    return vb2_queue_init(dst_vq);
}

static int aml_vcodec_device_run(void *priv)
{
    struct aml_vcodec_ctx *ctx = priv;
    struct vb2_v4l2_buffer *src_buf, *dst_buf;
    struct aml_video_buf *src_vbuf, *dst_vbuf;
    struct encode_request_s *request = &ctx->request;

    src_buf = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
    dst_buf = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
    src_vbuf = container_of(src_buf, struct aml_video_buf, vb);
    dst_vbuf = container_of(dst_buf, struct aml_video_buf, vb);

    request->src = src_vbuf->addr;
    request->framesize = vb2_get_plane_payload(&src_buf->vb2_buf, 0);
    request->fmt = ctx->src_fmt.pixelformat;
    request->type = DMA_BUFF;
    request->cmd = ENCODER_NON_IDR;
    request->timeout = 200;  // Timeout in ms, adjust as needed

    encode_wq_add_request(ctx->wq);

    // Wait for encoding to complete
    wait_event_interruptible_timeout(ctx->dev->encode_manager.event.hw_complete,
        (ctx->dev->encode_manager.encode_hw_status == ENCODER_IDR_DONE) ||
        (ctx->dev->encode_manager.encode_hw_status == ENCODER_NON_IDR_DONE),
        msecs_to_jiffies(request->timeout));

    if (ctx->dev->encode_manager.encode_hw_status != ENCODER_IDR_DONE &&
        ctx->dev->encode_manager.encode_hw_status != ENCODER_NON_IDR_DONE) {
        v4l2_err(&ctx->dev->v4l2_dev, "Encoding timeout or error\n");
        return -EIO;
    }

    // Copy encoded data to dst buffer
    memcpy(vb2_plane_vaddr(&dst_buf->vb2_buf, 0),
           phys_to_virt(ctx->wq->mem.BitstreamStart),
           ctx->wq->output_size);

    vb2_set_plane_payload(&dst_buf->vb2_buf, 0, ctx->wq->output_size);

    v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_DONE);
    v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_DONE);

    v4l2_m2m_job_finish(ctx->dev->m2m_dev, ctx->fh.m2m_ctx);

    return 0;
}

static int encode_wq_add_request(struct encode_wq_s *wq)
{
    struct aml_vcodec_ctx *ctx = container_of(wq, struct aml_vcodec_ctx, wq);
    struct encode_queue_item_s *pitem;

    pitem = kzalloc(sizeof(*pitem), GFP_KERNEL);
    if (!pitem)
        return -ENOMEM;

    memcpy(&pitem->request, &ctx->request, sizeof(struct encode_request_s));

    spin_lock(&ctx->dev->encode_manager.event.sem_lock);
    list_add_tail(&pitem->list, &ctx->dev->encode_manager.process_queue);
    spin_unlock(&ctx->dev->encode_manager.event.sem_lock);

    complete(&ctx->dev->encode_manager.event.request_in_com);

    return 0;
}

static void avc_init_input_buffer(void *info)
{
    struct encode_wq_s *wq = info;
    
    WRITE_HREG(HCODEC_QDCT_MB_START_PTR, wq->mem.dct_buff_start_addr);
    WRITE_HREG(HCODEC_QDCT_MB_END_PTR, wq->mem.dct_buff_end_addr);
    WRITE_HREG(HCODEC_QDCT_MB_WR_PTR, wq->mem.dct_buff_start_addr);
    WRITE_HREG(HCODEC_QDCT_MB_RD_PTR, wq->mem.dct_buff_start_addr);
    WRITE_HREG(HCODEC_QDCT_MB_BUFF, 0);
}

static void avc_init_output_buffer(void *info)
{
    struct encode_wq_s *wq = info;
    
    WRITE_HREG(HCODEC_VLC_VB_MEM_CTL, 
        (1 << 31) | (0x3f << 24) | (0x20 << 16) | (2 << 0));
    WRITE_HREG(HCODEC_VLC_VB_START_PTR, wq->mem.BitstreamStart);
    WRITE_HREG(HCODEC_VLC_VB_WR_PTR, wq->mem.BitstreamStart);
    WRITE_HREG(HCODEC_VLC_VB_SW_RD_PTR, wq->mem.BitstreamStart);
    WRITE_HREG(HCODEC_VLC_VB_END_PTR, wq->mem.BitstreamEnd);
    WRITE_HREG(HCODEC_VLC_VB_CONTROL, 1);
    WRITE_HREG(HCODEC_VLC_VB_CONTROL, 
        (0 << 14) | (7 << 3) | (1 << 1) | (0 << 0));
}

static irqreturn_t enc_isr(int irq_number, void *para)
{
    struct aml_vcodec_dev *dev = para;
    
    WRITE_HREG(HCODEC_IRQ_MBOX_CLR, 1);
    
    dev->encode_manager.encode_hw_status = READ_HREG(ENCODER_STATUS);
    
    if (dev->encode_manager.encode_hw_status == ENCODER_IDR_DONE ||
        dev->encode_manager.encode_hw_status == ENCODER_NON_IDR_DONE ||
        dev->encode_manager.encode_hw_status == ENCODER_SEQUENCE_DONE ||
        dev->encode_manager.encode_hw_status == ENCODER_PICTURE_DONE) {
        dev->encode_manager.process_irq = true;
        wake_up_interruptible(&dev->encode_manager.event.hw_complete);
    }
    
    return IRQ_HANDLED;
}

static void amvenc_avc_stop(void)
{
    ulong timeout = jiffies + HZ;

    WRITE_HREG(HCODEC_MPSR, 0);
    WRITE_HREG(HCODEC_CPSR, 0);
    
    while (READ_HREG(HCODEC_IMEM_DMA_CTRL) & 0x8000) {
        if (time_after(jiffies, timeout))
            break;
    }
    
    WRITE_VREG(DOS_SW_RESET1, 
        (1 << 12) | (1 << 11) | (1 << 2) | (1 << 6) | 
        (1 << 7) | (1 << 8) | (1 << 14) | (1 << 16) | (1 << 17));
    WRITE_VREG(DOS_SW_RESET1, 0);
}

static const char *select_ucode(u32 ucode_index)
{
#ifdef IGNORE_THIS_CODE
    switch (ucode_index) {
    case UCODE_MODE_FULL:
        if (get_cpu_type() >= MESON_CPU_MAJOR_ID_G12A)
            return "ga_h264_enc_cabac";
        else if (get_cpu_type() >= MESON_CPU_MAJOR_ID_TXL)
            return "txl_h264_enc_cavlc";
        else
            return "gxl_h264_enc";
    default:
        return "gxl_h264_enc";
    }
#else
    return "gxl_h264_enc";
#endif
}

static s32 amvenc_loadmc(const char *p, struct encode_wq_s *wq)
{
    ulong timeout;
    s32 ret = 0;

    if (!wq->mem.assit_buffer_offset) {
        pr_err("amvenc_loadmc: assit_buffer is not allocated\n");
        return -ENOMEM;
    }

    WRITE_HREG(HCODEC_MPSR, 0);
    WRITE_HREG(HCODEC_CPSR, 0);

    timeout = READ_HREG(HCODEC_MPSR);
    timeout = READ_HREG(HCODEC_MPSR);

    timeout = jiffies + HZ;

    WRITE_HREG(HCODEC_IMEM_DMA_ADR, wq->mem.assit_buffer_offset);
    WRITE_HREG(HCODEC_IMEM_DMA_COUNT, 0x1000);
    WRITE_HREG(HCODEC_IMEM_DMA_CTRL, (0x8000 | (7 << 16)));

    while (READ_HREG(HCODEC_IMEM_DMA_CTRL) & 0x8000) {
        if (time_before(jiffies, timeout))
            schedule();
        else {
            pr_err("hcodec load mc error\n");
            ret = -EBUSY;
            break;
        }
    }

    return ret;
}

static int avc_init(struct aml_vcodec_dev *dev)
{
    int ret;

    // Initialize encode_manager
    INIT_LIST_HEAD(&dev->encode_manager.wq);
    INIT_LIST_HEAD(&dev->encode_manager.process_queue);
    INIT_LIST_HEAD(&dev->encode_manager.free_queue);
    spin_lock_init(&dev->encode_manager.event.sem_lock);
    init_waitqueue_head(&dev->encode_manager.event.hw_complete);

    // Initialize hardware
    avc_poweron(clock_level);
    ret = amvenc_loadmc(select_ucode(dev->encode_manager.ucode_index), NULL);
    if (ret < 0) {
        dev_err(dev->dev, "Failed to load microcode\n");
        return ret;
    }

    // Initialize buffers
    ret = avc_init_input_buffer(NULL);
    if (ret < 0) {
        dev_err(dev->dev, "Failed to initialize input buffer\n");
        return ret;
    }

    ret = avc_init_output_buffer(NULL);
    if (ret < 0) {
        dev_err(dev->dev, "Failed to initialize output buffer\n");
        return ret;
    }

    // Request IRQ
    ret = request_irq(dev->encode_manager.irq_num, enc_isr, IRQF_SHARED,
                      "enc-irq", &dev->encode_manager);
    if (ret) {
        dev_err(dev->dev, "Failed to request IRQ\n");
        return ret;
    }

    dev->encode_manager.irq_requested = true;

    return 0;
}

static void avc_uninit(struct aml_vcodec_dev *dev)
{
    if (dev->encode_manager.irq_requested) {
        free_irq(dev->encode_manager.irq_num, &dev->encode_manager);
        dev->encode_manager.irq_requested = false;
    }

    avc_poweroff();
}

static int aml_vcodec_cma_init(struct aml_vcodec_ctx *ctx)
{
    ctx->wq->mem.buf_start = dma_alloc_coherent(ctx->dev, MIN_SIZE,
                                                &ctx->wq->mem.buf_start_phys,
                                                GFP_KERNEL);
    if (!ctx->wq->mem.buf_start) {
        dev_err(ctx->dev, "Failed to allocate CMA memory\n");
        return -ENOMEM;
    }

    ctx->wq->mem.buf_size = MIN_SIZE;

    return 0;
}

static void aml_vcodec_cma_free(struct aml_vcodec_ctx *ctx)
{
    if (ctx->wq->mem.buf_start) {
        dma_free_coherent(ctx->dev, ctx->wq->mem.buf_size,
                          ctx->wq->mem.buf_start,
                          ctx->wq->mem.buf_start_phys);
        ctx->wq->mem.buf_start = NULL;
    }
}

static int aml_vcodec_canvas_init(struct aml_vcodec_ctx *ctx)
{
    int ret;

    ctx->canvas = meson_canvas_get(ctx->dev);
    if (IS_ERR(ctx->canvas)) {
        dev_err(ctx->dev, "Failed to get canvas\n");
        return PTR_ERR(ctx->canvas);
    }

    ret = meson_canvas_alloc(ctx->canvas, &ctx->canvas_index);
    if (ret) {
        dev_err(ctx->dev, "Failed to allocate canvas\n");
        return ret;
    }

    return 0;
}

static void aml_vcodec_canvas_free(struct aml_vcodec_ctx *ctx)
{
    if (ctx->canvas) {
        meson_canvas_free(ctx->canvas, ctx->canvas_index);
    }
}

// V4L2 file operations
static const struct v4l2_file_operations aml_vcodec_fops = {
    .owner          = THIS_MODULE,
    .open           = v4l2_fh_open,
    .release        = vb2_fop_release,
    .poll           = v4l2_m2m_fop_poll,
    .unlocked_ioctl = video_ioctl2,
    .mmap           = vb2_fop_mmap,
};

// V4L2 ioctl operations
static const struct v4l2_ioctl_ops aml_vcodec_ioctl_ops = {
    .vidioc_querycap        = aml_vcodec_querycap,
    .vidioc_enum_fmt_vid_cap = aml_vcodec_enum_fmt_vid_cap,
    .vidioc_enum_fmt_vid_out = aml_vcodec_enum_fmt_vid_out,
    .vidioc_g_fmt_vid_cap_mplane = aml_vcodec_g_fmt,
    .vidioc_g_fmt_vid_out_mplane = aml_vcodec_g_fmt,
    .vidioc_s_fmt_vid_cap_mplane = aml_vcodec_s_fmt,
    .vidioc_s_fmt_vid_out_mplane = aml_vcodec_s_fmt,
    .vidioc_try_fmt_vid_cap_mplane = aml_vcodec_try_fmt,
    .vidioc_try_fmt_vid_out_mplane = aml_vcodec_try_fmt,
    .vidioc_reqbufs         = v4l2_m2m_ioctl_reqbufs,
    .vidioc_querybuf        = v4l2_m2m_ioctl_querybuf,
    .vidioc_qbuf            = v4l2_m2m_ioctl_qbuf,
    .vidioc_dqbuf           = v4l2_m2m_ioctl_dqbuf,
    .vidioc_streamon        = v4l2_m2m_ioctl_streamon,
    .vidioc_streamoff       = v4l2_m2m_ioctl_streamoff,
    .vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
    .vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

// V4L2 M2M operations
static const struct v4l2_m2m_ops aml_vcodec_m2m_ops = {
    .device_run = aml_vcodec_device_run,
    .job_ready  = aml_vcodec_job_ready,
    .job_abort  = aml_vcodec_job_abort,
};

static int aml_vcodec_querycap(struct file *file, void *priv,
                               struct v4l2_capability *cap)
{
    strscpy(cap->driver, DRIVER_NAME, sizeof(cap->driver));
    strscpy(cap->card, "Amlogic AVC Encoder", sizeof(cap->card));
    snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s", DRIVER_NAME);
    cap->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
    return 0;
}

static int aml_vcodec_enum_fmt_vid_cap(struct file *file, void *priv,
                                       struct v4l2_fmtdesc *f)
{
    if (f->index != 0)
        return -EINVAL;

    f->pixelformat = V4L2_PIX_FMT_H264;
    return 0;
}

static int aml_vcodec_enum_fmt_vid_out(struct file *file, void *priv,
                                       struct v4l2_fmtdesc *f)
{
    if (f->index != 0)
        return -EINVAL;

    f->pixelformat = V4L2_PIX_FMT_YUV420;
    return 0;
}

static int aml_vcodec_g_fmt(struct file *file, void *priv,
                            struct v4l2_format *f)
{
    struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);

    if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
        *f = ctx->dst_fmt;
    else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
        *f = ctx->src_fmt;
    else
        return -EINVAL;

    return 0;
}

static int aml_vcodec_try_fmt(struct file *file, void *priv,
                              struct v4l2_format *f)
{
    struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;

    if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        pix_mp->pixelformat = V4L2_PIX_FMT_H264;
    } else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        pix_mp->pixelformat = V4L2_PIX_FMT_YUV420;
    } else {
        return -EINVAL;
    }

    pix_mp->field = V4L2_FIELD_NONE;
    pix_mp->num_planes = 1;
    pix_mp->plane_fmt[0].sizeimage = pix_mp->width * pix_mp->height * 3 / 2;

    return 0;
}

static int aml_vcodec_s_fmt(struct file *file, void *priv,
                            struct v4l2_format *f)
{
    struct aml_vcodec_ctx *ctx = fh_to_ctx(priv);
    struct vb2_queue *vq;
    struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
    int ret;

    vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
    if (!vq)
        return -EINVAL;

    if (vb2_is_busy(vq)) {
        v4l2_err(&ctx->dev->v4l2_dev, "queue is busy\n");
        return -EBUSY;
    }

    ret = aml_vcodec_try_fmt(file, priv, f);
    if (ret)
        return ret;

    if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        ctx->src_fmt = *f;
        ctx->wq->pic.encoder_width = pix_mp->width;
        ctx->wq->pic.encoder_height = pix_mp->height;
    } else if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        ctx->dst_fmt = *f;
    }

    return 0;
}

static int aml_vcodec_ctx_init(struct aml_vcodec_ctx *ctx)
{
    int ret;

    ctx->wq = kzalloc(sizeof(*ctx->wq), GFP_KERNEL);
    if (!ctx->wq)
        return -ENOMEM;

    ret = aml_vcodec_cma_init(ctx);
    if (ret)
        goto free_wq;

    ret = aml_vcodec_canvas_init(ctx);
    if (ret)
        goto free_cma;

    return 0;

free_cma:
    aml_vcodec_cma_free(ctx);
free_wq:
    kfree(ctx->wq);
    return ret;
}

static void aml_vcodec_ctx_release(struct aml_vcodec_ctx *ctx)
{
    aml_vcodec_canvas_free(ctx);
    aml_vcodec_cma_free(ctx);
    kfree(ctx->wq);
}

static int aml_vcodec_open(struct file *file)
{
    struct aml_vcodec_dev *dev = video_drvdata(file);
    struct aml_vcodec_ctx *ctx = NULL;
    int ret = 0;

    ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;

    mutex_init(&ctx->lock);
    ctx->dev = dev;

    v4l2_fh_init(&ctx->fh, video_devdata(file));
    file->private_data = &ctx->fh;
    v4l2_fh_add(&ctx->fh);

    ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(dev->m2m_dev, ctx,
                                        &aml_vcodec_queue_init);
    if (IS_ERR(ctx->fh.m2m_ctx)) {
        ret = PTR_ERR(ctx->fh.m2m_ctx);
        goto err_fh_free;
    }

    ret = aml_vcodec_ctx_init(ctx);
    if (ret)
        goto err_m2m_ctx_release;

    return 0;

err_m2m_ctx_release:
    v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
err_fh_free:
    v4l2_fh_del(&ctx->fh);
    v4l2_fh_exit(&ctx->fh);
    kfree(ctx);
    return ret;
}

static int aml_vcodec_release(struct file *file)
{
    struct aml_vcodec_ctx *ctx = fh_to_ctx(file->private_data);

    aml_vcodec_ctx_release(ctx);
    v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
    v4l2_fh_del(&ctx->fh);
    v4l2_fh_exit(&ctx->fh);
    kfree(ctx);

    return 0;
}

static const struct v4l2_file_operations aml_vcodec_fops = {
    .owner          = THIS_MODULE,
    .open           = aml_vcodec_open,
    .release        = aml_vcodec_release,
    .poll           = v4l2_m2m_fop_poll,
    .unlocked_ioctl = video_ioctl2,
    .mmap           = v4l2_m2m_fop_mmap,
};



static int aml_vcodec_probe(struct platform_device *pdev)
{
    struct aml_vcodec_dev *dev;
    struct video_device *vfd;
    int ret;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
    if (ret)
        return ret;

    dev->enc_base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(dev->enc_base)) {
        ret = PTR_ERR(dev->enc_base);
        goto err_unregister_v4l2_dev;
    }

    dev->canvas = meson_canvas_get(&pdev->dev);
    if (IS_ERR(dev->canvas)) {
        ret = PTR_ERR(dev->canvas);
        goto err_unregister_v4l2_dev;
    }

    vfd = video_device_alloc();
    if (!vfd) {
        ret = -ENOMEM;
        goto err_unregister_v4l2_dev;
    }

    vfd->fops = &aml_vcodec_fops;
    vfd->ioctl_ops = &aml_vcodec_ioctl_ops;
    vfd->release = video_device_release;
    vfd->lock = &dev->dev_mutex;
    vfd->v4l2_dev = &dev->v4l2_dev;
    vfd->vfl_dir = VFL_DIR_M2M;
    vfd->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
    strscpy(vfd->name, DRIVER_NAME, sizeof(vfd->name));

    video_set_drvdata(vfd, dev);

    dev->m2m_dev = v4l2_m2m_init(&aml_vcodec_m2m_ops);
    if (IS_ERR(dev->m2m_dev)) {
        ret = PTR_ERR(dev->m2m_dev);
        goto err_free_vfd;
    }

    ret = video_register_device(vfd, VFL_TYPE_VIDEO, 0);
    if (ret)
        goto err_m2m_release;

    dev->vfd = vfd;
    platform_set_drvdata(pdev, dev);

    pm_runtime_enable(&pdev->dev);

    dev->dev = &pdev->dev;
    dev->encode_manager = encode_manager;

    ret = avc_init(dev);
    if (ret)
        goto err_pm_disable;

    dev_info(dev->dev, "Amlogic AVC encoder probed successfully\n");

    return 0;

err_pm_disable:
    pm_runtime_disable(&pdev->dev);
    video_unregister_device(vfd);
err_m2m_release:
    v4l2_m2m_release(dev->m2m_dev);
err_free_vfd:
    video_device_release(vfd);
err_unregister_v4l2_dev:
    v4l2_device_unregister(&dev->v4l2_dev);
    return ret;
}

static int aml_vcodec_remove(struct platform_device *pdev)
{
    struct aml_vcodec_dev *dev = platform_get_drvdata(pdev);

    avc_uninit(dev);
    pm_runtime_disable(&pdev->dev);
    v4l2_m2m_release(dev->m2m_dev);
    video_unregister_device(dev->vfd);
    v4l2_device_unregister(&dev->v4l2_dev);

    return 0;
}

static const struct of_device_id aml_vcodec_match[] = {
    { .compatible = "amlogic,meson-h264-encoder" },
    { },
};
MODULE_DEVICE_TABLE(of, aml_vcodec_match);

static struct platform_driver aml_vcodec_driver = {
    .probe  = aml_vcodec_probe,
    .remove = aml_vcodec_remove,
    .driver = {
        .name   = DRIVER_NAME,
        .of_match_table = aml_vcodec_match,
    },
};

static int __init aml_vcodec_init(void)
{
    return platform_driver_register(&aml_vcodec_driver);
}

static void __exit aml_vcodec_exit(void)
{
    platform_driver_unregister(&aml_vcodec_driver);
}

module_init(aml_vcodec_init);
module_exit(aml_vcodec_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Amlogic AVC Encoder V4L2 Driver");
MODULE_AUTHOR("Your Name");
