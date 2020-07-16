#pragma warning(disable : 4996);
#include "DecodeAudio2.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}
int DecodeAudio2::decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, SwrContext* swr_ctx, FILE* outfile)
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
		
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return 0;
		}
		else if (ret < 0)
		{
			std::cout << "avcodec_receive_frame fail,ret=" << ret;
			return -2;
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

int DecodeAudio2::decode_audio2(std::string input_filename, std::string output_filename)
{
	int ret = 0;
	FILE* f_out = NULL;

	AVFormatContext* fmt_ctx = NULL;
	AVFrame* frame = NULL;
	AVPacket* pkt = NULL;

	AVCodec* codec = NULL;
	AVCodecContext* decode_ctx = NULL;

	SwrContext* swr_ctx = NULL;

	frame = av_frame_alloc();
	pkt = av_packet_alloc();

	// 1.打开输入流
	ret = avformat_open_input(&fmt_ctx, input_filename.data(), NULL, NULL);
	if (ret < 0)
	{
		std::cout << "open the input stream error,ret=" << ret;
		return -1;
	}

	// 2.查找流信息
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0)
	{
		std::cout << "can not find the stream info,ret=" << ret;
		return -2;
	}
	av_dump_format(fmt_ctx, 0, input_filename.data(), 0);

	// 3.查找解码器
	codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
	if (!codec)
	{
		std::cout << "find decoder error";
		return -3;
	}

	// 4.alloc 解码器上下文
	decode_ctx = avcodec_alloc_context3(codec);
	if (!decode_ctx)
	{
		std::cout << "alloc decode context error";
		return -4;
	}
	// 5.从流信息中复制参数到解码器
	for (size_t i = 0; i < fmt_ctx->nb_streams; i++)
	{
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			avcodec_parameters_to_context(decode_ctx, fmt_ctx->streams[i]->codecpar);
			break;
		}
	}

	// 6.打开解码器
	ret = avcodec_open2(decode_ctx, codec, NULL);
	if (ret < 0)
	{
		std::cout << "open codec error,ret=" << ret;
		return -5;
	}

	// 7. 打开输出文件
	f_out = fopen(output_filename.data(), "wb+");

	// 8.初始化采样器
	swr_ctx = swr_alloc_set_opts(NULL, decode_ctx->channel_layout, AV_SAMPLE_FMT_S16, decode_ctx->sample_rate, decode_ctx->channel_layout, decode_ctx->sample_fmt, decode_ctx->sample_rate, 0, NULL);
	if (!swr_ctx)
	{
		std::cout << "swr_alloc_set_opts error";
		return -6;
	}

	ret = swr_init(swr_ctx);
	if (ret < 0)
	{
		std::cout << "swr_ctx init error,ret=" << ret;
		return -7;
	}

	// 9. 解码
	while (true)
	{
		ret = av_read_frame(fmt_ctx, pkt);
		if (ret < 0)
		{
			break;
		}
		// if stream is audio
		if (fmt_ctx->streams[pkt->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			ret = decode(decode_ctx, pkt, frame, swr_ctx, f_out);
			if (ret < 0)
			{
				break;
			}
			av_packet_unref(pkt);
		}
	}

	// flush the decoder
	decode(decode_ctx, NULL, frame, swr_ctx, f_out);

	// 释放资源
	fclose(f_out);
	avformat_close_input(&fmt_ctx);
	avcodec_free_context(&decode_ctx);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	swr_free(&swr_ctx);

	return 0;
}