#pragma once
#include "iostream"
extern"C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
class Encode_audio
{
public:
	int encode_audio(std::string input_file, std::string output_file);
	int encode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, AVFormatContext* fmt);
};

