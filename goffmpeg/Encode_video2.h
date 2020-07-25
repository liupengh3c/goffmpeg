#pragma once
#include "iostream"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
class Encode_video2
{
public:
	int encode_video(std::string input_file, std::string output_file);
	int encode(AVCodecContext* enc_ctx, AVPacket* pkt, AVFrame* frame, AVFormatContext* fmt);
};

