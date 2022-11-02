//
// Created by 胡启奇 on 2022/10/31.
//

#ifndef RTSP_VIDEOCAPTURE_H
#define RTSP_VIDEOCAPTURE_H

#include "CommonLooper.h"
#include "common.h"
#include <functional>

class AVFormatContext;

class VideoCapture : public CommonLooper {
public:
    VideoCapture();
    virtual ~VideoCapture();

    virtual void Loop() override;

    /**
     * @brief 初始化
     * @param
     * @return
     */
    COMMON_CODE Init(const Properties &properties);

    void SetCallBack(std::function<void(uint8_t *, int32_t)> cb);

private:
    COMMON_CODE yuyv422_to_yuv420p(uint8_t *yuyv422, uint8_t *yuv420p, int video_width, int video_height);

private:
    VideoCaptureParam param_;
    int yuv420p_buf_size;
    uint8_t *yuv420p_buf;
    bool bFirst_;//是否为第一帧
    std::function<void(uint8_t *, int32_t)> cb_;
    AVFormatContext *fmt_ctx_;
};

#endif//RTSP_VIDEOCAPTURE_H
