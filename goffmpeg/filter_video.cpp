#include "filter_video.h"
#include "iostream"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

int init_filters(char* desc, AVStream* stream, AVFormatContext* fmt, AVFilterContext *buffersrc, AVFilterContext *buffersink)
{
	char args[512];
	int ret = 0;

	// ��������filter��һ��ͷ��һ��β
	const AVFilter* buffersrc = avfilter_get_by_name("buffer");
	const AVFilter* buffersink = avfilter_get_by_name("buffersink");
}
int filter_video::filter_videos(std::string file)
{
	int ret = 0;
	AVFormatContext* fmt_ctx = NULL;
	AVCodecContext* dec_ctx = NULL;
	AVCodec* dec = NULL;

	AVFilterContext* buffersink_ctx;
	AVFilterContext* buffersrc_ctx;
	AVFilterGraph* filter_graph;

	int video_stream_index = -1;

	// 1.��������Ƶ�ļ�
	ret = avformat_open_input(&fmt_ctx, file.data(), NULL, NULL);
	if (ret < 0)
	{
		std::cout << "open input file error,ret=" << ret << std::endl;
		return -1;
	}

	// 2.���Ҷ�ý���ļ�����Ϣ
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0)
	{
		std::cout << "find stream info from input file error,ret=" << ret << std::endl;
		return -2;
	}

	// 3.�ҵ���Ƶ������
	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
	if (ret < 0)
	{
		std::cout << "find best stream from input file error,ret=" << ret << std::endl;
		return -3;
	}
	video_stream_index = ret;

	// 4.�������������
	dec_ctx = avcodec_alloc_context3(dec);
	if (!dec_ctx)
	{
		std::cout << "alloc dec ctx error" << std::endl;
		return -4;
	}

	// 5.���ý����������������и���
	ret = avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);
	if (ret < 0)
	{
		std::cout << "avcodec_parameters_to_context error,ret=" << ret << std::endl;
		return -5;
	}

	// 6.��ʼ��������������
	ret = avcodec_open2(dec_ctx, dec, NULL);
	if (ret < 0)
	{
		std::cout << "init dec context error,ret=" << ret << std::endl;
		return -6;
	}
	return 0;
}