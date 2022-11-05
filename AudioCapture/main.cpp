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

int main() {
    cout << "Audio Capture Begin!" << endl;

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

    std::string strDeviceName;
#ifdef _WIN32
    // TODO：获取设备名称
    strDeviceName = "audio=";
#elif __linux__
    // TODO：获取设备名称
    strDeviceName = "audio=";
#elif __APPLE__
    strDeviceName = ":0";
#endif

    // 4、打开设备
    AVFormatContext *pFmtCtx = nullptr;
    int iRet = avformat_open_input(&pFmtCtx, strDeviceName.c_str(), pInputFmt, nullptr);
    if (iRet < 0) {
        cout << "avformat_open_input error, return code " << iRet << endl;
        return iRet;
    }
//    cout << "Param info => [device_name] = " << strDeviceName << " ,[resolution] = " << strResolution
//         << " ,[pixfmt] = " << strPixfmt << " , [fps] = " << strFps << endl;

    // 获取输入流
    AVStream *stream = pFmtCtx->streams[0];
    const AVCodec *audioInCodec = avcodec_find_decoder(stream->codecpar->codec_id);
    AVCodecContext * audioInCodecCtx = avcodec_alloc_context3(audioInCodec);
    avcodec_parameters_to_context(audioInCodecCtx, stream->codecpar);
    avcodec_open2(audioInCodecCtx, audioInCodec, nullptr);

    // 获取音频参数
    AVCodecParameters *params = stream->codecpar;
    // 声道数
    int iChannels = params->channels;
    cout << "iChannels " << iChannels << endl;
    // 采样率
    int iSample_rate = params->sample_rate;
    cout << "iSample_rate " << iSample_rate << endl;
    // 采样格式
    int iFormat = params->format;
    cout << "iFormat " << iFormat << endl;
    // 每一个样本的一个声道占用多少个字节
    int iBytes = av_get_bytes_per_sample((AVSampleFormat) params->format);
    cout << "iBytes " << iBytes << endl;

    // 计算一帧图像都大小，这里只为了后续写文件
//    int yuyv422Size = av_image_get_buffer_size(AV_PIX_FMT_YUYV422, iWidth, iHeight, 1);
//    int yuv420pSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, iWidth, iHeight, 1);

//    cout << "yuyv422 frame size = " << yuyv422Size << endl;
//    cout << "yuv420p frame size = " << yuv420pSize << endl;

    // 打开文件
    const string strPCMFileName = "Stream_Audio.pcm";
    //const string strYUV420pFileName = "Stream_" + strResolution + "_yuv420p_" + strFps + "fps.yuv";

    FILE *pcm_fd = fopen(strPCMFileName.c_str(), "wb+");
    if (!pcm_fd) {
        cout << "open file error" << endl;
        avformat_close_input(&pFmtCtx);
        return -1;
    }

    // 5、采集数据
    AVPacket *pkt = av_packet_alloc();
    //uint8_t *yuv420p = new uint8_t[yuv420pSize];
    time_point<system_clock> start;
    time_point<system_clock> end;
    while (true) {
        iRet = av_read_frame(pFmtCtx, pkt);
        if (iRet == 0)// 成功
        {
            fwrite(pkt->data, 1, pkt->size, pcm_fd);
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

    if (pcm_fd) {
        fclose(pcm_fd);
    }

    cout << "Audio Capture End" << endl;
    return 0;
}