#pragma once
#include "iostream"
extern "C"
{
#include "libavcodec/avcodec.h"
}
class Encode_video
{
public:
	int encode_video(std::string input_file, std::string output_file);
	int encode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, FILE* outfile);
};

