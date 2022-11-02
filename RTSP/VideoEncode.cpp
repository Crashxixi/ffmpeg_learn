//
// Created by 胡启奇 on 2022/10/31.
//

#include "VideoEncode.h"
#include "ffmpeg.h"
#include "logger.h"

VideoEncode::VideoEncode() : codec_ctx_(nullptr) {
}

VideoEncode::~VideoEncode() {
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_);
    }
    if (frame_) {
        av_frame_free(&frame_);
    }
}

COMMON_CODE VideoEncode::Init(const Properties &properties) {
    param_.width_ = properties.GetProperty("width", 0);
    if (param_.width_ == 0 || param_.width_ % 2 != 0) {
        LogError("width error, width = %d", param_.width_);
        return CODE_FAIL;
    }

    param_.height_ = properties.GetProperty("height", 0);
    if (param_.height_ == 0 || param_.height_ % 2 != 0) {
        LogError("height error, height = %d", param_.height_);
        return CODE_FAIL;
    }

    param_.fps_ = properties.GetProperty("fps", 0);
    if (param_.fps_ == 0) {
        LogError("fps error, fps = %d", param_.fps_);
        return CODE_FAIL;
    }

    param_.bFrames_ = properties.GetProperty("bFrames", 0);
    param_.bitrate_ = properties.GetProperty("bitrate", 500 * 1024);
    param_.gop_ = properties.GetProperty("gop", param_.fps_);
    param_.pix_fmt_ = properties.GetProperty("pix_fmt", AV_PIX_FMT_YUV420P);
    param_.qp_min_ = properties.GetProperty("qp_min", 10);
    param_.qp_max_ = properties.GetProperty("qp_max", 30);
    param_.codec_name_ = properties.GetProperty("codec_name", "default");
    param_.preset_ = properties.GetProperty("preset", "medium");
    param_.tune_ = properties.GetProperty("tune", "zerolatency");
    param_.profile_ = properties.GetProperty("profile", "high");

    // 查找编码器
    const AVCodec *codec = nullptr;
    if (param_.codec_name_ == "default") {
        LogInfo("set default encoder");
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    } else {
        LogInfo("set %s encoder", param_.codec_name_.c_str());
        codec = avcodec_find_encoder_by_name(param_.codec_name_.c_str());
    }

    if (!codec) {
        LogError("can not find encoder");
        return CODE_FAIL;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        LogError("avcodec_alloc_context3 error");
        return CODE_FAIL;
    }

    // 编码器参数设置
    // 最大和最小量化系数，取值范围为0~51
    // QP(quantization parameter) 越大，精度损失越大，图像越不清晰
    codec_ctx_->qmin = param_.qp_min_;
    codec_ctx_->qmax = param_.qp_max_;
    // 编码后的视频帧大小，以像素为单位
    codec_ctx_->width = param_.width_;
    codec_ctx_->height = param_.height_;
    // 编码后的码率：值越大越清晰，值越小越流畅
    codec_ctx_->bit_rate = param_.bitrate_;
    // gop
    codec_ctx_->gop_size = param_.gop_;
    // 帧率的基本单位
    // time_base.num为时间线分子，time_base.den为时间线分母，帧率=分子/分母。
    codec_ctx_->time_base.num = 1;
    codec_ctx_->time_base.den = param_.fps_;//fps
    codec_ctx_->framerate.num = param_.fps_;//fps
    codec_ctx_->framerate.den = 1;
    // 图像色彩空间的格式，采用什么样的色彩空间来表明一个像素点
    codec_ctx_->pix_fmt = (AVPixelFormat) param_.pix_fmt_;
    // 编码器编码的数据类型
    codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    // B帧数量
    // 两个非B帧之间允许出现多少个B帧数，设置0表示不使用B帧，没有编码延时。B帧越多，压缩率越高
    codec_ctx_->max_b_frames = param_.bFrames_;
    // 线程数量
    // 开了多线程后也会导致帧输出延迟, 需要缓存thread_count帧后再编码
    codec_ctx_->thread_count = param_.thread_num_;
    // 开启多线程需要设置为FF_THREAD_FRAME
    codec_ctx_->thread_type = FF_THREAD_FRAME;
    /* 对于H264 AV_CODEC_FLAG_GLOBAL_HEADER  设置则只包含I帧，此时sps pps需要从codec_ctx->extradata读取
     *  不设置则每个I帧都带 sps pps sei
     */
    // 存本地文件时不要去设置
    codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    AVDictionary *codecOptions = nullptr;
    int iRet = 0;
    if (codec_ctx_->codec_id == AV_CODEC_ID_H264) {
        iRet = av_dict_set(&codecOptions, "preset", param_.preset_.c_str(), 0);
        if (0 != iRet) {
            LogError("av_dict_set preset error");
        }
        // 没有延迟的输出
        iRet = av_dict_set(&codecOptions, "tune", param_.tune_.c_str(), 0);
        if (0 != iRet) {
            LogError("av_dict_set tune error");
        }
        iRet = av_dict_set(&codecOptions, "profile", param_.profile_.c_str(), 0);
        if (0 != iRet) {
            LogError("av_dict_set profile error");
        }
    }

    // 打开解码器
    iRet = avcodec_open2(codec_ctx_, codec, &codecOptions);
    if (iRet < 0) {
        char buf[1024] = {0};
        av_strerror(iRet, buf, sizeof(buf) - 1);
        LogError("avcodec_open2 error, reason is %s", buf);
        return CODE_FAIL;
    }

    // 初始化AVFrame
    AVFrame *pFrame = av_frame_alloc();
    pFrame->width = codec_ctx_->width;
    pFrame->height = codec_ctx_->height;
    pFrame->format = codec_ctx_->pix_fmt;

    // 为AVFrame增加引用计数
    av_frame_get_buffer(pFrame, 0);

    return CODE_SUCCESS;
}

