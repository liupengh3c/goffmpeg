#include "FFmpeg_Info.h"
#include <iostream>

extern "C"
{
#include "libavformat/avformat.h"
}

int FFmpeg_Info::avformat_info()
{
	std::cout << avformat_configuration() << std::endl;
	std::cout << avformat_version() << std::endl;
	return 0;
}

int FFmpeg_Info::avcodec_info()
{
	std::cout << avcodec_configuration() << std::endl;
	std::cout << avcodec_version() << std::endl;
	return 0;
}