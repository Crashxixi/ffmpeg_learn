#include <cstdarg>
#include <cstdio>
#include <string>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
}
using namespace std;

#define LOG(fmt, ...)                                                \
    do {                                                             \
        fprintf(stdout, "[log] [%s] [%d] ", __FUNCTION__, __LINE__); \
        fprintf(stdout, fmt, ##__VA_ARGS__);                         \
        fprintf(stdout, "\n");                                       \
    } while (0);

int64_t get_time() {
    return av_gettime_relative() / 1000;// 换算成毫秒
}

int encode(AVCodecContext *ctx, AVFrame *in, AVPacket *pkt, FILE *outFile) {
    int ret = 0;

    if (!ctx) {
        return -1;
    }

    if (in) {
        LOG("send frame to encoder, pts = %d", in->pts);
    }

    //送原始数据(yuv420p)给编码器进行编码
    /*
	参数1：编码器上下文
	参数2：需要编码的视频数据
	*/
    ret = avcodec_send_frame(ctx, in);
    if (ret < 0) {
        LOG("Error, Failed to send a frame for enconding!");
        return -1;
    }

    //如果ret>=0说明数据送给编码器成功，此后 我们才能从编码器中去获取编码好的视频帧数据
    while (ret >= 0) {
        //获取编码后的视频数据,如果成功，需要重复获取，直到失败为止
        /*
		我们向编码器送一帧视频帧数据的时候，编码器不一定就会马上返回一个AVPacket视频帧。
		有可能是我们送了很多帧视频数据后，编码器才会返回一个编码好的AVPacket视频帧。也有
		可能同时返回多个编码好的AVPacket视频帧。

		参数1：编码器上下文
		参数2：编码器编码后的视频帧数据
		*/
        ret = avcodec_receive_packet(ctx, pkt);

        //如果编码器数据不足时会返回  EAGAIN,或者到数据尾时会返回 AVERROR_EOF
        /*
		ret > 0  表示本次获取成功，然后继续循环获取，因为有可能后面还有编码好的数据需要获取
		ret == AVERROR(EAGAIN) : 表示当前编码没有任何问题，但是输入的视频频帧不够，所以当前没有packet输出，需要继续送入frame进行编码码
		ret == AVERROR_EOF ：内部缓冲区中数据全部编码完成，不再有编码后的数据包输出。编码到最后了 没有任何数据了
		other ,ret < 0 && ret!= AVERROR(EAGAIN) && ret!=AVERROR_EOF  :编码错误 直接退出
		*/
        if (ret == AVERROR(EAGAIN)) {
            //	printf("data is not enough\n");
            return 0;
        }
        if (ret == AVERROR_EOF) {
            //	printf("encoding end\n");
            return 0;
        } else if (ret < 0) {// 编码错误直接退出
            LOG("Error, encoding video frame")
            return -1;
        }

        if (pkt->flags & AV_PKT_FLAG_KEY)
            printf("Write packet flags:%d pts:%3" PRId64 " dts:%3" PRId64 " (size:%5d)\n",
                   pkt->flags, pkt->pts, pkt->dts, pkt->size);
        if (!pkt->flags)
            printf("Write packet flags:%d pts:%3" PRId64 " dts:%3" PRId64 " (size:%5d)\n",
                   pkt->flags, pkt->pts, pkt->dts, pkt->size);

        fwrite(pkt->data, 1, pkt->size, outFile);
        av_packet_unref(pkt);//减少pkt引用计数
    }
}

int main() {

    LOG("yuv to h264 begin!");

    const std::string strInFileName = "../Data/test_1280x720_yuv420p.yuv";
    const std::string strOutFileName = "../Data/test.h264";

    const int iWidth = 1280;
    const int iHeight = 720;
    const int fps = 30;

    // 打开文件
    FILE *yuv_fd = fopen(strInFileName.c_str(), "rb+");
    if (!yuv_fd) {
        LOG("open yuv file error");
        return -1;
    }

    FILE *h264_fd = fopen(strOutFileName.c_str(), "wb+");
    if (!h264_fd) {
        LOG("open h264 file error");
        return -1;
    }

    // 1、寻找编码器
    const AVCodec *pEncoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!pEncoder) {
        LOG("avcodec_find_encoder error");
        return -1;
    }

    // 2、分配解码器上下文
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pEncoder);
    if (!pCodecCtx) {
        LOG("avcodec_alloc_context3 error");
        return -1;
    }

    // 3、编码器参数设置
    // 最大和最小量化系数，取值范围为0~51
    // QP(quantization parameter) 越大，精度损失越大，图像越不清晰
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 31;

    // 编码后的视频帧大小，以像素为单位
    pCodecCtx->width = iWidth;
    pCodecCtx->height = iHeight;

    // 编码后的码率：值越大越清晰，值越小越流畅
    pCodecCtx->bit_rate = 4000;

    // gop
    pCodecCtx->gop_size = 1 * fps;

    // 帧率的基本单位
    // time_base.num为时间线分子，time_base.den为时间线分母，帧率=分子/分母。
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = fps;//fps

    pCodecCtx->framerate.num = fps;//fps
    pCodecCtx->framerate.den = 1;

    // 图像色彩空间的格式，采用什么样的色彩空间来表明一个像素点
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    // 编码器编码的数据类型
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;

    // B帧数量
    // 两个非B帧之间允许出现多少个B帧数，设置0表示不使用B帧，没有编码延时。B帧越多，压缩率越高
    pCodecCtx->max_b_frames = 0;

    //pCodecCtx->thread_count = 4;  // 开了多线程后也会导致帧输出延迟, 需要缓存thread_count帧后再编程。
    //pCodecCtx->thread_type = FF_THREAD_FRAME; // 并设置为FF_THREAD_FRAME

    /* 对于H264 AV_CODEC_FLAG_GLOBAL_HEADER  设置则只包含I帧，此时sps pps需要从codec_ctx->extradata读取
     *  不设置则每个I帧都带 sps pps sei
     */
    //pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // 存本地文件时不要去设置

    AVDictionary *codecOptions = nullptr;
    int iRet = 0;
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        iRet = av_dict_set(&codecOptions, "preset", "medium", 0);
        if (0 != iRet) {
            LOG("av_dict_set preset error");
        }
        // 没有延迟的输出
        iRet = av_dict_set(&codecOptions, "tune", "zerolatency", 0);
        if (0 != iRet) {
            LOG("av_dict_set tune error");
        }
        iRet = av_dict_set(&codecOptions, "profile", "high", 0);
        if (0 != iRet) {
            LOG("av_dict_set profile error");
        }
    }

    // 4、打开解码器
    iRet = avcodec_open2(pCodecCtx, pEncoder, &codecOptions);
    if (iRet < 0) {
        LOG("avcodec_open2 error, return code = %d", iRet);
        return -1;
    }

    // 5、初始化AVFrame
    AVFrame *pFrame = av_frame_alloc();
    pFrame->width = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;
    pFrame->format = pCodecCtx->pix_fmt;

    // 为AVFrame增加引用计数
    iRet = av_frame_get_buffer(pFrame, 0);

    // 计算一帧都大小
    int iFrameSize = av_image_get_buffer_size((AVPixelFormat) pFrame->format,
                                              pFrame->width,
                                              pFrame->height,
                                              1);

    LOG("iFrame size = %d", iFrameSize);
    int iCalSize = (iWidth * iHeight * 3) >> 1;
    assert(iFrameSize == iCalSize);

    AVPacket *pkt = av_packet_alloc();
    uint8_t *yuv_buffer = new uint8_t[iFrameSize];

    LOG("Start encode");

    int64_t begin_time = get_time();
    int64_t end_time = begin_time;
    int64_t all_begin_time = get_time();
    int64_t all_end_time = all_begin_time;
    int64_t base = 0;

    while (true) {
        memset(yuv_buffer, 0, iFrameSize);
        size_t read_bytes = fread(yuv_buffer, 1, iFrameSize, yuv_fd);
        if (read_bytes <= 0) {
            LOG("read file finish");
            break;
        }

        /* 确保该frame可写, 如果编码器内部保持了内存参考计数，则需要重新拷贝一个备份
            目的是新写入的数据和编码器保存的数据不能产生冲突
        */
        bool bWrite = false;
        if (0 == av_frame_is_writable(pFrame)) {
            LOG("Frame can not write, buf %p", pFrame->buf[0]);
            if (pFrame->buf && pFrame->buf[0]) {
                // 打印引用计数，必须保证传入的是有效指针
                LOG("ref count = %d", av_buffer_get_ref_count(pFrame->buf[0]));
            }
        }
        if (0 == av_frame_make_writable(pFrame)) {
            bWrite = true;
        }
        if (bWrite) {
            int iNeedSize = av_image_fill_arrays(pFrame->data, pFrame->linesize, yuv_buffer,
                                                 (AVPixelFormat) pFrame->format,
                                                 pFrame->width, pFrame->height, 1);
            if (iNeedSize != iFrameSize) {
                LOG("need size != frame size");
                break;
            }

            pFrame->pts = ++base;
            begin_time = get_time();
            // 6、编码
            iRet = encode(pCodecCtx, pFrame, pkt, h264_fd);
            end_time = get_time();
            printf("encode time:%lldms\n", end_time - begin_time);
            LOG("encode time = %lld ms", end_time - begin_time);
            if (iRet < 0) {
                LOG("encode fail");
                break;
            }
        }
    }

    // 冲刷编码器
    encode(pCodecCtx, nullptr, pkt, h264_fd);
    all_end_time = get_time();
    LOG("all encode time = %lld ms", all_end_time - all_begin_time);

    // 关闭文件
    if (yuv_fd) {
        fclose(yuv_fd);
    }
    if (h264_fd) {
        fclose(h264_fd);
    }

    // 7、释放内存
    if (yuv_buffer) {
        delete[] yuv_buffer;
        yuv_buffer = nullptr;
    }

    av_frame_free(&pFrame);
    av_packet_free(&pkt);
    avcodec_free_context(&pCodecCtx);

    LOG("yuv to h264 end!");
    return 0;
}
