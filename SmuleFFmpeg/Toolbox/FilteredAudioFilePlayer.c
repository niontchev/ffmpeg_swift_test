//
//  FilteredAudioFilePlayer.c
//  SmuleFFmpeg
//
//  Created by NI on 18.05.21.
//

#include "FilteredAudioFilePlayer.h"
#include "AudioQueuePlayer.h"
//#include "WavFile.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>

#ifdef __cplusplus
}
#endif


#define RAW_OUT_ON_PLANAR 							  1
#define FAUDIO_FILE_PLAYER_BUFFER_SIZE				512
#define FAUDIO_INPUT_FIFO_NUM_SAMPLES				1024

typedef struct FilteredAudioFilePlayer_t {
	H_AUDIO_QUEUE_PLAYER		queue_player;
	int							destroy_state_active;
	int 						playing;
	int 						paused;
	float						fifo[FAUDIO_INPUT_FIFO_NUM_SAMPLES * 4];
	int 						fifo_head;
	int 						fifo_tail;
	//FFmpeg
	AVFormatContext*			formatCtx;
	AVCodec* 					codec;
	AVCodecContext* 			codecCtx;
	AVFrame* 					frame;
	AVPacket* 					packet;
	int 						audioStreamIndex;
	int 						sample_rate;
	int 						channels;
	// Filter
	faudio_file_filter_callback	filter;
	void*						filter_user_data;
} FilteredAudioFilePlayer;

static int printError(const char* prefix, int errorCode) {
	if(errorCode == 0) {
		return 0;
	} else {
		const size_t bufsize = 64;
		char buf[bufsize];

		if(av_strerror(errorCode, buf, bufsize) != 0) {
			strcpy(buf, "UNKNOWN_ERROR");
		}
		fprintf(stderr, "%s (%d: %s)\n", prefix, errorCode, buf);

		return errorCode;
	}
}

/**
 * Extract a single sample and convert to float.
 */
static float getSample(const AVCodecContext* codecCtx, uint8_t* buffer, int sampleIndex) {
	int64_t val = 0;
	float ret = 0;
	int sampleSize = av_get_bytes_per_sample(codecCtx->sample_fmt);
	switch(sampleSize) {
		case 1:
			// 8bit samples are always unsigned
			val = ((uint8_t*)buffer)[sampleIndex];
			// make signed
			val -= 127;
			break;

		case 2:
			val = ((int16_t*)buffer)[sampleIndex];
			break;

		case 4:
			val = ((int32_t*)buffer)[sampleIndex];
			break;

		case 8:
			val = ((int64_t*)buffer)[sampleIndex];
			break;

		default:
			fprintf(stderr, "Invalid sample size %d.\n", sampleSize);
			return 0;
	}

	// Check which data type is in the sample.
	switch(codecCtx->sample_fmt) {
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_U8P:
		case AV_SAMPLE_FMT_S16P:
		case AV_SAMPLE_FMT_S32P:
			// integer => Scale to [-1, 1] and convert to float.
			ret = (float)val / (float)((1 << (sampleSize*8-1))-1);
			break;

		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
			// float => reinterpret
			ret = *(float*)&val;
			break;

		case AV_SAMPLE_FMT_DBL:
		case AV_SAMPLE_FMT_DBLP:
			// double => reinterpret and then static cast down
			ret = *(double*)&val;
			break;

		default:
			fprintf(stderr, "Invalid sample format %s.\n", av_get_sample_fmt_name(codecCtx->sample_fmt));
			return 0;
	}

	return ret;
}

/**
 * Find the first audio stream and returns its index. If there is no audio stream returns -1.
 */
static int findAudioStream(const AVFormatContext* formatCtx) {
	int audioStreamIndex = -1;
	for(int i = 0; i < formatCtx->nb_streams; ++i) {
		// Use the first audio stream we can find.
		// NOTE: There may be more than one, depending on the file.
		if(formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStreamIndex = i;
			break;
		}
	}
	return audioStreamIndex;
}

