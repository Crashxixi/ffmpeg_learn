#include <chrono>
#include <iostream>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}
#ifdef _WIN32
#define FORMAT_NAME "dshow"
#elif __linux__
#define FORMAT_NAME "video4linux2"
#elif __APPLE__
#define FORMAT_NAME "avfoundation"
#endif

using namespace std;
using namespace std::chrono;

void YUYV422_TO_YUV420P(uint8_t *yuyv422, uint8_t *yuv420p, int video_width, int video_height) {
    // 1、申请空间
    AVFrame *pInFrame = av_frame_alloc();
    AVFrame *pOutFrame = av_frame_alloc();
    // 2、设置转化上下文
    SwsContext *pSwsCtx = sws_getContext(
            video_width, video_height, AV_PIX_FMT_YUYV422,//输入
            video_width, video_height, AV_PIX_FMT_YUV420P,//输出
            SWS_BICUBIC, nullptr, nullptr, nullptr);
    // 3、为AVFrame分配内存
    av_image_fill_arrays(pInFrame->data, pInFrame->linesize, yuyv422,
                         AV_PIX_FMT_YUYV422, video_width, video_height, 1);
    av_image_fill_arrays(pOutFrame->data, pOutFrame->linesize, yuv420p,
                         AV_PIX_FMT_YUV420P, video_width, video_height, 1);

    // 4、像素格式转化
    sws_scale(pSwsCtx, (uint8_t const **) pInFrame->data,
              pInFrame->linesize, 0, video_height,
              pOutFrame->data, pOutFrame->linesize);
    // 5、释放空间
    if (pInFrame)
        av_frame_free(&pInFrame);
    if (pOutFrame)
        av_frame_free(&pOutFrame);
    if (pSwsCtx)
        sws_freeContext(pSwsCtx);
}

int main() {
    cout << "Video Capture Begin!" << endl;

    // 1、注册
    avdevice_register_all();

    // 2、获取输入格式
    const AVInputFormat *pInputFmt = av_find_input_format(FORMAT_NAME);
    if (!pInputFmt) {
        cout << "av_find_input_format error" << endl;
        return -1;
    }

    // 3、设置参数
    AVDictionary *inputOptions = nullptr;
    int iWidth = 1280;
    int iHeight = 720;
    int fps = 30;
    std::string strPixfmt = "yuyv422";
    std::string strResolution = std::to_string(iWidth) + "x" + std::to_string(iHeight);
    std::string strFps = std::to_string(fps);
    // TODO：获取摄像头支持都分辨率列表
    av_dict_set(&inputOptions, "video_size", strResolution.c_str(), 0);
    // TODO：获取摄像头支持都图像格式（yuyv422大多数摄像头都支持）
    av_dict_set(&inputOptions, "pixel_format", strPixfmt.c_str(), 0);
    // TODO：获取摄像头支持都帧率范围
    av_dict_set(&inputOptions, "framerate", strFps.c_str(), 0);

    std::string strDeviceName;
#ifdef _WIN32
    // TODO：获取设备名称
    strDeviceName = "video=";
#elif __linux__
    // TODO：获取设备名称
    strDeviceName = "video=";
#elif __APPLE__
    strDeviceName = "0:";
#endif

    // 4、打开设备
    AVFormatContext *pFmtCtx = nullptr;
    int iRet = avformat_open_input(&pFmtCtx, strDeviceName.c_str(), pInputFmt, &inputOptions);
    if (iRet < 0) {
        cout << "avformat_open_input error, return code " << iRet << endl;
        return iRet;
    }
    cout << "Param info => [device_name] = " << strDeviceName << " ,[resolution] = " << strResolution
         << " ,[pixfmt] = " << strPixfmt << " , [fps] = " << strFps << endl;

    // 计算一帧图像都大小，这里只为了后续写文件
    int yuyv422Size = av_image_get_buffer_size(AV_PIX_FMT_YUYV422, iWidth, iHeight, 1);
    int yuv420pSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, iWidth, iHeight, 1);
    cout << "yuyv422 frame size = " << yuyv422Size << endl;
    cout << "yuv420p frame size = " << yuv420pSize << endl;

    // 打开文件
    const string strYUYV422FileName = "Stream_" + strResolution + "_yuyv422_" + strFps + "fps.yuv";
    const string strYUV420pFileName = "Stream_" + strResolution + "_yuv420p_" + strFps + "fps.yuv";

    FILE *yuyv422_fd = fopen(strYUYV422FileName.c_str(), "wb+");
    if (!yuyv422_fd) {
        cout << "open file error" << endl;
        avformat_close_input(&pFmtCtx);
        return -1;
    }

    FILE *yuv420p_fd = fopen(strYUV420pFileName.c_str(), "wb+");
    if (!yuv420p_fd) {
        cout << "open file error" << endl;
        avformat_close_input(&pFmtCtx);
        return -1;
    }

    // 5、采集数据
    AVPacket *pkt = av_packet_alloc();
    uint8_t *yuv420p = new uint8_t[yuv420pSize];
    int iFrameNum = 0;
    time_point<system_clock> start;
    time_point<system_clock> end;
    while (true) {
        iRet = av_read_frame(pFmtCtx, pkt);
        if (iRet == 0)// 成功
        {
            if (iFrameNum == 0) {
                start = system_clock::now();
                ++iFrameNum;
            } else if (iFrameNum == fps) {
                end = system_clock::now();
                auto elapsed = duration_cast<milliseconds>(end - start).count();
                cout << fps << "fps, elapsed time: " << elapsed << "ms" << endl;
                iFrameNum = 0;
            } else {
                ++iFrameNum;
            }

            fwrite(pkt->data, 1, yuyv422Size, yuyv422_fd);
            YUYV422_TO_YUV420P(pkt->data, yuv420p, iWidth, iHeight);
            fwrite(yuv420p, 1, yuv420pSize, yuv420p_fd);

            av_packet_unref(pkt);
        } else if (iRet == AVERROR(EAGAIN)) {
            continue;
        } else {
            cout << "av_read_frame error, return code " << iRet << endl;
            break;
        }
    }

    // 6、资源释放
    av_packet_free(&pkt);
    avformat_close_input(&pFmtCtx);

    if (yuyv422_fd) {
        fclose(yuyv422_fd);
    }
    if (yuv420p_fd) {
        fclose(yuv420p_fd);
    }
    if (yuv420p) {
        delete[] yuv420p;
        yuv420p = nullptr;
    }

    cout << "Video Capture End" << endl;
    return 0;
}