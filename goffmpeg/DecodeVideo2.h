#pragma once
#include "iostream"
extern "C"
{
#include "libavcodec/avcodec.h"
}
class DecodeVideo2
{
public:
	int decode_video2(std::string input_filename, std::string output_filename);
	int decode(AVCodecContext* ctx, AVPacket* pkt, AVFrame* frame, FILE* f);
};

