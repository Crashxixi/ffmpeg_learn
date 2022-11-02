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
};

/**
 * 线程安全
 */
class PacketQueue {
public:
    PacketQueue();
    ~PacketQueue();

private:
    std::mutex mtx_;
    std::condition_variable cond_;
    std::queue<MediaPacket *> queue_;
};


#endif//RTSP_PACKETQUEUE_H
