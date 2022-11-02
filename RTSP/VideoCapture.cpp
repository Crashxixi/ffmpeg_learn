//
// Created by 胡启奇 on 2022/10/31.
//

#include "VideoCapture.h"
#include "logger.h"
#include "ffmpeg.h"

VideoCapture::VideoCapture()
    : yuv420p_buf_size(0), bFirst_(true), fmt_ctx_(nullptr), yuv420p_buf(nullptr), cb_(nullptr) {}

VideoCapture::~VideoCapture() {
    if (yuv420p_buf) {
        delete[] yuv420p_buf;
        yuv420p_buf = nullptr;
    }
}

COMMON_CODE VideoCapture::Init(const Properties &properties) {
    param_.width_ = properties.GetProperty("width", 1280);
    param_.height_ = properties.GetProperty("height", 720);
    param_.fps_ = properties.GetProperty("fps", 25);
    param_.fmt_name_ = properties.GetProperty("fmt_name", "avfoundation");
    param_.pix_fmt_ = properties.GetProperty("pix_fmt", "yuyv422");

    // 获取输入格式
    const AVInputFormat *pInputFmt = av_find_input_format(param_.fmt_name_.c_str());
    if (!pInputFmt) {
        LogError("av_find_input_format error");
        return CODE_FAIL;
    }

    // 设置参数
    AVDictionary *inputOptions = nullptr;
    std::string strResolution = std::to_string(param_.width_) + "x" + std::to_string(param_.height_);
    std::string strFps = std::to_string(param_.fps_);
    // TODO：获取摄像头支持都分辨率列表
    av_dict_set(&inputOptions, "video_size", strResolution.c_str(), 0);
    // TODO：获取摄像头支持都图像格式（yuyv422大多数摄像头都支持）
    av_dict_set(&inputOptions, "pixel_format", param_.pix_fmt_.c_str(), 0);
    // TODO：获取摄像头支持都帧率范围
    av_dict_set(&inputOptions, "framerate", strFps.c_str(), 0);
    // 默认打开第一个摄像头
    std::string strDeviceName = "0:";

    // 打开设备
    int iRet = avformat_open_input(&fmt_ctx_, strDeviceName.c_str(), pInputFmt, &inputOptions);
    if (iRet < 0) {
        LogError("avformat_open_input error, return code = %d", iRet);
        return CODE_FAIL;
    }

    //TODO: 设置是否输出原始文件

    return CODE_SUCCESS;
}

void VideoCapture::Loop() {
    LogTrace("into VideoCapture loop");

    // 分辨率保证为偶数
    param_.width_ += param_.width_ % 2;
    param_.height_ += param_.height_ % 2;

    // 计算一帧的大小
    // yuv420p_buf_size = (width_ * height_ * 3) >> 1;
    yuv420p_buf_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, param_.width_, param_.height_, 1);
    assert(yuv420p_buf_size > 0);
    yuv420p_buf = new uint8_t[yuv420p_buf_size];

    // 采集数据
    LogTrace("into VideoCapture while loop");

    AVPacket *pkt = av_packet_alloc();
    int iRet = 0;
    while (true) {
        if (abortReq_) {
            break;
        }

        iRet = av_read_frame(fmt_ctx_, pkt);
        if (iRet == 0) {
            if (bFirst_) {
                bFirst_ = false;
                LogInfo("capture the first frame");
            }
            // 获取成功, 将yuyv422转换为yuv420p
            yuyv422_to_yuv420p(pkt->data, yuv420p_buf, param_.width_, param_.height_);
            if (cb_) {
                cb_(yuv420p_buf, yuv420p_buf_size);
            }

            av_packet_unref(pkt);
        } else if (iRet == AVERROR(EAGAIN)) {
            continue;
        } else {
            LogError("av_read_frame error, return code = %d", iRet);
            break;
        }
    }

    // 资源释放
    av_packet_free(&pkt);
    avformat_close_input(&fmt_ctx_);

    if (yuv420p_buf) {
        delete[] yuv420p_buf;
        yuv420p_buf = nullptr;
    }
}

void VideoCapture::SetCallBack(std::function<void(uint8_t *, int32_t)> cb) {
    cb_ = cb;
}

COMMON_CODE VideoCapture::yuyv422_to_yuv420p(uint8_t *yuyv422, uint8_t *yuv420p, int video_width, int video_height) {
    // 1、申请空间
    AVFrame *pInFrame = av_frame_alloc();
    AVFrame *pOutFrame = av_frame_alloc();
    // 2、设置转化上下文
    SwsContext *pSwsCtx = sws_getContext(
            video_width, video_height, AV_PIX_FMT_YUYV422,//输入
            video_width, video_height, AV_PIX_FMT_YUV420P,//输出
            SWS_BICUBIC, nullptr, nullptr, nullptr);
    // 3、为AVFrame分配内存
    av_image_fill_arrays(pInFrame->data, pInFrame->linesize, yuyv422,
                         AV_PIX_FMT_YUYV422, video_width, video_height, 1);
    av_image_fill_arrays(pOutFrame->data, pOutFrame->linesize, yuv420p,
                         AV_PIX_FMT_YUV420P, video_width, video_height, 1);

    // 4、像素格式转化
    sws_scale(pSwsCtx, (uint8_t const **) pInFrame->data,
              pInFrame->linesize, 0, video_height,
              pOutFrame->data, pOutFrame->linesize);
    // 5、释放空间
    if (pInFrame)
        av_frame_free(&pInFrame);
    if (pOutFrame)
        av_frame_free(&pOutFrame);
    if (pSwsCtx)
        sws_freeContext(pSwsCtx);
    return CODE_SUCCESS;
}
