//
// Created by 胡启奇 on 2022/11/1.
//

#ifndef RTSP_RTSPPUSHER_H
#define RTSP_RTSPPUSHER_H
#include "CommonLooper.h"
#include "common.h"

class AVFormatContext;
class AVStream;
class AVCodecContext;
class PacketQueue;
class AVPacket;

class RtspPusher : public CommonLooper {
public:
    RtspPusher();
    ~RtspPusher();

    COMMON_CODE Init(const Properties &properties);
    void DeInit();

    COMMON_CODE Connect();
    COMMON_CODE PushPkt(AVPacket *pkt, const MEDIA_TYPE &type);

    virtual void Loop() override;

    COMMON_CODE ConfigVideoStream(const AVCodecContext *codec_ctx);
    COMMON_CODE ConfigAudioStream(const AVCodecContext *codec_ctx);

private:
    RtspPusherParam param_;
    // 整个输出流的上下文
    AVFormatContext *fmt_ctx_;
    // 视频编码器上下文
    AVCodecContext *vCodec_ctx;
    // 音频频编码器上下文
    AVCodecContext *aCodec_ctx;
    // 音视频数据队列
    PacketQueue *pktQueue_;
    // 流成分
    int vIndex_;
    int aIndex_;
    AVStream *vStream_;
    AVStream *aStream_;
};


#endif//RTSP_RTSPPUSHER_H
