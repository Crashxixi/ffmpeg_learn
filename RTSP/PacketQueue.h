//
// Created by 胡启奇 on 2022/11/2.
//

#ifndef RTSP_PACKETQUEUE_H
#define RTSP_PACKETQUEUE_H

#include "common.h"
#include <condition_variable>
#include <mutex>
#include <queue>

class AVPacket;

struct MediaPacket {
    AVPacket *pkt_;
    MEDIA_TYPE type_;

    MediaPacket() : pkt_(nullptr), type_(MEDIA_UNKNOWN) {}
    MediaPacket(AVPacket *pkt, MEDIA_TYPE type) : pkt_(pkt), type_(type) {}
};

struct MediaInfo {
    int audio_nb_packets;  // 音频包数量
    int video_nb_packets;  // 视频包数量
    int audio_size;        // 音频总大小 字节
    int video_size;        // 视频总大小 字节
    int64_t audio_duration;// 音频持续时长
    int64_t video_duration;// 视频持续时长

    MediaInfo() : audio_nb_packets(0), audio_size(0), audio_duration(0),
                  video_nb_packets(0), video_size(0), video_duration(0) {}
};

/**
 * 线程安全
 */
class PacketQueue {
public:
    PacketQueue();
    ~PacketQueue();

    COMMON_CODE Push(AVPacket *pkt, const MEDIA_TYPE &type);
    COMMON_CODE Pop(AVPacket **pkt, MEDIA_TYPE &type, int timeout = 0);
    bool Empty();
    void Abort();

private:
    std::mutex mtx_;
    std::condition_variable cond_;
    std::queue<MediaPacket *> queue_;
    MediaInfo info_;
    bool abortReq_;
};


#endif//RTSP_PACKETQUEUE_H
