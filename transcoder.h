#ifndef _TRANSCODER_H
#define _TRANSCODER_H

extern "C" {
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#include <ffmpeg/fifo.h>
}

#include <pthread.h>

#include <string>


class Transcoder {
	std::string inputFile;
	std::string outputFile;

	AVFormatContext* inputFormat;
	AVStream* videoStream;
	AVCodec* videoCodec;
	AVStream* audioStream;
	AVCodec* audioCodec;

	AVFormatContext* outputFormat;
	AVStream* videoStreamOut;
	AVCodec* videoCodecOut;
	AVStream* audioStreamOut;
	AVCodec* audioCodecOut;

	uint8_t* buffer;
	int buffer_len;
	pthread_t thread;

	AVStream* getStream(CodecType);

	public:
		Transcoder();
		~Transcoder();
		bool setInputFile(std::string filename);
		bool setOutput(const char* type, CodecID videoCodecID, CodecID audioCodecID);
		bool setOutputFile(std::string filename, CodecID videoCodecID, CodecID audioCodecID);
		void transcode();
		void startTranscoder();
		void stopTranscoder();
};

#endif
