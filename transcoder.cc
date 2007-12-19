
#include "transcoder.h"
#include <math.h>
#include <iostream>

#define MAX_AUDIO_PACKET_SIZE (128 * 1024) 

using namespace std;

Transcoder::Transcoder() {
	//pthread_mutex_init(&this->lock, NULL);
	avcodec_init();
    	avcodec_register_all();
    	av_register_all();
}

Transcoder::~Transcoder() {

}

bool Transcoder::setInputFile(string filename) {
	// open the input file
	if(av_open_input_file(&this->inputFormat, filename.c_str(), NULL, 0, NULL) != 0) {
		return false;
	}

	if(av_find_stream_info(this->inputFormat) < 0) {
		return false;
	}

	dump_format(this->inputFormat, 0, filename.c_str(), false);

	videoStream = getStream(CODEC_TYPE_VIDEO);
	if(videoStream != NULL) {
		videoCodec = avcodec_find_decoder(videoStream->codec->codec_id);
		if(videoCodec == NULL) {
			return false;
		}

		if(avcodec_open(videoStream->codec, videoCodec) < 0) {
			return false;
		}
	}

	audioStream = getStream(CODEC_TYPE_AUDIO);
	if(audioStream != NULL) {
		audioCodec = avcodec_find_decoder(audioStream->codec->codec_id);
		if(audioCodec == NULL) {
			fprintf(stderr,"Cannot find decoder for audio\n");
			return false;
		}

		if(avcodec_open(audioStream->codec, audioCodec) < 0) {
			fprintf(stderr,"Cannot open decoder for audio\n");
			return false;
		}
		printf("frame size: %d\n", audioStream->codec->frame_size);
	}

	//if(videoCodec->capabilities & CODEC_CAP_TRUNCATED) {
	//	videoStream->codec->flags|=CODEC_FLAG_TRUNCATED;
	//}

	inputFile = filename;

	return true;
}

bool Transcoder::setOutput(const char* type, CodecID videoCodecID, CodecID audioCodecID) {
	AVOutputFormat* fmt;
	AVCodecContext* cc;

	// open the output file
	fmt = guess_format(type, NULL, NULL);

	fmt->video_codec = videoCodecID;
	fmt->audio_codec = audioCodecID;

	outputFormat = av_alloc_format_context();
	outputFormat->oformat = fmt;

	// add the video stream
	videoStreamOut = av_new_stream(outputFormat, 0);
	cc = videoStreamOut->codec;
	cc->codec_id = fmt->video_codec;
	cc->codec_type = CODEC_TYPE_VIDEO;
	cc->bit_rate = 3000000;
	cc->width = videoStream->codec->width;
	cc->height = videoStream->codec->height;
	cc->time_base = videoStream->codec->time_base;
	cc->pix_fmt = videoStream->codec->pix_fmt;
	cc->gop_size = videoStream->codec->gop_size;

	if(audioCodecID != CODEC_ID_NONE) {
		// add the audio stream
		audioStreamOut = av_new_stream(outputFormat, 1);
		cc = audioStreamOut->codec;
		cc->codec_id = fmt->audio_codec;
		cc->codec_type = CODEC_TYPE_AUDIO;
		cc->bit_rate = audioStream->codec->bit_rate;
		cc->sample_rate = audioStream->codec->sample_rate;
		cc->channels = audioStream->codec->channels;
		cc->time_base = audioStream->codec->time_base;
	}

	if (av_set_parameters(outputFormat, NULL) < 0) {
	        fprintf(stderr, "Invalid output format parameters\n");
	        exit(1);
	}

	dump_format(outputFormat, 0, NULL, 1);

	videoCodecOut = avcodec_find_encoder(videoStreamOut->codec->codec_id);
	if(videoCodecOut == NULL) {
		fprintf(stderr, "Cannot find video encoder!\n");
		return false;
	}
	if(avcodec_open(videoStreamOut->codec, videoCodecOut) < 0) {
		fprintf(stderr, "Cannot open video encoder!\n");
		return false;
	}

	if(audioCodecID != CODEC_ID_NONE) {
		audioCodecOut = avcodec_find_encoder(audioStreamOut->codec->codec_id);
		if(audioCodecOut == NULL) {
			fprintf(stderr, "Cannot find audio encoder!\n");
			return false;
		}
		if(avcodec_open(audioStreamOut->codec, audioCodecOut) < 0) {
			fprintf(stderr, "Cannot open audio encoder!\n");
			return false;
		}
	}

	return true;
}


