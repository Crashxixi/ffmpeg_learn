//
// Created by 胡启奇 on 2022/11/1.
//

#include "RtspPusher.h"
#include "ffmpeg.h"
#include "logger.h"

RtspPusher::RtspPusher()
    : fmt_ctx_(nullptr), vCodec_ctx(nullptr), aCodec_ctx(nullptr),
      vStream_(nullptr), aStream_(nullptr), pktQueue_(nullptr),
      vIndex_(-1), aIndex_(-1) {
}

RtspPusher::~RtspPusher() {
}

COMMON_CODE RtspPusher::Init(const Properties &properties) {
    param_.url_ = properties.GetProperty("url", "");
    if (param_.url_.empty()) {
        LogError("url is empty");
        return CODE_FAIL;
    }
    param_.rtsp_transport_ = properties.GetProperty("rtsp_transport", "tcp");
    param_.timeout_ = properties.GetProperty("timeout", 5);
    param_.maxQueueCapacity_ = properties.GetProperty("maxQueueCapacity", 500);

    int iRet = CODE_FAIL;
    char buf[1024] = {0};
    // 初始化网络库
    iRet = avformat_network_init();
    if (iRet < 0) {
        av_strerror(iRet, buf, sizeof(buf) - 1);
        LogError("avformat_network_init error, reason is %s", buf);
        return CODE_INIT_NETWORK_FAIL;
    }

    iRet = avformat_alloc_output_context2(&fmt_ctx_, nullptr, "rtsp", param_.url_.c_str());
    if (iRet < 0) {
        av_strerror(iRet, buf, sizeof(buf) - 1);
        LogError("avformat_alloc_output_context2 error, reason is %s", buf);
        return CODE_FAIL;
    }
    iRet = av_opt_set(fmt_ctx_->priv_data, "rtsp_transport", param_.rtsp_transport_.c_str(), 0);
    if (iRet < 0) {
        av_strerror(iRet, buf, sizeof(buf) - 1);
        LogError("av_opt_set rtsp_transport error, reason is %s", buf);
        return CODE_FAIL;
    }

    return CODE_SUCCESS;
}

void RtspPusher::DeInit() {
    if (fmt_ctx_) {
        avformat_free_context(fmt_ctx_);
        fmt_ctx_ = nullptr;
    }
}

COMMON_CODE RtspPusher::Connect() {
    if (!vStream_ && !aStream_) {
        return CODE_FAIL;
    }
    LogInfo("connect to %s", param_.url_.c_str());
    //RestTiemout();

    LogTrace("call avformat_write_header");
    // 连接服务器
    int iRet = avformat_write_header(fmt_ctx_, nullptr);
    if (iRet < 0) {
        char buf[1024] = {0};
        av_strerror(iRet, buf, sizeof(buf) - 1);
        LogError("avformat_write_header error, reason is %s", buf);
        return CODE_CONNECT_FAIL;
    }
    LogTrace("call avformat_write_header over");
    // 启动线程
    return Start();
}

void RtspPusher::Loop() {
}

COMMON_CODE RtspPusher::ConfigVideoStream(const AVCodecContext *codec_ctx) {
    if (!fmt_ctx_ || !codec_ctx) {
        LogError("input param is null");
        return CODE_FAIL;
    }

    // 添加视频流
    vStream_ = avformat_new_stream(fmt_ctx_, nullptr);
    if (!vStream_) {
        LogError("avformat_new_stream failed");
        return CODE_FAIL;
    }
    vStream_->codecpar->codec_tag = 0;
    // 从编码器拷贝信息
    avcodec_parameters_from_context(vStream_->codecpar, codec_ctx);
    vCodec_ctx = const_cast<AVCodecContext *>(codec_ctx);
    // 复制流索引
    vIndex_ = vStream_->index;
    return CODE_SUCCESS;
}

COMMON_CODE RtspPusher::ConfigAudioStream(const AVCodecContext *codec_ctx) {
    if (!fmt_ctx_ || !codec_ctx) {
        LogError("input param is null");
        return CODE_FAIL;
    }

    // 添加音频流
    aStream_ = avformat_new_stream(fmt_ctx_, nullptr);
    if (!aStream_) {
        LogError("avformat_new_stream failed");
        return CODE_FAIL;
    }
    aStream_->codecpar->codec_tag = 0;
    // 从编码器拷贝信息
    avcodec_parameters_from_context(aStream_->codecpar, codec_ctx);
    aCodec_ctx = const_cast<AVCodecContext *>(codec_ctx);
    // 复制流索引
    aIndex_ = aStream_->index;
    return CODE_SUCCESS;
}
