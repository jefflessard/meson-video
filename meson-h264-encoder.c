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
#include <linux/amlogic/meson_canvas.h>

#define DRIVER_NAME "meson-h264-encoder"
#define MIN_SIZE amvenc_buffspec[0].min_buffsize

struct aml_vcodec_ctx {
    struct v4l2_fh fh;
    struct v4l2_ctrl_handler ctrl_handler;
    struct v4l2_pix_format_mplane src_fmt;
    struct v4l2_pix_format_mplane dst_fmt;
    struct vb2_queue src_queue;
    struct vb2_queue dst_queue;
    struct device *dev;
    void __iomem *reg_base;
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
    struct encode_manager_s encode_manager;
    void __iomem *enc_base;
    struct meson_canvas *canvas;
};

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
                                   ctx->dst_fmt.height, 0, 0, 0);
    }

    return 0;
}

static int aml_vcodec_buf_prepare(struct vb2_buffer *vb)
{
    struct aml_vcodec_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
    struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
    struct v4l2_m2m_buffer *mb = container_of(vbuf, struct v4l2_m2m_buffer, vb);
    struct aml_video_buf *buf = container_of(mb, struct aml_video_buf, m2m_buf);

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