COMMON_CODE VideoEncode::Encode(uint8_t *yuv, int size, int64_t pts, AVPacket *pkt) {
    int iRet = CODE_FAIL;
    if (!yuv) {
        iRet = avcodec_send_frame(codec_ctx_, nullptr);
    } else {
        int iNeedSize = av_image_fill_arrays(frame_->data, frame_->linesize,
                                             yuv, (AVPixelFormat) frame_->format,
                                             frame_->width, frame_->height, 1);

        if (iNeedSize != size) {
            LogError("encode need size != input size");
            pkt = nullptr;
            return CODE_FAIL;
        }

        frame_->pts = pts;
        frame_->pict_type = AV_PICTURE_TYPE_NONE;
        iRet = avcodec_send_frame(codec_ctx_, frame_);
    }

    if (iRet < 0) {
        // 不能正常处理该frame
        char buf[1024] = {0};
        av_strerror(iRet, buf, sizeof(buf) - 1);
        LogError("avcodec_send_frame error, reason is %s", buf);

        if (iRet == AVERROR(EAGAIN)) {
            /**
             *  当前状态下编码器不接受输入，必须使用 avcodec_receive_packet()读取packet，
             *  一旦读取所有packet，应重新发送frame，并且这次调用不会再有 EAGAIN 的错误）
             */
            pkt = nullptr;
            return CODE_EAGAIN;
        } else if (iRet == AVERROR_EOF) {
            /**
             * 编码器已被刷新，无法向其发送新的frame
             * */
            pkt = nullptr;
            return CODE_EOF;
        } else {
            /**
             * 真正报错
             */
            pkt = nullptr;
            return CODE_FAIL;
        }
    }

    // 此时，iRet>=0 说明数据送给编码器成功，此后，才能从编码器中去获取编码好的视频帧数据
    // 获取编码后的视频数据,如果成功，需要重复获取，直到失败为止
    /**
     * 向编码器送一帧视频帧数据的时候，编码器不一定就会马上返回一个AVPacket视频帧。
     * 有可能是我们送了很多帧视频数据后，编码器才会返回一个编码好的AVPacket视频帧。
     * 也有可能同时返回多个编码好的AVPacket视频帧。
     */
    while (iRet >= 0) {
        iRet = avcodec_receive_packet(codec_ctx_, pkt);
        /**
         * iRet >= 0  表示本次获取成功，然后继续循环获取，因为有可能后面还有编码好的数据需要获取
         * iRet == AVERROR(EAGAIN) : 表示当前编码没有任何问题，但是输入的视频帧不够，所以当前没有packet输出，需要继续送入frame进行编码
         * iRet == AVERROR_EOF ：内部缓冲区中数据全部编码完成，不再有编码后的数据包输出。编码到最后了，没有任何数据了
         * other ,iRet < 0 && iRet!= AVERROR(EAGAIN) && iRet!=AVERROR_EOF ：编码错误，直接退出
         */
        if (iRet == AVERROR(EAGAIN)) {
            pkt = nullptr;
            return CODE_EAGAIN;
        }
        if (iRet == AVERROR_EOF) {
            pkt = nullptr;
            return CODE_EOF;
        } else if (iRet < 0) {
            pkt = nullptr;
            return CODE_FAIL;
        }

        if (pkt->flags & AV_PKT_FLAG_KEY) {
            LogDebug("receive packet flags:%d pts:%3\" PRId64 \" dts:%3\" PRId64 \" (size:%5d)",
                     pkt->flags, pkt->pts, pkt->dts, pkt->size);
        }
        if (!pkt->flags) {
            LogDebug("receive packet flags:%d pts:%3\" PRId64 \" dts:%3\" PRId64 \" (size:%5d)",
                     pkt->flags, pkt->pts, pkt->dts, pkt->size);
        }
        // 减少pkt引用计数
        av_packet_unref(pkt);
        return CODE_SUCCESS;
    }
}
