#pragma once
#include "iostream"
extern "C"
{
#include "libavcodec/avcodec.h"
}
class DecodeVideo
{
	int decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, FILE* f);
public:
	int decode_video(std::string input_filename,std::string output_filename);
};

