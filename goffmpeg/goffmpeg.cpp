// goffmpeg.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "FFmpeg_Info.h"
#include "Demux.h"
#include "DecodeVideo.h"
#include "DecodeVideo2.h"
#include "DecodeAudio.h"

int strToInt(char* p)
{
	int length = strlen(p);
	int val = 0;
	for (size_t i = 0; i < length; i++)
	{
		val += (p[i] - 0x30) * pow(10, length - 1 - i);
	}
	return val;
}
int main()
{
	char input[5] = {};
	int number = 0;

	std::string msg = "\n\nAll the funtions are:\n\
	1. print ffmpeg informations.\n\
	2. demux mp4 to h264+aac/dts,you should input the mp4 path.\n\
	3. decode h264 to yuv420p(av_parser_parser2).\n\
	4. decode h264 to yuv420p(av_read_frame).\n\
	5. decode aac to pcm(av_parser_parser2).\n";
	while (true)
	{
		std::cout << msg << std::endl;
		std::cout << "please select the number:";
		std::cin >> input;
		number = strToInt(input);
		switch (number)
		{
			case 1:
			{
				FFmpeg_Info* infos = new FFmpeg_Info();
				infos->avformat_info();
				infos->avcodec_info();
				break;
			}
			case 2:
			{
				std::cout << "please input the mp4 file path:";
				std::string path;
				std::cin >> path;
				Demux* demux = new Demux();
				demux->demux(path);
				break;
			}
			case 3:
			{
				std::cout << "please input the h264 file path:";
				std::string h264;
				std::string yuv420;
				std::cin >> h264;

				std::cout << "please input the yuv420p file path:";
				std::cin >> yuv420;
				DecodeVideo* decode = new DecodeVideo();
				decode->decode_video(h264, yuv420);
				break;
			}
			case 4:
			{
				std::cout << "please input the media file path:";
				std::string h264;
				std::string yuv420;
				std::cin >> h264;

				std::cout << "please input the yuv420p file path:";
				std::cin >> yuv420;
				DecodeVideo2* decode = new DecodeVideo2();
				decode->decode_video2(h264, yuv420);
				break;
			}
			case 5:
			{
				std::cout << "please input the dts file path:";
				std::string dts;
				std::string pcm;
				std::cin >> dts;

				std::cout << "please input the pcm file path:";
				std::cin >> pcm;
				DecodeAudio* decode = new DecodeAudio();
				decode->decode_audio(dts, pcm);
				break;
			}
			default:
				break;
		}
	}
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
