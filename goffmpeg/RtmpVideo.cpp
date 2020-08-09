#pragma warning(disable : 4996);
#include "RtmpVideo.h"
#include "iostream"

#include "SDL2/SDL.h"
extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"

#include <libavutil/imgutils.h>
}
struct SDL_Info
{
    //SDL---------------------------
    SDL_Window* screen;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* sdlTexture;
    SDL_Rect sdlRect;
};
AVStream* istream = NULL, * ostream = NULL;
int RtmpVideo::encode(AVCodecContext* enc_ctx, AVPacket* pkt, AVFrame* frame, AVFormatContext* fmt, int index, FILE* f)
{
    int ret = 0;

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0)
    {
        std::cout << "avcodec_send_frame error,ret=" << ret << std::endl;
        return -1;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if (ret < 0)
        {
            std::cout << "avcodec_receive_packet error,ret=" << ret << std::endl;
            return -2;
        }

        // fwrite(pkt->data, 1, pkt->size, f);
        // pkt->stream_index = 0;
        pkt->pts = av_rescale_q(index, { 1, 30 }, ostream->time_base);
        pkt->dts = pkt->pts;

        std::cout << "index = " << fmt->streams[0]->codecpar->width << std::endl;
        // std::cout << "ostream->time_base.den = " << ostream->time_base.den << std::endl;
        // std::cout << "ostream->time_base.num = " << ostream->time_base.num << std::endl;
        std::cout << "pkt->pts = " << pkt->pts << std::endl;
        ret = av_interleaved_write_frame(fmt, pkt);
        av_packet_unref(pkt);
        if (ret < 0)
        {
            std::cout << "av_interleaved_write_frame error,ret=" << ret << std::endl;
        }
    }
    return 0;
}
int RtmpVideo::rtmp_video(std::string flv)
{
    int ret = 0;
    int video_index = -1;
    int64_t start_time = 0;
    int dst_width = 640, dst_height = 480;

    AVFormatContext* ifmt_ctx = NULL;
    AVInputFormat* fmt = NULL;

    AVFormatContext* ofmt_ctx = NULL;

    AVPacket* pkt = NULL;

    FILE* f_out = NULL;

    AVFrame* frame = NULL, * yuv_frame = NULL;
    struct SwsContext* sws_ctx = NULL;
    struct SDL_Info display;

    const char* rtmp_server = "rtmp://106.13.105.231:8234/liupeng/video";
    AVCodecContext* enc_ctx = NULL;
    AVCodec* codec = NULL;

    f_out = fopen(flv.data(), "wb+");
    pkt = av_packet_alloc();
    frame = av_frame_alloc();
    yuv_frame = av_frame_alloc();

    // 1. 注册所有输入/输出设备，必须步骤
    avdevice_register_all();

    // 2. find input format
    fmt = av_find_input_format("dshow");
    if (!fmt)
    {
        std::cout << "find input format error" << std::endl;
        return -1;
    }

    // 3. 打开摄像头
    ret = avformat_open_input(&ifmt_ctx, "video=B525 HD Webcam", fmt, NULL);
    if (ret < 0)
    {
        std::cout << "avformat_open_input error,ret=" << ret << std::endl;
        return -2;
    }

    // 4. 查找流信息
    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0)
    {
        std::cout << "avformat_find_stream_info error,ret=" << ret << std::endl;
        return -3;
    }
    av_dump_format(ifmt_ctx, 0, "video=B525 HD Webcam", 0);

    // 为输出文件分配avformat_context
    avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", rtmp_server);
    if (!ofmt_ctx)
    {
        std::cout << "avformat_alloc_output_context2 for output file error,ret=" << std::endl;
        return -4;
    }

    codec = avcodec_find_encoder_by_name("h264_nvenc");
    if (!codec)
    {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec)
        {
            std::cout << "avcodec_find_encoder_by_name h264_nvenc fail" << std::endl;
            return -7;
        }
        enc_ctx->codec_id = AV_CODEC_ID_H264;
    }

    enc_ctx = avcodec_alloc_context3(codec);
    if (!enc_ctx)
    {
        std::cout << "avcodec_alloc_context3 fail" << std::endl;
        return -8;
    }

    enc_ctx->bit_rate = 4000000;
    enc_ctx->width = dst_width;
    enc_ctx->height = dst_height;
    enc_ctx->time_base = AVRational{ 1, 30 };
    enc_ctx->gop_size = 12;
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    // 5. 获取视频流索引
    for (size_t i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        istream = ifmt_ctx->streams[i];
        ostream = avformat_new_stream(ofmt_ctx, NULL);
        ostream->id = 0;
        if (istream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
            avcodec_parameters_from_context(ostream->codecpar, enc_ctx);
            ostream->avg_frame_rate = AVRational{ 30, 1 };
            ostream->time_base = AVRational{ 1, 30 };
            ostream->codec = enc_ctx;
            break;
        }
    }


    if (video_index == -1)
    {
        std::cout << "can not find video index" << std::endl;
        return -9;
    }
    std::cout << "-----------------------------------\n"
        << std::endl;
    av_dump_format(ofmt_ctx, 0, rtmp_server, 1);

    // 8. 打开解码器
    ret = avcodec_open2(enc_ctx, codec, NULL);
    if (ret < 0)
    {
        std::cout << "avcodec_open2 fail,ret=" << ret << std::endl;
        return -10;
    }
    // 打开输出文件
    ret = avio_open(&ofmt_ctx->pb, rtmp_server, AVIO_FLAG_WRITE);
    if (ret < 0)
    {
        std::cout << "open output file error,ret=" << ret << std::endl;
        return -5;
    }

    printf("ofmt_ctx->priv_data:%v\n", ofmt_ctx->priv_data);


    // 写文件头
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0)
    {
        std::cout << "avformat_write_header to file error,ret=" << ret << std::endl;
        return -6;
    }
    // 9. 初始化转换器
    sws_ctx = sws_getContext(istream->codecpar->width, istream->codecpar->height, AVPixelFormat(istream->codecpar->format), dst_width, dst_height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx)
    {
        std::cout << "sws_getContext fail,ret=" << ret << std::endl;
        return -11;
    }

    yuv_frame->width = dst_width;
    yuv_frame->height = dst_height;
    yuv_frame->format = AV_PIX_FMT_YUV420P;
    av_frame_get_buffer(yuv_frame, 0);

    frame->width = istream->codecpar->width;
    frame->height = istream->codecpar->height;
    frame->format = istream->codecpar->format;
    av_frame_get_buffer(frame, 0);

    // SDL初始化
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    display.screen = SDL_CreateWindow("get yuv data from webcam", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        dst_width, dst_height,
        SDL_WINDOW_OPENGL);
    if (!display.screen)
    {
        std::cout << "sdl create window fail" << std::endl;
        return -9;
    }

    display.sdlRenderer = SDL_CreateRenderer(display.screen, -1, 0);
    display.sdlTexture = SDL_CreateTexture(display.sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, dst_width, dst_height);

    display.sdlRect.x = 0;
    display.sdlRect.y = 0;
    display.sdlRect.w = dst_width;
    display.sdlRect.h = dst_height;

    AVPacket* opkt = NULL;
    opkt = av_packet_alloc();
    start_time = av_gettime();
    int frame_index = 0;

    while (true)
    {
        ret = av_read_frame(ifmt_ctx, pkt);
        if (ret < 0)
        {
            break;
        }
        memcpy(frame->data[0], pkt->data, pkt->size);
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, yuv_frame->data, yuv_frame->linesize);
        yuv_frame->pts = av_rescale_q(pkt->pts, istream->time_base, ostream->time_base);
        encode(enc_ctx, opkt, yuv_frame, ofmt_ctx, frame_index, f_out);
        frame_index++;
        // 设置纹理数据
        SDL_UpdateYUVTexture(display.sdlTexture, &display.sdlRect,
            yuv_frame->data[0], yuv_frame->linesize[0],
            yuv_frame->data[1], yuv_frame->linesize[1],
            yuv_frame->data[2], yuv_frame->linesize[2]);
        // SDL_RenderClear(sdlRenderer);
        // 纹理复制给渲染器
        SDL_RenderCopy(display.sdlRenderer, display.sdlTexture, NULL, &display.sdlRect);
        // 显示
        SDL_RenderPresent(display.sdlRenderer);
        if (av_gettime() - start_time >= 10000000)
        {
            break;
        }
        // AVRational time_base = istream->time_base;
        // AVRational time_base_q = {1, AV_TIME_BASE};
        // int64_t pts_time = av_rescale_q(pkt->pts, time_base, time_base_q);
        // int64_t now_time = av_gettime() - start_time;
        // if (pts_time > now_time)
        //     av_usleep(pts_time - now_time);

        // std::cout << "pts_time=" << pts_time << std::endl;
        // std::cout << "pkt->dts=" << pkt->dts << std::endl;
        // std::cout << "pkt->pts=" << pkt->pts << std::endl;
        // std::cout << "av_gettime() - start_time=" << av_gettime() - start_time << std::endl;
        // std::cout << "istream->time_base.den=" << istream->time_base.den << std::endl;
        // std::cout << "istream->time_base.num=" << istream->time_base.num << std::endl;
    }

    encode(enc_ctx, opkt, NULL, ofmt_ctx, frame_index, f_out);
    av_write_trailer(ofmt_ctx);

    sws_freeContext(sws_ctx);

    av_packet_free(&pkt);
    av_packet_free(&opkt);
    av_frame_free(&frame);
    av_frame_free(&yuv_frame);
    avcodec_free_context(&enc_ctx);
    avformat_close_input(&ifmt_ctx);

    avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    fclose(f_out);

    SDL_Quit();
    return 0;
}
