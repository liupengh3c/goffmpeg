#include "DecodeVideo.h"
extern "C"
{
#include "libavcodec/avcodec.h"
}
/*
	�������Խ���h264�ļ�Ϊ����������yuv�ļ�����·��
*/
int DecodeVideo::decode_video(std::string input_filename, std::string output_filename)
{
	int ret = 0;

	AVCodec* codec = NULL;
	AVPacket *pkt = NULL;
	AVFrame* frame = NULL;

	FILE* f = NULL;

	pkt = av_packet_alloc();

	// 1���ҽ�����
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);

	av_parser_init()
}
