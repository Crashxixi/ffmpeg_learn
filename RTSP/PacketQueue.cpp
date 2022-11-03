//
// Created by 胡启奇 on 2022/11/2.
//

#include "PacketQueue.h"
#include "ffmpeg.h"
#include "logger.h"

PacketQueue::PacketQueue()
    : abortReq_(false) {
}

PacketQueue::~PacketQueue() {
}
COMMON_CODE PacketQueue::Push(AVPacket *pkt, const MEDIA_TYPE &type) {
    if (!pkt) {
        LogError("pkt is null");
        return CODE_FAIL;
    }

    if (type != MEDIA_VIDEO && type != MEDIA_AUDIO) {
        LogError("media type error");
        return CODE_FAIL;
    }

    if (abortReq_) {
        LogWarn("abort request");
        return CODE_FAIL;
    }

    MediaPacket *mediaPkt = new MediaPacket(pkt, type);

    {
        std::lock_guard<std::mutex> locker(mtx_);
        if (type == MEDIA_VIDEO) {
            info_.video_nb_packets++;
            info_.video_size += mediaPkt->pkt_->size;

        } else if (type == MEDIA_AUDIO) {
            info_.audio_nb_packets++;
            info_.audio_size += mediaPkt->pkt_->size;
        }
        queue_.push(mediaPkt);
    }

    return CODE_SUCCESS;
}
COMMON_CODE PacketQueue::Pop(AVPacket **pkt, MEDIA_TYPE &type, int timeout /* = 0*/) {
    if (!pkt) {
        LogError("pkt is null");
        return CODE_FAIL;
    }

    if (abortReq_) {
        LogWarn("abort request");
        return CODE_FAIL;
    }

    std::unique_lock<std::mutex> locker(mtx_);

    if (timeout <= 0) {
        cond_.wait(locker, [this]() {
            return !queue_.empty() | abortReq_;
        });
    } else {
        cond_.wait_for(locker, std::chrono::milliseconds(timeout), [this] {
            return true;
        });
    }

    if (abortReq_) {
        LogWarn("abort request");
        return CODE_FAIL;
    }

    MediaPacket *mediaPkt = queue_.front();
    *pkt = mediaPkt->pkt_;
    type = mediaPkt->type_;

    if (type == MEDIA_VIDEO) {
        info_.video_nb_packets--;
        info_.video_size -= mediaPkt->pkt_->size;

    } else if (type == MEDIA_AUDIO) {
        info_.audio_nb_packets--;
        info_.audio_size -= mediaPkt->pkt_->size;
    }

    queue_.pop();
    SAFE_DELETE(mediaPkt);
    return CODE_SUCCESS;
}

bool PacketQueue::Empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return queue_.empty();
}

void PacketQueue::Abort() {
    abortReq_ = false;
    cond_.notify_all();
}
