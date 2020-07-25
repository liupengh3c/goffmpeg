#pragma warning(disable : 4996);
#include "Encode_audio.h"
/*
本示例程序编码pcm为aac
ffmpeg不支持直接编码s16，,只能编码fltp，如果从pcm文件读取数据，pcm文件一般都是s16，则需要使用重采样器进行转码
*/


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
}


int Encode_audio::encode(AVCodecContext* enc_ctx, AVPacket* pkt, AVFrame* frame, AVFormatContext *fmt)
{
	int ret = 0;

	ret = avcodec_send_frame(enc_ctx, frame);
	if (ret < 0)
	{
		std::cout << "avcodec_send_frame error,ret=" << ret << std::endl;
		return -1;
	}
	while (ret >= 0)
	{
		ret = avcodec_receive_packet(enc_ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return 0;
		}
		else if (ret < 0)
		{
			return -2;
		}
		std::cout << "Write packet " << pkt->pts << " size=" << pkt->size << std::endl;
		pkt->stream_index = 0;
		av_interleaved_write_frame(fmt, pkt);
		av_packet_unref(pkt);
	}
	return 0;
}

int Encode_audio::encode_audio(std::string input_file, std::string output_file)
{
	int ret = 0;
	uint8_t samples[4096];

	FILE* f_in = NULL;

	AVFormatContext* fmt_ctx = NULL;

	AVCodec* codec = NULL;
	AVCodecContext* enc_ctx = NULL;

	AVFrame* frame = NULL;
	AVPacket* pkt = NULL;

	SwrContext* swr_ctx = NULL;
	AVStream* stream = NULL;

	uint8_t** dst_data;
	int dst_linesize;

	f_in = fopen(input_file.data(), "rb");

	frame = av_frame_alloc();
	pkt = av_packet_alloc();

	// 1. 输出文件分配avformatcontext
	ret = avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, output_file.data());
	if (ret < 0)
	{
		std::cout << "alloc avformatcontext for output file error,ret=" << ret << std::endl;
		return -1;
	}

	// 2. 打开输出文件
	ret = avio_open(&fmt_ctx->pb, output_file.data(), AVIO_FLAG_WRITE);
	if (ret < 0)
	{
		std::cout << "open the output file error,ret=" << ret << std::endl;
		return -2;
	}


	// 3. find encoder
	codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!codec)
	{
		std::cout << "find encoder aac error" << std::endl;
		return -3;
	}

	// 4. 分配编码上下文
	enc_ctx = avcodec_alloc_context3(codec);
	if (!enc_ctx)
	{
		std::cout << "alloc context error" << std::endl;
		return -4;
	}
	
	// ffmpeg 编码aac，不支持直接编码 packet数据，因此从pcm文件编码必须使用重采样器转码 s16->fltp
	enc_ctx->bit_rate = 128000;
	enc_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	enc_ctx->sample_rate = 48000;
	enc_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
	enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
	enc_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
	

	// 5. 打开编码器
	ret = avcodec_open2(enc_ctx, codec, NULL);
	if (ret < 0)
	{
		std::cout << "encoder open fail,ret = " << ret << std::endl;
		return -5;
	}

	// 6. 添加流,并复制参数，必须将编码器参数复制到stream
	stream = avformat_new_stream(fmt_ctx, NULL);
	ret = avcodec_parameters_from_context(stream->codecpar, enc_ctx);
	if (ret < 0)
	{
		std::cout << "copy param to stream from avcodec context,err=" << ret;
		return -6;
	}

	// 7. 写header
	ret = avformat_write_header(fmt_ctx, NULL);
	if (ret < 0)
	{
		std::cout << "write aac file header error,err=" << ret;
		return -7;
	}


	// 8. 初始化重采样器
	swr_ctx = swr_alloc_set_opts(NULL, enc_ctx->channel_layout, enc_ctx->sample_fmt, enc_ctx->sample_rate, enc_ctx->channel_layout, AV_SAMPLE_FMT_S16, enc_ctx->sample_rate, 0, NULL);
	ret = swr_init(swr_ctx);
	if (ret < 0)
	{
		std::cout << "swr_init swr_ctx error,ret=" << ret << std::endl;
		return -8;
	}
	// 9. 申请空间，用于存储pcm数据
	frame->format = enc_ctx->sample_fmt;
	frame->nb_samples = enc_ctx->frame_size;
	frame->channel_layout = enc_ctx->channel_layout;
	av_frame_get_buffer(frame, 0);

	

	// 10. 读取文件、编码
	while (!feof(f_in))
	{
		ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, av_get_channel_layout_nb_channels(frame->channel_layout), frame->nb_samples, AVSampleFormat(AV_SAMPLE_FMT_S16), 0);
		dst_data[0] = samples;
		int unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(AVSampleFormat(frame->format));
		ret = fread(samples, 1, unpadded_linesize, f_in);
		ret = swr_convert(swr_ctx, (uint8_t**)frame->data, frame->nb_samples, (const uint8_t**)dst_data, frame->nb_samples);
		encode(enc_ctx, pkt, frame, fmt_ctx);
	}

	// 11. flush the encoder
	encode(enc_ctx, pkt, NULL, fmt_ctx);

	av_write_trailer(fmt_ctx);

	// 12. 释放资源
	fclose(f_in);
	avio_close(fmt_ctx->pb);
	avformat_free_context(fmt_ctx);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&enc_ctx);

	return 0;
}