bool Transcoder::setOutputFile(string filename, CodecID videoCodecID, CodecID audioCodecID) {
	AVOutputFormat* tmp;
	bool ret = true;

	tmp = guess_format(NULL, filename.c_str(), NULL);
	if(tmp != NULL) {
		if(videoCodecID == -1 && audioCodecID == -1) {
			ret = setOutput(tmp->name, tmp->video_codec, tmp->audio_codec);
		}
		else {
			ret = setOutput(tmp->name, videoCodecID, audioCodecID);
		}
	}
	else {
		cout << "Cannot guess format for " << filename << endl;
		ret = false;
	}

	if(ret) {
		outputFile = filename;
	}

	return ret;
}

AVStream* Transcoder::getStream(CodecType type) {
	for(int i = 0; i < this->inputFormat->nb_streams; i++) {
		if(this->inputFormat->streams[i]->codec->codec_type == type) {
			return this->inputFormat->streams[i];
		}
	}

	return NULL;
}


void Transcoder::transcode() {
	AVFrame* frame;
	AVFrame* frameRGB;
	AVPacket packet;
	int frameFinished;
	int out_size;
	int outbuf_len;
	uint8_t* outbuf;
	uint8_t* audiobuf;
	uint8_t* resamplebuf;
	uint8_t* databuf;
	int audiobuf_len;
	int resamplebuf_len;
	int ret;
	float t, tincr;
	int64_t pt;
	ReSampleContext* resample;
	AVFifoBuffer fifo;

	frame = avcodec_alloc_frame();
	frameRGB = avcodec_alloc_frame();


	av_fifo_init(&fifo, AVCODEC_MAX_AUDIO_FRAME_SIZE);

	outbuf_len = avpicture_get_size(videoStream->codec->pix_fmt, videoStream->codec->width, videoStream->codec->height);
	outbuf = (uint8_t*) calloc(outbuf_len, sizeof(uint8_t));

	audiobuf = (uint8_t*) calloc(AVCODEC_MAX_AUDIO_FRAME_SIZE, sizeof(uint8_t));
	resamplebuf = (uint8_t*) calloc(AVCODEC_MAX_AUDIO_FRAME_SIZE, sizeof(uint8_t));
	databuf = (uint8_t*) calloc(AVCODEC_MAX_AUDIO_FRAME_SIZE, sizeof(uint8_t));

	avpicture_fill((AVPicture *)frameRGB, outbuf, PIX_FMT_YUV420P, videoStream->codec->width, videoStream->codec->height);

	//uint8_t* filebuf = (uint8_t*) malloc(4096);
	//url_open_buf(&outputFormat->pb, filebuf, 4096, URL_WRONLY);
	//outputFormat->pb.write_packet = io_writePacket;
	cout << "Input file: " << inputFile << endl;
	cout << "Output file: " << outputFile << endl;
	if(outputFile.empty()) {
		printf("outputFile cannot be empty\n");
		return;
	}

	if(url_fopen(&outputFormat->pb, outputFile.c_str(), URL_WRONLY) < 0) {
		printf("error opening output file\n");
		return;
	}

	resample = audio_resample_init(audioStreamOut->codec->channels, audioStream->codec->channels, audioStreamOut->codec->sample_rate, audioStream->codec->sample_rate);

	av_write_header(outputFormat);

	while(av_read_frame(inputFormat, &packet) >= 0) {
		pt = packet.pts;
		if(packet.stream_index == videoStream->index) {
			avcodec_decode_video(videoStream->codec, frame, &frameFinished, packet.data, packet.size);

			//frame->pts = av_rescale(frame, AV_TIME_BASE*(int64_t) outputCodecCtx->frame_rate_base,
			//				outputCodecCtx->frame_rate);

			if(frameFinished) {
				img_convert((AVPicture *)frameRGB, PIX_FMT_YUV420P, 
					(AVPicture*)frame, videoStream->codec->pix_fmt, videoStream->codec->width, 
					videoStream->codec->height);

				out_size = avcodec_encode_video(videoStreamOut->codec, outbuf, outbuf_len, frame);
				//fprintf(stderr,"size = %d, ff = %d, outsize=%d\n", packet.size, frameFinished, out_size);
				if(out_size > 0) {
					AVPacket pkt;
					av_init_packet(&pkt);

					pkt.pts= av_rescale_q(videoStreamOut->codec->coded_frame->pts, videoStreamOut->codec->time_base, videoStreamOut->time_base);
					if(videoStreamOut->codec->coded_frame->key_frame)
						pkt.flags |= PKT_FLAG_KEY;

					pkt.stream_index= videoStreamOut->index;
					//pkt.data= video_outbuf;
					pkt.data= outbuf;
					pkt.size= out_size;

					/* write the compressed frame in the media file */
					ret = av_write_frame(outputFormat, &pkt);

					//fwrite(outbuf, 1, out_size, fp);

					av_free_packet(&pkt);
				}
			}
		}
		else if(audioStreamOut->codec->codec_id != CODEC_ID_NONE && packet.stream_index == audioStream->index) {
			if(audioStream->codec->codec_id == audioStreamOut->codec->codec_id && audioStream->codec->sample_rate == audioStreamOut->codec->sample_rate && audioStream->codec->channels == audioStream->codec->channels) {
				av_write_frame(outputFormat, &packet);
			}
			else {
				avcodec_decode_audio(audioStream->codec, (short *)audiobuf, &audiobuf_len, packet.data, packet.size);

				if(audioStream->codec->sample_rate != audioStreamOut->codec->sample_rate || audioStream->codec->channels != audioStreamOut->codec->channels) {
					// resample the audio
					resamplebuf_len = audio_resample(resample, (short*)resamplebuf, (short*)audiobuf, audiobuf_len / (audioStream->codec->channels * 2));
					av_fifo_write(&fifo, resamplebuf, resamplebuf_len * audioStreamOut->codec->channels * 2);
				}
				else {
					av_fifo_write(&fifo, audiobuf, audiobuf_len);
				}

				if(audiobuf_len > 0) {
					int frame_bytes = audioStreamOut->codec->frame_size * 2 * audioStreamOut->codec->channels;
					while(av_fifo_read(&fifo, databuf, frame_bytes) == 0) {
						AVPacket pkt;
						av_init_packet(&pkt);

						pkt.size = avcodec_encode_audio(audioStreamOut->codec, outbuf, outbuf_len, (short*)databuf);
						pkt.stream_index = audioStreamOut->index;
						pkt.data = outbuf;
						pkt.flags |= PKT_FLAG_KEY;

						// write the compressed frame in the media file
						ret = av_write_frame(outputFormat, &pkt);

						av_free_packet(&pkt);
					}
				}
			}
		}
		av_free_packet(&packet);
	}

	av_write_trailer(outputFormat);
}

void* _start_transcoder(void* obj) {
	((Transcoder*) obj)->transcode();
	return NULL;
}

void Transcoder::startTranscoder() {
	cout << "Starting transcoder" << endl;
	pthread_create(&thread, NULL, _start_transcoder, this);
}

void Transcoder::stopTranscoder() {
	cout << "Stopping transcoder" << endl;
	pthread_cancel(thread);
}

/*int main(int argc, char* argv[]) {
	Transcoder* t;
	string filename;

	if(argc == 2) {
		filename = string(argv[1]);

		printf("attempting to transcode\n");

		t = new Transcoder();

		t->setInputFile(filename);
		t->setOutputFile("testing.wmv", CODEC_ID_WMV2, CODEC_ID_MP3);
		//t->setOutputFile("testing.wmv", CODEC_ID_WMV2, CODEC_ID_PCM_S16LE);
		t->transcode();

		delete t;
	}
}*/
