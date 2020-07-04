#include "DecodeVideo.h"
extern "C"
{
#include "libavcodec/avcodec.h"
}
/*
	本函数以解码h264文件为例，解码后的yuv文件生成路径
*/
int DecodeVideo::decode_video(std::string input_filename, std::string output_filename)
{
	int ret = 0;

	AVCodec* codec = NULL;
	AVPacket *pkt = NULL;
	AVFrame* frame = NULL;

	FILE* f = NULL;

	pkt = av_packet_alloc();

	// 1、找解码器
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);

	av_parser_init()
}
