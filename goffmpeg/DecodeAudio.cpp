#pragma warning(disable : 4996);
#include "DecodeAudio.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}
#define AUDIO_INBUF_SIZE 2048
#define AUDIO_REFILL_THRESH 4096

int  DecodeAudio::decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, SwrContext *swr_ctx, FILE* outfile)
{
	int ret = 0, data_size = 0;
	uint8_t** dst_data;
	int dst_linesize;
	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0)
	{
		return -1;
	}
	while (ret >= 0)
	{
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret < 0)
		{
			return -2;
		}
		else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return 0;
		}

		data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
		// 计算每帧采样所需字节数
		int unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(AVSampleFormat(frame->format));
		av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, av_get_channel_layout_nb_channels(frame->channel_layout), frame->nb_samples, AVSampleFormat(frame->format), 0);

		swr_convert(swr_ctx, dst_data, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
		fwrite(dst_data[0], 1, unpadded_linesize, outfile);

	}
	return 0;
}

int DecodeAudio::decode_audio(std::string input_filename, std::string output_filename)
{
	int ret = 0;
	uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	uint8_t* data;
	size_t data_size;

	FILE* f_in = NULL, * f_out = NULL;

	AVCodec* codec = NULL;
	AVCodecContext* codec_ctx = NULL;
	
	AVFormatContext* fmt_ctx = NULL;

	AVPacket* pkt = NULL;
	AVFrame* frame = NULL;

	AVCodecParserContext* parser = NULL;
	SwrContext* swr_ctx = NULL;

	pkt = av_packet_alloc();
	frame = av_frame_alloc();
	//inbuf = (uint8_t *) malloc(AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);
	//memset(inbuf, 0, AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);

	// 1.find 解码器，如果是aac的，传参AV_CODEC_ID_AAC
	codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
	if (!codec)
	{
		std::cout << "avcodec_find_decoder error" << std::endl;
		return -1;
	}

	// 2.初始化解析器上下文
	parser = av_parser_init(codec->id);
	if (!parser)
	{
		std::cout << "av_parser_init error" << std::endl;
		return -2;
	}


	// 3.allocate decode context
	codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx)
	{
		std::cout << "avcodec_alloc_context3 error,ret=" << ret << std::endl;
		return -3;
	}

	// 4.打开编码器
	ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret < 0)
	{
		std::cout << "avcodec_open2 error,ret=" << ret << std::endl;
		return -4;
	}

	// 5.打开输入、输出文件
	f_in = fopen(input_filename.data(), "rb");
	f_out = fopen(output_filename.data(), "wb+");

	// 6. 获取输入流信息并读取文件header
	ret = avformat_open_input(&fmt_ctx, input_filename.data(), NULL, NULL);
	if (ret < 0)
	{
		std::cout << "avformat_open_input error,ret=" << ret << std::endl;
		return -5;
	}
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0)
	{
		std::cout << "avformat_find_stream_info error,ret=" << ret << std::endl;
		return -6;
	}
	for (size_t i = 0; i < fmt_ctx->nb_streams; i++)
	{
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[i]->codecpar);
			break;
		}
	}
	// 6.初始化重采样器
	swr_ctx = swr_alloc_set_opts(NULL, codec_ctx->channel_layout, AV_SAMPLE_FMT_S16, codec_ctx->sample_rate, codec_ctx->channel_layout, AV_SAMPLE_FMT_FLTP, codec_ctx->sample_rate, 0, NULL);
	ret = swr_init(swr_ctx);
	if (ret < 0)
	{
		std::cout << "swr_init swr_ctx error,ret=" << ret << std::endl;
		return -4;
	}

	while (!feof(f_in))
	{
		data_size = fread(inbuf, 1, AUDIO_INBUF_SIZE, f_in);
		if (data_size <= 0)
		{
			break;
		}
		data = inbuf;

		while (data_size > 0)
		{
			ret = av_parser_parse2(parser, codec_ctx, &pkt->data, &pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0)
			{
				std::cout << "av_parser_parse2 error,ret=" << ret << std::endl;
				return -5;
			}
			data += ret;
			data_size -= ret;
			if (pkt->size)
			{
				decode(codec_ctx, pkt, frame, swr_ctx,f_out);
			}
			av_packet_unref(pkt);
		}
	}


	pkt->data = NULL;
	pkt->size = 0;
	decode(codec_ctx, pkt, frame, swr_ctx,f_out);

	fclose(f_in);
	fclose(f_out);
	av_parser_close(parser);
	avcodec_free_context(&codec_ctx);
	av_frame_free(&frame);
	av_packet_free(&pkt);

	return 0;
}
