//
// Created by 胡启奇 on 2022/11/1.
//

#ifndef RTSP_PUSHWORKER_H
#define RTSP_PUSHWORKER_H
#include "common.h"

class VideoEncode;
class VideoCapture;
class RtspPusher;

class PushWorker {
public:
    PushWorker();
    ~PushWorker();

    COMMON_CODE Init(const Properties &properties);
    void DeInit();

private:
    void VideoCaptureCallBack(uint8_t *yuv, int32_t size);
    void AudioCaptureCallBack(uint8_t *yuv, int32_t size);

private:
    PushStreamParam param_;
    // 视频编码器
    VideoEncode *vEncode_;
    // 视频采集器
    VideoCapture *vCapture_;

    // rtsp推流
    RtspPusher* rtspPusher_;
};


#endif//RTSP_PUSHWORKER_H
