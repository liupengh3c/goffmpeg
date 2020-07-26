#include "Streamer.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
}
int Streamer::streamer(std::string input)
{
	int ret = 0;
	int frame_index = 0;
	int video_index = -1;

	std::string output = "rtmp://106.13.105.231:8935/live/movie";

	AVFormatContext* ifmt = NULL, * ofmt = NULL;

	AVPacket* pkt = av_packet_alloc();
	AVStream* istream, *ostream = NULL;

	avformat_network_init();

	// 1. 打开输入文件
	ret = avformat_open_input(&ifmt, input.data(), NULL, NULL);
	if (ret < 0)
	{
		std::cout << "open input stream error,ret=" << ret << std::endl;
		return -1;
	}

	// 2. 查找流信息
	ret = avformat_find_stream_info(ifmt, NULL);
	if (ret < 0)
	{
		std::cout << "avformat_find_stream_info error,ret=" << ret << std::endl;
		return -2;
	}
	av_dump_format(ifmt, 0, input.data(), 0);

	// 3. 确定视频流索引
	for (size_t i = 0; i < ifmt->nb_streams; i++)
	{
		if (ifmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_index = i;
			break;
		}
	}

	// 4. ouput
	ret = avformat_alloc_output_context2(&ofmt, NULL, "flv", output.data());
	if (ret < 0)
	{
		std::cout << "avformat_alloc_output_context2 error,ret=" << ret << std::endl;
		return -3;
	}

	// 5. 输出添加流
	for (size_t i = 0; i < ifmt->nb_streams; i++)
	{
		istream = ifmt->streams[i];
		ostream = avformat_new_stream(ofmt, NULL);
		avcodec_parameters_copy(ostream->codecpar, istream->codecpar);
	}
	av_dump_format(ofmt, 0, output.data(), 1);

	// 6. 打开输出文件
	ret = avio_open(&ofmt->pb, output.data(), AVIO_FLAG_WRITE);
	if (ret < 0)
	{
		std::cout << "avio_open output file error,ret=" << ret << std::endl;
		return -4;
	}
	// 7. 写header
	ret = avformat_write_header(ofmt, NULL);
	if (ret < 0)
	{
		std::cout << "avformat_write_header for output file error,ret=" << ret << std::endl;
		return -5;
	}

	// 8. 发送视频流
	int64_t start_time = av_gettime();
	while (true)
	{
		AVStream* stream;
		ret = av_read_frame(ifmt, pkt);
		if (ret < 0)
		{
			break;
		}
		stream = ifmt->streams[pkt->stream_index];
		if (pkt->pts == AV_NOPTS_VALUE)
		{
			// 从帧率时间基->AV_TIME_BASE
			pkt->pts = av_rescale_q(frame_index, AVRational{ 1, int(av_q2d(stream->r_frame_rate)) }, AVRational{ 1, AV_TIME_BASE });
			pkt->pts = av_rescale_q(pkt->pts, AVRational{ 1, AV_TIME_BASE }, stream->time_base);
			pkt->dts = pkt->pts;

		}
		// 计算延迟。推流段要根据视频帧率设置，相当于限速，否则流媒体服务器的压力会很大
		if (pkt->stream_index == video_index)
		{
			AVRational time_base = ifmt->streams[video_index]->time_base;
			AVRational time_base_cur = { 1, AV_TIME_BASE };
			int64_t pts_time = av_rescale_q(pkt->pts, time_base, time_base_cur);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time)
			{
				av_usleep(pts_time - now_time);
			}
		}

		ret = av_interleaved_write_frame(ofmt, pkt);
		if (ret < 0)
		{
			std::cout << "av_interleaved_write_frame error,ret=" << ret << std::endl;
			break;
		}
		av_packet_unref(pkt);
	}

	// 写尾巴
	av_write_trailer(ofmt);

	avformat_close_input(&ifmt);
	avio_close(ofmt->pb);
	avformat_free_context(ofmt);

	return 0;
}