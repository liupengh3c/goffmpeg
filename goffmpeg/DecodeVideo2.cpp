#pragma warning(disable : 4996);
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
	while (ret >= 0)
	{
		ret = avcodec_receive_frame(ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return 0;
		}
		else if (ret < 0)
		{
			return -2;
		}
		// дY����
		for (size_t i = 0; i < frame->height; i++)
		{
			fwrite(frame->data[0] + frame->linesize[0] * i, 1, frame->width, f);
		}

		// дU����
		for (size_t i = 0; i < frame->height / 2; i++)
		{
			fwrite(frame->data[1] + frame->linesize[1] * i, 1, frame->width / 2, f);
		}

		// дV����
		for (size_t i = 0; i < frame->height / 2; i++)
		{
			fwrite(frame->data[2] + frame->linesize[2] * i, 1, frame->width / 2, f);
		}
	}
	return 0;
}
int DecodeVideo2::decode_video2(std::string input_filename, std::string output_filename)
{
	int ret = 0;
	FILE* f_out = NULL;

	AVFormatContext* ifmt_ctx = NULL, * ofmt_ctx = NULL;
	AVCodec* codec = NULL;
	AVCodecContext* codec_ctx = NULL;

	AVPacket* pkt = NULL;
	AVFrame* frame = NULL;

	f_out = fopen(output_filename.data(), "wb+");

	pkt = av_packet_alloc();
	
	frame = av_frame_alloc();

	// 1���򿪶�ý���ļ�
	ret = avformat_open_input(&ifmt_ctx, input_filename.data(), NULL, NULL);
	if (ret < 0)
	{
		std::cout << "avformat_open_input error,ret=" << ret << std::endl;
		return -4;
	}

	ret = avformat_find_stream_info(ifmt_ctx, NULL);
	if (ret < 0)
	{
		std::cout << "avformat_find_stream_info error,ret=" << ret << std::endl;
		return -5;
	}
	av_dump_format(ifmt_ctx, 0, input_filename.data(), 0);


	// 2��find������
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (codec == NULL)
	{
		std::cout << "avcodec_find_decoder(AV_CODEC_ID_H264) error" << std::endl;
		return -1;
	}


	// 3�����������������
	codec_ctx = avcodec_alloc_context3(codec);
	if (codec_ctx == NULL)
	{
		std::cout << "avcodec_alloc_context3 error" << std::endl;
		return -2;
	}

	// 4����ȡ���������֧��mp4/H264�ļ�,�⼸�д���ǳ���Ҫ�������ܽ���mp4��yuv
	for (size_t i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			avcodec_parameters_to_context(codec_ctx, ifmt_ctx->streams[i]->codecpar);
			break;
		}
	}


	// 5��open������
	ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret < 0)
	{
		std::cout << "avcodec_open2 error,ret=" << ret << std::endl;
		return -3;
	}

	while (av_read_frame(ifmt_ctx, pkt) >= 0)
	{
		// �������Ƶ
		if (ifmt_ctx->streams[pkt->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ret = decode(codec_ctx, pkt, frame, f_out);
			av_packet_unref(pkt);
			if (ret < 0)
			{
				break;
			}
		}
	}
	std::cout << "decoded success-------" << std::endl;

	avcodec_free_context(&codec_ctx);
	avformat_free_context(ifmt_ctx);

	av_packet_free(&pkt);
	av_frame_free(&frame);
	fclose(f_out);

	return 0;
}
