#pragma once
#include "iostream"
extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"

}
class RtmpVideo
{
public:
	int encode(AVCodecContext* enc_ctx, AVPacket* pkt, AVFrame* frame, AVFormatContext* fmt, int index, FILE* f);
	int rtmp_video(std::string flv);
};

