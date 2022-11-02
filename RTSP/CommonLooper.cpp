//
// Created by 胡启奇 on 2022/10/31.
//

#include "CommonLooper.h"
#include "logger.h"

CommonLooper::CommonLooper()
    : worker_(nullptr), abortReq_(false), running_(false) {
}

CommonLooper::~CommonLooper() {
}

COMMON_CODE CommonLooper::Start() {
    worker_ = new std::thread(trampoline, this);
    if (!worker_) {
        LogError("Common looper start error");
        return CODE_FAIL;
    }
    return CODE_SUCCESS;
}

void CommonLooper::Stop() {
    abortReq_ = true;

    if (worker_) {
        worker_->join();
        delete worker_;
        worker_ = nullptr;
    }
}

bool CommonLooper::isRunning() {
    return running_;
}

void CommonLooper::trampoline(void *obj) {
    CommonLooper *looper = (CommonLooper *) (obj);
    if (looper) {
        looper->Loop();
    }
}
