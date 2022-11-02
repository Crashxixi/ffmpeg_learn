//
// Created by 胡启奇 on 2022/11/1.
//

#include "PushWorker.h"
#include "RtspPusher.h"
#include "VideoCapture.h"
#include "VideoEncode.h"
#include "ffmpeg.h"
#include "logger.h"

PushWorker::PushWorker()
    : vEncode_(nullptr), vCapture_(nullptr), rtspPusher_(nullptr) {
}

PushWorker::~PushWorker() {
    if (vCapture_) {
        SAFE_DELETE(vCapture_);
    }
    if (vEncode_) {
        SAFE_DELETE(vEncode_);
    }
    if (rtspPusher_) {
        SAFE_DELETE(rtspPusher_);
    }
}

COMMON_CODE PushWorker::Init(const Properties &properties) {
    // 视频采集相关
    param_.vCaptureParam_.width_ = properties.GetProperty("width", 1280);
    param_.vCaptureParam_.height_ = properties.GetProperty("height", 720);
    param_.vCaptureParam_.fps_ = properties.GetProperty("fps", 25);
    param_.vCaptureParam_.fmt_name_ = properties.GetProperty("fmt_name", "avfoundation");
    param_.vCaptureParam_.pix_fmt_ = properties.GetProperty("pix_fmt", "yuyv422");

    // 视频编码相关
    param_.vEncodeParam_.width_ = properties.GetProperty("width", 0);
    param_.vEncodeParam_.height_ = properties.GetProperty("height", 0);
    param_.vEncodeParam_.fps_ = properties.GetProperty("fps", 0);
    param_.vEncodeParam_.bFrames_ = properties.GetProperty("bFrames", 0);
    param_.vEncodeParam_.bitrate_ = properties.GetProperty("bitrate", 500 * 1024);
    param_.vEncodeParam_.gop_ = properties.GetProperty("gop", param_.vEncodeParam_.fps_);
    param_.vEncodeParam_.pix_fmt_ = properties.GetProperty("pix_fmt", AV_PIX_FMT_YUV420P);
    param_.vEncodeParam_.qp_min_ = properties.GetProperty("qp_min", 10);
    param_.vEncodeParam_.qp_max_ = properties.GetProperty("qp_max", 30);
    param_.vEncodeParam_.codec_name_ = properties.GetProperty("codec_name", "default");
    param_.vEncodeParam_.preset_ = properties.GetProperty("preset", "medium");
    param_.vEncodeParam_.tune_ = properties.GetProperty("tune", "zerolatency");
    param_.vEncodeParam_.profile_ = properties.GetProperty("profile", "high");

    // 音频采集相关

    // 音频编码相关

    // 注册设备
    avdevice_register_all();
    LogTrace("call avdevice_register_all");

    // 初始化视频编码器
    int iRet = CODE_FAIL;
    vEncode_ = new VideoEncode();
    iRet = vEncode_->Init(param_.vEncodeParam_.GetProperties());
    if (iRet != CODE_SUCCESS) {
        LogError("video encode init fail");
        return CODE_INIT_VIDEO_ENCODE_FAIL;
    }

    // 初始化音频编码器

    // 初始化RTSP推流
    rtspPusher_ = new RtspPusher();
    iRet = rtspPusher_->Init(param_.rtspParam_.GetProperties());
    if (iRet != CODE_SUCCESS) {
        LogError("rtsp pusher init fail");
        return CODE_INIT_RTSP_PUSHER_INIT_FAIL;
    }

    // 创建视频流
    iRet = rtspPusher_->ConfigVideoStream(vEncode_->GetVideoCodecContext());
    if (iRet != CODE_SUCCESS) {
        LogError("rtsp pusher config video stream fail");
        return CODE_FAIL;
    }

    // 创建音频流

    // rtsp连接
    iRet = rtspPusher_->Connect();
    if (iRet != CODE_SUCCESS) {
        LogError("rtsp connect fail");
        return CODE_CONNECT_FAIL;
    }

    // 初始化视频采集
    vCapture_ = new VideoCapture();
    iRet = vCapture_->Init(param_.vCaptureParam_.GetProperties());
    if (iRet != CODE_SUCCESS) {
        LogError("video capture init fail");
        return CODE_INIT_VIDEO_CAPTURE_FAIL;
    }

    vCapture_->SetCallBack(std::bind(&PushWorker::VideoCaptureCallBack, this,
                                     std::placeholders::_1, std::placeholders::_2));

    // 初始化音频采集


    iRet = vCapture_->Start();
    if (iRet != CODE_SUCCESS) {
        LogError("video capture start fail");
        return CODE_FAIL;
    }

    return CODE_SUCCESS;
}

void PushWorker::DeInit() {
    if (vCapture_) {
        vCapture_->Stop();
        vCapture_->SetCallBack(nullptr);
        SAFE_DELETE(vCapture_);
    }
}

void PushWorker::VideoCaptureCallBack(uint8_t *yuv, int32_t size) {
}

void PushWorker::AudioCaptureCallBack(uint8_t *yuv, int32_t size) {
}
