#pragma warning(disable : 4996);

#include "Encode_video.h"
/*
本示例程序编码yuv420p为h264
*/


extern "C"
{
#include "libavcodec/avcodec.h"
}
#define WIDTH 1280
#define HEIGHT 720
#define FRAME_RATE 25
#define BIT_RATE 2000000
#define PIX_FMT AV_PIX_FMT_YUV420P

int Encode_video::encode(AVCodecContext* enc_ctx, AVPacket* pkt, AVFrame* frame, FILE* outfile)
{
	int ret = 0;
	if (frame)
	{
		std::cout << "send frame " << frame->pts << std::endl;
	}

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
		fwrite(pkt->data, 1, pkt->size, outfile);
		av_packet_unref(pkt);
	}
	return 0;
}
int Encode_video::encode_video(std::string input_file, std::string output_file)
{
	int ret = 0;
	int frame_index = 0;

	FILE* f_in = NULL;
	FILE* f_out = NULL;

	AVCodec* codec = NULL;
	AVCodecContext* enc_ctx = NULL;

	AVFrame* frame = NULL;
	AVPacket* pkt = NULL;

	// 1. find encoder
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec)
	{
		std::cout << "find encoder error" << std::endl;
		return -1;
	}

	// 2. 分配编码器上线问
	enc_ctx = avcodec_alloc_context3(codec);
	if (!enc_ctx)
	{
		std::cout << "avcodec_alloc_context3 error" << std::endl;
		return -2;
	}

	// 3. 设置编码参数，宽、高、比特率、帧率、时间基
	enc_ctx->width = WIDTH;
	enc_ctx->height = HEIGHT;
	enc_ctx->bit_rate = BIT_RATE;

	enc_ctx->pix_fmt = PIX_FMT;

	enc_ctx->framerate = AVRational{ 25,1 };
	enc_ctx->time_base = AVRational{ 1,25 };

	// 4. 打开编码器
	ret = avcodec_open2(enc_ctx, codec, NULL);
	if (ret < 0)
	{
		std::cout << "avcodec_open2 error,ret=" << ret << std::endl;
		return -3;
	}

	// 5. alloc frame and pkt
	frame = av_frame_alloc();
	pkt = av_packet_alloc();

	// 6. 打开yuv文件
	f_in = fopen(input_file.data(), "rb");
	f_out = fopen(output_file.data(), "wb+");

	// 7. 分配存储空间，用于存储读取到的yuv数据
	frame->format = PIX_FMT;
	frame->width = WIDTH;
	frame->height = HEIGHT;
	av_frame_get_buffer(frame, 0);


	while (!feof(f_in))
	{
		ret = av_frame_make_writable(frame);
		if (ret < 0)
		{
			std::cout << "av_frame_make_writable error,ret=" << ret << std::endl;
			return -4;
		}

		// 读Y分量
		for (size_t i = 0; i < frame->height; i++)
		{
			fread(frame->data[0] + frame->linesize[0] * i, 1, frame->width, f_in);
		}

		// 读U分量
		for (size_t i = 0; i < frame->height / 2; i++)
		{
			fread(frame->data[1] + frame->linesize[1] * i, 1, frame->width / 2, f_in);
		}

		// 读U分量
		for (size_t i = 0; i < frame->height / 2; i++)
		{
			fread(frame->data[2] + frame->linesize[2] * i, 1, frame->width / 2, f_in);
		}

		frame->pts = frame_index++;
		encode(enc_ctx, pkt, frame, f_out);
	}

	// 8. flush the encoder
	encode(enc_ctx, pkt, NULL, f_out);


	// 9. 释放资源
	fclose(f_in);
	fclose(f_out);

	avcodec_free_context(&enc_ctx);
	av_frame_free(&frame);
	av_packet_free(&pkt);

	return 0;
}