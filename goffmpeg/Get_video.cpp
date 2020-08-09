
#pragma warning(disable : 4996);
#include "Get_video.h"

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
AVFrame* frame = NULL, * yuv_frame = NULL;
struct SwsContext* sws_ctx = NULL;

//SDL---------------------------
SDL_Window* screen;
SDL_Renderer* sdlRenderer;
SDL_Texture* sdlTexture;
SDL_Rect sdlRect;

// 注意，必需定义为static类型，不同的文件中decode重名，会报错
static int decode(AVCodecContext* ctx, AVPacket* pkt, AVFrame* frame, FILE* f)
{
    int ret = 0;
    ret = avcodec_send_packet(ctx, pkt);

    if (ret < 0)
    {
        return -1;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if (ret < 0)
        {
            return -2;
        }
        // yuyv422 转 yuv420p
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, yuv_frame->data, yuv_frame->linesize);

        // 写Y分量
        for (size_t i = 0; i < yuv_frame->height; i++)
        {
            fwrite(yuv_frame->data[0] + yuv_frame->linesize[0] * i, 1, yuv_frame->width, f);
        }

        // 写U分量
        for (size_t i = 0; i < yuv_frame->height / 2; i++)
        {
            fwrite(yuv_frame->data[1] + yuv_frame->linesize[1] * i, 1, yuv_frame->width / 2, f);
        }

        // 写V分量
        for (size_t i = 0; i < yuv_frame->height / 2; i++)
        {
            fwrite(yuv_frame->data[2] + yuv_frame->linesize[2] * i, 1, yuv_frame->width / 2, f);
        }

        // 设置纹理数据
        SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
            yuv_frame->data[0], yuv_frame->linesize[0],
            yuv_frame->data[1], yuv_frame->linesize[1],
            yuv_frame->data[2], yuv_frame->linesize[2]);
        // SDL_RenderClear(sdlRenderer);
        // 纹理复制给渲染器
        SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
        // 显示
        SDL_RenderPresent(sdlRenderer);
        // SDL_Delay(40);
    }
    return 0;
}
int Get_video::video(std::string yuv)
{
    int ret = 0;
    int video_index = -1;
    int64_t start_time = 0;
    int dst_width = 640, dst_height = 480;

    AVCodecContext* dec_ctx = NULL;
    AVCodec* decodec = NULL;
    AVFormatContext* fmt_ctx = NULL;
    AVInputFormat* fmt = NULL;
    AVStream* stream = NULL;

    AVPacket* pkt = NULL;

    FILE* f_out = NULL;

    f_out = fopen(yuv.data(), "wb+");
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
    ret = avformat_open_input(&fmt_ctx, "video=B525 HD Webcam", fmt, NULL);
    if (ret < 0)
    {
        std::cout << "avformat_open_input error,ret=" << ret << std::endl;
        return -2;
    }

    // 4. 查找流信息
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0)
    {
        std::cout << "avformat_find_stream_info error,ret=" << ret << std::endl;
        return -3;
    }
    av_dump_format(fmt_ctx, 0, "video=B525 HD Webcam", 0);

    // 5. 获取视频流索引
    for (size_t i = 0; i < fmt_ctx->nb_streams; i++)
    {
        stream = fmt_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
            decodec = avcodec_find_decoder(stream->codecpar->codec_id);
            break;
        }
    }

    if (video_index == -1)
    {
        std::cout << "can not find video index" << std::endl;
        return -4;
    }

    // 6. 分配解码器上下文
    dec_ctx = avcodec_alloc_context3(decodec);
    if (!dec_ctx)
    {
        std::cout << "avcodec_alloc_context3 fail" << std::endl;
        return -5;
    }

    // 7. 复制参数到解码器
    ret = avcodec_parameters_to_context(dec_ctx, stream->codecpar);
    if (ret < 0)
    {
        std::cout << "avcodec_parameters_to_context fail,ret=" << ret << std::endl;
        return -6;
    }

    // 8. 打开解码器
    ret = avcodec_open2(dec_ctx, decodec, NULL);
    if (ret < 0)
    {
        std::cout << "avcodec_open2 fail,ret=" << ret << std::endl;
        return -7;
    }
    std::cout << "stream->codecpar->codec_id=" << stream->codecpar->codec_id << std::endl;
    std::cout << "dec_ctx->height=" << dec_ctx->height << std::endl;
    std::cout << "dec_ctx->width=" << dec_ctx->width << std::endl;

    // 9. 初始化转换器
    sws_ctx = sws_getContext(dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt, dst_width, dst_height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx)
    {
        std::cout << "sws_getContext fail,ret=" << ret << std::endl;
        return -8;
    }

    yuv_frame->width = dst_width;
    yuv_frame->height = dst_height;
    yuv_frame->format = AV_PIX_FMT_YUV420P;
    av_frame_get_buffer(yuv_frame, 0);

    // SDL初始化
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    screen = SDL_CreateWindow("get yuv data from webcam", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        dst_width, dst_height,
        SDL_WINDOW_OPENGL);
    if (!screen)
    {
        std::cout << "sdl create window fail" << std::endl;
        return -9;
    }

    sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, dst_width, dst_height);

    sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = dst_width;
    sdlRect.h = dst_height;

    start_time = av_gettime();

    while (true)
    {
        ret = av_read_frame(fmt_ctx, pkt);
        if (ret < 0)
        {
            break;
        }
        ret = decode(dec_ctx, pkt, frame, f_out);
        if (ret < 0)
        {
            break;
        }
        if (av_gettime() - start_time >= 10000000)
        {
            break;
        }
        //std::cout << "pkt->pts=" << pkt->pts << std::endl;
        av_packet_unref(pkt);
    }
    decode(dec_ctx, NULL, frame, f_out);
    sws_freeContext(sws_ctx);

    av_packet_free(&pkt);
    av_frame_free(&frame);
    av_frame_free(&yuv_frame);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);

    fclose(f_out);

    SDL_Quit();
    return 0;
}
