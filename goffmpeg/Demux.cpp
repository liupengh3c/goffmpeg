#include "Demux.h"
#include "iostream"
extern "C"
{
#include "libavformat/avformat.h"
}

int Demux::demux(std::string filename)
{
	int ret;
	int video_index= -1, audio_index = -1;

	std::string h264_file = "";
	std::string aac_file = "";
	const char* mp4 = filename.data();

	AVFormatContext* ifmt_ctx = NULL,*h264_fmt_ctx=NULL,*aac_fmt_ctx=NULL;
	AVStream* in_stream = NULL,*out_stream = NULL;
	AVPacket avpacket;

	h264_file.append(filename).append(".h264");
	aac_file.append(filename).append(".aac");

	// 1����mp4�ļ�
	avformat_open_input(&ifmt_ctx, mp4, NULL, NULL);

	// 2����ȡ����Ϣ
	avformat_find_stream_info(ifmt_ctx, NULL);
	av_dump_format(ifmt_ctx, 0, mp4, 0);

	// 3��h264�ļ�����avformatcontext
	ret = avformat_alloc_output_context2(&h264_fmt_ctx, NULL, NULL, h264_file.data());

	// 4��aac�ļ�����avformatcontext
	ret = avformat_alloc_output_context2(&aac_fmt_ctx, NULL, NULL, aac_file.data());

	// 5���ֱ�Ϊh264��aac�ļ������
	for (size_t i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		in_stream = ifmt_ctx->streams[i];
		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			out_stream = avformat_new_stream(h264_fmt_ctx, NULL);
			video_index = i;
		}
		if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			out_stream = avformat_new_stream(aac_fmt_ctx, NULL);
			audio_index = i;
		}
		if (out_stream)
		{
			avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
		}
	}

	// ��ӡ����ļ���Ϣ
	std::cout << "--------------------------------------------------" << std::endl;
	av_dump_format(h264_fmt_ctx, 0, h264_file.data(), 1);
	av_dump_format(aac_fmt_ctx, 0, aac_file.data(), 1);

	// 6������Ƶ�ļ�
	ret = avio_open(&h264_fmt_ctx->pb, h264_file.data(), AVIO_FLAG_WRITE);
	if (ret < 0)
	{
		std::cout << "create and init avio context for access h264 file error,err=" << ret;
		return -1;
	}

	// 7������Ƶ�ļ�
	ret = avio_open(&aac_fmt_ctx->pb, aac_file.data(), AVIO_FLAG_WRITE);
	if (ret < 0)
	{
		std::cout << "create and init avio context for access aac file error,err=" << ret;
		return -2;
	}

	// 8����Ƶ�ļ�дheader
	ret = avformat_write_header(h264_fmt_ctx, NULL);
	if (ret < 0)
	{
		std::cout << "write h264 file header error,err=" << ret;
		return -3;
	}

	// 9����Ƶ�ļ�дheader
	ret = avformat_write_header(aac_fmt_ctx, NULL);
	if (ret < 0)
	{
		std::cout << "write aac file header error,err=" << ret;
		return -4;
	}

	// 10����ʼд����
	while (true)
	{
		ret = av_read_frame(ifmt_ctx, &avpacket);
		if (ret < 0)
		{
			break;
		}
		if (avpacket.stream_index == video_index)
		{
			avpacket.stream_index = 0;
			av_interleaved_write_frame(h264_fmt_ctx, &avpacket);
		}
		else if (avpacket.stream_index == audio_index)
		{
			avpacket.stream_index = 0;
			av_interleaved_write_frame(aac_fmt_ctx, &avpacket);
		}
		av_packet_unref(&avpacket);
	}

	// 11��д�ļ�β
	av_write_trailer(h264_fmt_ctx);
	av_write_trailer(aac_fmt_ctx);

	// 12���ر����롢����ļ�,�ͷ���Դ
	avformat_close_input(&ifmt_ctx);
	avio_close(h264_fmt_ctx->pb);
	avio_close(aac_fmt_ctx->pb);

	avformat_free_context(h264_fmt_ctx);
	avformat_free_context(aac_fmt_ctx);
	return 0;
}