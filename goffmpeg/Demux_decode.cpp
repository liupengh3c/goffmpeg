#pragma warning(disable : 4996);

#include "Demux_decode.h"
#include "iostream"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}
int Demux_decode::decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, SwrContext* swr_ctx, FILE* audio_file, FILE *video_file)
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
			return -2;
		}
		if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
			// 计算每帧采样所需字节数
			int unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(AVSampleFormat(frame->format));
			av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, av_get_channel_layout_nb_channels(frame->channel_layout), frame->nb_samples, AVSampleFormat(frame->format), 0);

			swr_convert(swr_ctx, dst_data, frame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
			fwrite(dst_data[0], 1, unpadded_linesize, audio_file);
		}
		else
		{
			// 写Y分量
			for (size_t i = 0; i < frame->height; i++)
			{
				fwrite(frame->data[0] + frame->linesize[0] * i, 1, frame->width, video_file);
			}

			// 写U分量
			for (size_t i = 0; i < frame->height / 2; i++)
			{
				fwrite(frame->data[1] + frame->linesize[1] * i, 1, frame->width / 2, video_file);
			}

			// 写V分量
			for (size_t i = 0; i < frame->height / 2; i++)
			{
				fwrite(frame->data[2] + frame->linesize[2] * i, 1, frame->width / 2, video_file);
			}
		}
	}
	return 0;
}

int Demux_decode::demux_decode(std::string input_file, std::string video_file, std::string audio_file)
{
	int ret = 0;

	FILE* f_audio = NULL;
	FILE* f_video = NULL;
	
	AVCodecContext* dec_video_ctx = NULL, * dec_audio_ctx = NULL;
	AVFormatContext* fmt_ctx = NULL;

	SwrContext* swr_ctx = NULL;

	AVPacket* pkt = NULL;
	AVFrame* frame = NULL;

	// 1. 打开输入流
	ret = avformat_open_input(&fmt_ctx, input_file.data(), NULL, NULL);
	if (ret < 0)
	{
		std::cout << "open input stream error,ret=" << ret;
		return -1;
	}

	// 2. find stream info
	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0)
	{
		std::cout << "find input stream info error,ret=" << ret;
		return -2;
	}
	av_dump_format(fmt_ctx, 0, input_file.data(), 0);

	// 3. 查找解码器 + 复制解码器参数 + 打开解码器
	for (size_t i = 0; i < fmt_ctx->nb_streams; i++)
	{
		AVCodec* codec = NULL;
		AVStream* stream = NULL;

		stream = fmt_ctx->streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			codec = avcodec_find_decoder(stream->codecpar->codec_id);
			if (!codec)
			{
				std::cout << "find decoder for video error";
				return -3;
			}
			// 从流中复制解码器参数到dec_video_ctx
			dec_video_ctx = avcodec_alloc_context3(codec);
			if (!dec_video_ctx)
			{
				std::cout << "avcodec_alloc_context3 for video error";
				return -4;
			}
			ret = avcodec_parameters_to_context(dec_video_ctx, stream->codecpar);
			if (ret < 0)
			{
				std::cout << "avcodec_parameters_to_context for video error";
				return -5;
			}
			ret = avcodec_open2(dec_video_ctx, codec, NULL);
			if (ret < 0)
			{
				std::cout << "avcodec_open2 for video error";
				return -6;
			}
		}
		else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			codec = avcodec_find_decoder(stream->codecpar->codec_id);
			if (!codec)
			{
				std::cout << "find decoder for audio error";
				return -7;
			}
			// 从流中复制解码器参数到dec_audio_ctx
			dec_audio_ctx = avcodec_alloc_context3(codec);
			if (!dec_audio_ctx)
			{
				std::cout << "avcodec_alloc_context3 for audio error";
				return -8;
			}
			ret = avcodec_parameters_to_context(dec_audio_ctx, stream->codecpar);
			if (ret < 0)
			{
				std::cout << "avcodec_parameters_to_context for audio error";
				return -9;
			}
			ret = avcodec_open2(dec_audio_ctx, codec, NULL);
			if (ret < 0)
			{
				std::cout << "avcodec_open2 for audio error";
				return -10;
			}
		}
	}

	// 4. 初始化重采样器
	if (dec_audio_ctx)
	{
		swr_ctx = swr_alloc_set_opts(NULL, dec_audio_ctx->channel_layout, AV_SAMPLE_FMT_S16, dec_audio_ctx->sample_rate, dec_audio_ctx->channel_layout, dec_audio_ctx->sample_fmt, dec_audio_ctx->sample_rate, 0, NULL);
		swr_init(swr_ctx);
	}
	
	// 4. 打开输出文件
	f_video = fopen(video_file.data(), "wb+");
	f_audio = fopen(audio_file.data(), "wb+");

	// 5. alloc frame && pkt
	pkt = av_packet_alloc();
	frame = av_frame_alloc();


	while (true)
	{
		ret = av_read_frame(fmt_ctx, pkt);
		if (ret < 0)
		{
			break;
		}
		// 如果是视频
		if (fmt_ctx->streams[pkt->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ret = decode(dec_video_ctx, pkt, frame, swr_ctx, f_audio, f_video);
			
		}
		else
		{
			ret = decode(dec_audio_ctx, pkt, frame, swr_ctx, f_audio, f_video);
		}
		av_packet_unref(pkt);
		if (ret < 0)
		{
			break;
		}
	}

	// flush decoder
	decode(dec_audio_ctx, NULL, frame, swr_ctx, f_audio, f_video);

	// 释放资源
	avformat_close_input(&fmt_ctx);
	avcodec_free_context(&dec_audio_ctx);
	avcodec_free_context(&dec_video_ctx);

	av_packet_free(&pkt);
	av_frame_free(&frame);

	swr_close(swr_ctx);

	fclose(f_audio);
	fclose(f_video);

	return 0;
}