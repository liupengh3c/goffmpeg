#pragma once
#include "iostream"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
}
class Demux_decode
{
public:
	int decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, SwrContext* swr_ctx, FILE* audio_file, FILE* video_file);
	int demux_decode(std::string input_file, std::string video_file, std::string audio_file);
};