/*
 * Print information about the input file and the used codec.
 */
static void printStreamInformation(const AVCodec* codec, const AVCodecContext* codecCtx, int audioStreamIndex) {
	fprintf(stderr, "Codec: %s\n", codec->long_name);
	if(codec->sample_fmts != NULL) {
		fprintf(stderr, "Supported sample formats: ");
		for(int i = 0; codec->sample_fmts[i] != -1; ++i) {
			fprintf(stderr, "%s", av_get_sample_fmt_name(codec->sample_fmts[i]));
			if(codec->sample_fmts[i+1] != -1) {
				fprintf(stderr, ", ");
			}
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "---------\n");
	fprintf(stderr, "Stream:        %7d\n", audioStreamIndex);
	fprintf(stderr, "Sample Format: %7s\n", av_get_sample_fmt_name(codecCtx->sample_fmt));
	fprintf(stderr, "Sample Rate:   %7d\n", codecCtx->sample_rate);
	fprintf(stderr, "Sample Size:   %7d\n", av_get_bytes_per_sample(codecCtx->sample_fmt));
	fprintf(stderr, "Channels:      %7d\n", codecCtx->channels);
	fprintf(stderr, "Planar:        %7d\n", av_sample_fmt_is_planar(codecCtx->sample_fmt));
	fprintf(stderr, "Float Output:  %7s\n", !RAW_OUT_ON_PLANAR || av_sample_fmt_is_planar(codecCtx->sample_fmt) ? "yes" : "no");
}

static void output_and_filter(H_FAUDIO_FILE_PLAYER pPlayer, float* input, float* output, int input_num, int output_num) {
	// drain the fifo first
	while (output_num > 0 && pPlayer->fifo_head != pPlayer->fifo_tail) {
		int num_to_read = (pPlayer->fifo_head + FAUDIO_INPUT_FIFO_NUM_SAMPLES - pPlayer->fifo_tail) % FAUDIO_INPUT_FIFO_NUM_SAMPLES;
		int num_max = FAUDIO_INPUT_FIFO_NUM_SAMPLES - pPlayer->fifo_tail;
		if (num_max < num_to_read)
			num_to_read = num_max;
		memcpy(output, pPlayer->fifo + pPlayer->fifo_tail, num_to_read * sizeof(float));
		pPlayer->fifo_tail = (pPlayer->fifo_tail + num_to_read) % FAUDIO_INPUT_FIFO_NUM_SAMPLES;
		output_num -= num_to_read;
	}
	if (output_num > 0 && input_num > 0) {
		int num_samples = output_num < input_num ? output_num : input_num;
		if (pPlayer->filter)
			pPlayer->filter(input, output, num_samples, pPlayer->filter_user_data);
		else
			memcpy(output, input, num_samples * sizeof(float));
		input_num -= num_samples;
		output_num -= num_samples;
	}
	while (input_num > 0) {
		int num_to_write = input_num;
		int num_max = FAUDIO_INPUT_FIFO_NUM_SAMPLES - pPlayer->fifo_head;
		if (num_max < num_to_write)
			num_to_write = num_max;
		if (pPlayer->filter)
			pPlayer->filter(input, pPlayer->fifo + pPlayer->fifo_head, num_to_write, pPlayer->filter_user_data);
		else
			memcpy(pPlayer->fifo + pPlayer->fifo_head, input, num_to_write * sizeof(float));
		pPlayer->fifo_head = (pPlayer->fifo_head + num_to_write) % FAUDIO_INPUT_FIFO_NUM_SAMPLES;

		input_num -= num_to_write;
	}
}
/**
 * Receive as many frames as available and handle them.
 */
static int receiveAndHandle(H_FAUDIO_FILE_PLAYER pPlayer, float* outBuffer, int num_samples) {
	int err = 0;
	int samples_read = 0;
	// Read the packets from the decoder.
	// NOTE: Each packet may generate more than one frame, depending on the codec.
	while(samples_read < num_samples && (err = avcodec_receive_frame(pPlayer->codecCtx, pPlayer->frame)) == 0) {
		// Let's handle the frame

		if(av_sample_fmt_is_planar(pPlayer->codecCtx->sample_fmt) == 1) {
			// This means that the data of each channel is in its own buffer.
			// => frame->extended_data[i] contains data for the i-th channel.
			for(int s = 0; s < pPlayer->frame->nb_samples; ++s) {
				for(int c = 0; c < pPlayer->codecCtx->channels; ++c) {
					float sample = getSample(pPlayer->codecCtx, pPlayer->frame->extended_data[c], s);
					int out_samples = samples_read < num_samples ? 1 : 0;
					output_and_filter(pPlayer, &sample, outBuffer, 1, out_samples);
					++samples_read;

				}
			}
		} else {
			// This means that the data of each channel is in the same buffer.
			// => frame->extended_data[0] contains data of all channels.
			if(RAW_OUT_ON_PLANAR) {
				int n = pPlayer->frame->linesize[0] / sizeof(float);
				int out_samples = samples_read < num_samples ? n : 0;
				if (out_samples > 0 && num_samples - samples_read < out_samples)
					out_samples = num_samples - samples_read;

				output_and_filter(pPlayer, (float*)pPlayer->frame->extended_data, outBuffer, n, out_samples);
				samples_read += n;
			} else {
				for(int s = 0; s < pPlayer->frame->nb_samples; ++s) {
					for(int c = 0; c < pPlayer->codecCtx->channels; ++c) {
						float sample = getSample(pPlayer->codecCtx, pPlayer->frame->extended_data[0], s * pPlayer->codecCtx->channels+c);
						int out_samples = samples_read < num_samples ? 1 : 0;
						output_and_filter(pPlayer, &sample, outBuffer, 1, out_samples);
						++samples_read;
					}
				}
			}
		}

		// Free any buffers and reset the fields to default values.
		av_frame_unref(pPlayer->frame);
	}
	return err;
}

static void close_ffmpeg_objects(H_FAUDIO_FILE_PLAYER pPlayer) {
	// Close all objects if open
	if (pPlayer->packet != 0)
		av_packet_free(&pPlayer->packet);
	// Free all data used by the frame.
	if (pPlayer->frame)
		av_frame_free(&pPlayer->frame);
	// Close the context and free all data associated to it, but not the context itself.
	if (pPlayer->codecCtx) {
		avcodec_close(pPlayer->codecCtx);
		// Free the context itself.
		avcodec_free_context(&pPlayer->codecCtx);
	}
	// Close the input.
	if (pPlayer->formatCtx)
		avformat_close_input(&pPlayer->formatCtx);
}

static void _faudio_file_player_real_destroy(H_FAUDIO_FILE_PLAYER pPlayer) {
	if (pPlayer->queue_player) {
		audio_queue_player_destroy(pPlayer->queue_player);
		pPlayer->queue_player = 0;
	}
	
	close_ffmpeg_objects(pPlayer);
	
	free(pPlayer);
}

static int faudio_file_player_read(H_FAUDIO_FILE_PLAYER pPlayer, float* outBuffer, int num_samples){
	int err = 0;
	while ((err = av_read_frame(pPlayer->formatCtx, pPlayer->packet)) != AVERROR_EOF) {
		if(err != 0) {
			// Something went wrong.
			printError("Read error.", err);
			break; // Don't return, so we can clean up nicely.
		}
		// Does the packet belong to the correct stream?
		if(pPlayer->packet->stream_index != pPlayer->audioStreamIndex) {
			// Free the buffers used by the frame and reset all fields.
			av_packet_unref(pPlayer->packet);
			continue;
		}
		// We have a valid packet => send it to the decoder.
		if((err = avcodec_send_packet(pPlayer->codecCtx, pPlayer->packet)) == 0) {
			// The packet was sent successfully. We don't need it anymore.
			// => Free the buffers used by the frame and reset all fields.
			av_packet_unref(pPlayer->packet);
		} else {
			// Something went wrong.
			// EAGAIN is technically no error here but if it occurs we would need to buffer
			// the packet and send it again after receiving more frames. Thus we handle it as an error here.
			printError("Send error.", err);
			break; // Don't return, so we can clean up nicely.
		}

		// Receive and handle frames.
		// EAGAIN means we need to send before receiving again. So thats not an error.
		if((err = receiveAndHandle(pPlayer, outBuffer, num_samples)) != AVERROR(EAGAIN)) {
			// Not EAGAIN => Something went wrong.
			printError("Receive error.", err);
			break; // Don't return, so we can clean up nicely.
		}
	}
	return err;
}

static void _faudio_file_player_callback(void* user_data, float* outBuffer, int bufferLen) {
	H_FAUDIO_FILE_PLAYER pPlayer = (H_FAUDIO_FILE_PLAYER)user_data;
	if (pPlayer && pPlayer->playing) {
		
		int err = faudio_file_player_read(pPlayer, outBuffer, bufferLen);
		
		if (err == AVERROR_EOF) {
			pPlayer->playing = 0;
			audio_queue_player_stop(pPlayer->queue_player);
		}
	}
}

static void audio_queue_player_stopped_callback(void* user_data) {
	H_FAUDIO_FILE_PLAYER pPlayer = (H_FAUDIO_FILE_PLAYER)user_data;
	if (pPlayer && pPlayer->destroy_state_active)
		_faudio_file_player_real_destroy(pPlayer);
}

H_FAUDIO_FILE_PLAYER faudio_file_player_init() {
	H_FAUDIO_FILE_PLAYER pPlayer = (H_FAUDIO_FILE_PLAYER)malloc(sizeof(FilteredAudioFilePlayer));
	memset(pPlayer, 0, sizeof(FilteredAudioFilePlayer));
	return pPlayer;
}

void faudio_file_player_open(H_FAUDIO_FILE_PLAYER pPlayer, const char* filePath) {
	int err = 0;

	if (pPlayer->queue_player && audio_queue_player_is_playing(pPlayer->queue_player)) {
		audio_queue_player_stop(pPlayer->queue_player);
	}
	pPlayer->destroy_state_active = 0;
	pPlayer->playing = 0;
	pPlayer->paused = 0;
	pPlayer->fifo_tail = pPlayer->fifo_head;
	close_ffmpeg_objects(pPlayer);
	
	pPlayer->formatCtx = NULL;
	 // Open the file and read the header.
	if ((err = avformat_open_input(&pPlayer->formatCtx, filePath, NULL, 0)) != 0) {
		printError("Error opening file.", err);
		return;
	}

	// In case the file had no header, read some frames and find out which format and codecs are used.
	// This does not consume any data. Any read packets are buffered for later use.
	avformat_find_stream_info(pPlayer->formatCtx, NULL);

	// Try to find an audio stream.
	pPlayer->audioStreamIndex = findAudioStream(pPlayer->formatCtx);
 	if(pPlayer->audioStreamIndex == -1) {
		// No audio stream was found.
		fprintf(stderr, "None of the available %d streams are audio streams.\n", pPlayer->formatCtx->nb_streams);
		close_ffmpeg_objects(pPlayer);
		return;
	}

	// Find the correct decoder for the codec.
	pPlayer->codec = (AVCodec*)avcodec_find_decoder(pPlayer->formatCtx->streams[pPlayer->audioStreamIndex]->codecpar->codec_id);
	if (pPlayer->codec == NULL) {
		// Decoder not found.
		fprintf(stderr, "Decoder not found. The codec is not supported.\n");
		close_ffmpeg_objects(pPlayer);
		return;
	}

	// Initialize codec context for the decoder.
	pPlayer->codecCtx = avcodec_alloc_context3(pPlayer->codec);
	if (pPlayer->codecCtx == NULL) {
		// Something went wrong. Cleaning up...
		fprintf(stderr, "Could not allocate a decoding context.\n");
		close_ffmpeg_objects(pPlayer);
		return;
	}

	// Fill the codecCtx with the parameters of the codec used in the read file.
	if ((err = avcodec_parameters_to_context(pPlayer->codecCtx, pPlayer->formatCtx->streams[pPlayer->audioStreamIndex]->codecpar)) != 0) {
		// Something went wrong. Cleaning up...
		printError("Error setting codec context parameters.", err);
		close_ffmpeg_objects(pPlayer);
		return;
	}

	// Explicitly request non planar data.
	pPlayer->codecCtx->request_sample_fmt = av_get_alt_sample_fmt(pPlayer->codecCtx->sample_fmt, 0);

	// Initialize the decoder.
	if ((err = avcodec_open2(pPlayer->codecCtx, pPlayer->codec, NULL)) != 0) {
		close_ffmpeg_objects(pPlayer);
		return;
	}

 
	pPlayer->sample_rate = pPlayer->codecCtx->sample_rate;
	pPlayer->channels = pPlayer->codecCtx->channels;
	
	// Print some intersting file information.
	printStreamInformation(pPlayer->codec, pPlayer->codecCtx, pPlayer->audioStreamIndex);

	if ((pPlayer->frame = av_frame_alloc()) == NULL) {
		close_ffmpeg_objects(pPlayer);
		return;
	}

	// Prepare the packet.
	//	AVPacket packet;
	// Set default values.
	pPlayer->packet = av_packet_alloc();
	
	if (pPlayer->queue_player == 0) {
		pPlayer->queue_player = audio_queue_player_init(pPlayer->sample_rate, FAUDIO_FILE_PLAYER_BUFFER_SIZE, _faudio_file_player_callback, pPlayer);
		audio_queue_player_register_stopped_callback(pPlayer->queue_player, audio_queue_player_stopped_callback, pPlayer);
	}
}

void faudio_file_player_destroy(H_FAUDIO_FILE_PLAYER pPlayer) {
	if (audio_queue_player_is_playing(pPlayer->queue_player)) {
		audio_queue_player_stop(pPlayer->queue_player);
		pPlayer->destroy_state_active = 1;
	}
	else {
		// if not playing destroy roght away
		_faudio_file_player_real_destroy(pPlayer);
	}
}

void faudio_file_player_start(H_FAUDIO_FILE_PLAYER pPlayer) {
	if (!pPlayer->playing) {
		pPlayer->playing = 1;
		if (!pPlayer->paused)
			audio_queue_player_start(pPlayer->queue_player);
		pPlayer->paused = 0;
	}
}

void faudio_file_player_stop(H_FAUDIO_FILE_PLAYER pPlayer) {
	if (pPlayer->playing || pPlayer->paused) {
		pPlayer->playing = 0;
		pPlayer->paused = 0;
		audio_queue_player_stop(pPlayer->queue_player);
	}

}

void faudio_file_player_resume(H_FAUDIO_FILE_PLAYER pPlayer) {
	if (!pPlayer->playing && pPlayer->paused) {
		pPlayer->playing = 1;
		pPlayer->paused = 0;
	}
}

void faudio_file_player_pause(H_FAUDIO_FILE_PLAYER pPlayer) {
	if (pPlayer->playing) {
		pPlayer->playing = 0;
		pPlayer->paused = 1;
	}
}

void faudio_file_player_register_filter(H_FAUDIO_FILE_PLAYER pPlayer, faudio_file_filter_callback filter, void* user_data) {
	pPlayer->filter = filter;
	pPlayer->filter_user_data = user_data;
}
