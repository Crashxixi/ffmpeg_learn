#include "PushWorker.h"
#include "common.h"
#include "logger.h"

int main() {
    LogInfo("rtsp push stream begin...");

    PushWorker worker;
    Properties properties;
    // 视频采集
    properties.SetProperty("width", 1280);
    properties.SetProperty("height", 720);
    properties.SetProperty("fps", 25);
    properties.SetProperty("fmt_name", "avfoundation");
    properties.SetProperty("pix_fmt", "yuyv422");
    // 视频编码
    properties.SetProperty("width", 1280);
    properties.SetProperty("height", 720);
    properties.SetProperty("fps", 25);
    properties.SetProperty("bFrames", 0);
    properties.SetProperty("bitrate", 500 * 1024);
    properties.SetProperty("pix_fmt", 1 * 25);
    properties.SetProperty("qp_min", 10);
    properties.SetProperty("qp_max", 30);
    properties.SetProperty("preset", "medium");
    properties.SetProperty("tune", "zerolatency");
    properties.SetProperty("profile", "high");
    // 音频采集

    // 音频编码


    int iRet = worker.Init(properties);
    if (iRet != CODE_SUCCESS) {
        LogError("push worker init fail");
        return -1;
    }

    return 0;
}
