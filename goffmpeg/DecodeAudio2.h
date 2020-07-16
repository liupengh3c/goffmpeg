#pragma once
#include "iostream"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
}
class DecodeAudio2
{
public:
	int decode_audio2(std::string input_filename, std::string output_filename);
	int decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, SwrContext* swr_ctx, FILE* outfile);
};

