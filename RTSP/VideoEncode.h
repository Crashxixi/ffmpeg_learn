//
// Created by 胡启奇 on 2022/10/31.
//

#ifndef RTSP_VIDEOENCODE_H
#define RTSP_VIDEOENCODE_H
#include "common.h"
#include <string>

class AVCodecContext;
class AVFrame;
class AVPacket;

class VideoEncode {
public:
    VideoEncode();
    ~VideoEncode();

    /**
     * @brief 初始化
     * @param
     * @return
     */
    COMMON_CODE Init(const Properties &properties);
    /**
     * @brief 编码
     * @param
     * @return
     */
    COMMON_CODE Encode(uint8_t *yuv, int size, int64_t pts, AVPacket *pkt);

    inline AVCodecContext *GetVideoCodecContext() {
        return codec_ctx_;
    }


private:
    VideoEncodeParam param_;
    AVCodecContext *codec_ctx_;
    AVFrame *frame_;
};


#endif//RTSP_VIDEOENCODE_H
