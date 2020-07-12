#include "DecodeVideo2.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
int decode(AVCodecContext* ctx, AVPacket* pkt, AVFrame* frame, FILE* f)
{
	int ret = 0;
	ret = avcodec_send_packet(ctx, pkt);
	if (ret < 0)
	{
		return -1;
	}

}
int DecodeVideo2::decode_video2(std::string filename)
{

}
