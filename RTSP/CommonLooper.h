//
// Created by 胡启奇 on 2022/10/31.
//

#ifndef RTSP_COMMONLOOPER_H
#define RTSP_COMMONLOOPER_H

#include "common.h"
#include <thread>

class CommonLooper {
public:
    CommonLooper();
    virtual ~CommonLooper();

    // 开启线程
    COMMON_CODE Start();
    // 结束线程
    void Stop();
    // 是否运行
    bool isRunning();

    virtual void Loop() = 0;

protected:
    std::thread* worker_;
    bool abortReq_;
    bool running_;

private:
    static void trampoline(void* obj);
};


#endif//RTSP_COMMONLOOPER_H
