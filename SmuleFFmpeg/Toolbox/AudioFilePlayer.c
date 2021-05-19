//
//  AudioFilePlayer.c
//  SmuleFFmpeg
//
//  Created by NI on 18.05.21.
//

#include "AudioFilePlayer.h"
#include "AudioQueuePlayer.h"
#include "WavFile.h"

#define AUDIO_FILE_PLAYER_BUFFER_SIZE				512

typedef struct AudioFilePlayer_t {
	H_READ_WAVE_FILE			wave_file;
	H_AUDIO_QUEUE_PLAYER		queue_player;
	int							destroy_state_active;
	int 						playing;
	int 						paused;
	int 						start_new;
	// Filter
	audio_file_filter_callback	filter;
	void*						filter_user_data;
	char						file_path[1024];
} AudioFilePlayer;

static void _audio_file_player_real_destroy(H_AUDIO_FILE_PLAYER pPlayer) {
	if (pPlayer->queue_player) {
		audio_queue_player_destroy(pPlayer->queue_player);
		pPlayer->queue_player = 0;
	}
	if (pPlayer->wave_file) {
		read_wave_file_destroy(pPlayer->wave_file);
		pPlayer->wave_file = 0;
	}
	free(pPlayer);
}

static void _audio_file_player_callback(void* user_data, float* outBuffer, int bufferLen) {
	H_AUDIO_FILE_PLAYER pPlayer = (H_AUDIO_FILE_PLAYER)user_data;
	if (pPlayer && pPlayer->playing) {
		int bytes_read = read_wave_file_read(pPlayer->wave_file, outBuffer, bufferLen);
		if (bytes_read < bufferLen) {
			pPlayer->playing = 0;
			audio_queue_player_stop(pPlayer->queue_player);
		}
		else if (pPlayer->filter != 0)
			pPlayer->filter(pPlayer->filter_user_data, outBuffer, outBuffer, bufferLen);
	}
}

static void audio_queue_player_stopped_callback(void* user_data);

static int _real_audio_file_player_open(H_AUDIO_FILE_PLAYER pPlayer, const char* filePath) {
	int error = 0;

	pPlayer->destroy_state_active = 0;
	pPlayer->playing = 0;
	pPlayer->paused = 0;
	pPlayer->start_new = 0;
	
	if (pPlayer->wave_file)
		read_wave_file_destroy(pPlayer->wave_file);
	pPlayer->wave_file = read_wave_file_init(filePath);
	if (pPlayer->wave_file == 0)
		error = -1;
	if (!error && pPlayer->queue_player == 0) {
		float sample_rate = read_wave_file_get_sample_rate(pPlayer->wave_file);
		pPlayer->queue_player = audio_queue_player_init(sample_rate, AUDIO_FILE_PLAYER_BUFFER_SIZE, _audio_file_player_callback, pPlayer);
		audio_queue_player_register_stopped_callback(pPlayer->queue_player, audio_queue_player_stopped_callback, pPlayer);
	}
	return error;
}

static void audio_queue_player_stopped_callback(void* user_data) {
	H_AUDIO_FILE_PLAYER pPlayer = (H_AUDIO_FILE_PLAYER)user_data;
	if (pPlayer && pPlayer->destroy_state_active)
		_audio_file_player_real_destroy(pPlayer);
	else if (pPlayer->start_new) {
		_real_audio_file_player_open(pPlayer, pPlayer->file_path);
	}
}

H_AUDIO_FILE_PLAYER audio_file_player_init() {
	H_AUDIO_FILE_PLAYER pPlayer = (H_AUDIO_FILE_PLAYER)malloc(sizeof(AudioFilePlayer));
	memset(pPlayer, 0, sizeof(AudioFilePlayer));
	return pPlayer;
}

int audio_file_player_open(H_AUDIO_FILE_PLAYER pPlayer, const char* filePath) {
	int error = 0;
	if (pPlayer->queue_player && audio_queue_player_is_playing(pPlayer->queue_player)) {
		pPlayer->start_new = 1;
		strcpy(pPlayer->file_path, filePath);
		audio_queue_player_stop(pPlayer->queue_player);
	}
	else
		error = _real_audio_file_player_open(pPlayer, filePath);

	return error;
}

void audio_file_player_destroy(H_AUDIO_FILE_PLAYER pPlayer) {
	if (audio_queue_player_is_playing(pPlayer->queue_player)) {
		audio_queue_player_stop(pPlayer->queue_player);
		pPlayer->destroy_state_active = 1;
	}
	else {
		// if not playing destroy roght away
		_audio_file_player_real_destroy(pPlayer);
	}
}

void audio_file_player_start(H_AUDIO_FILE_PLAYER pPlayer) {
	if (!pPlayer->playing) {
		pPlayer->playing = 1;
		if (!pPlayer->paused)
			audio_queue_player_start(pPlayer->queue_player);
		pPlayer->paused = 0;
	}
}

void audio_file_player_stop(H_AUDIO_FILE_PLAYER pPlayer) {
	if (pPlayer->playing || pPlayer->paused) {
		pPlayer->playing = 0;
		pPlayer->paused = 0;
		audio_queue_player_stop(pPlayer->queue_player);
	}
}

void audio_file_player_resume(H_AUDIO_FILE_PLAYER pPlayer) {
	if (!pPlayer->playing && pPlayer->paused) {
		pPlayer->playing = 1;
		pPlayer->paused = 0;
	}
}

void audio_file_player_pause(H_AUDIO_FILE_PLAYER pPlayer) {
	if (pPlayer->playing) {
		pPlayer->playing = 0;
		pPlayer->paused = 1;
	}
}

void audio_file_player_register_filter(H_AUDIO_FILE_PLAYER pPlayer, audio_file_filter_callback filter, void* user_data) {
	pPlayer->filter = filter;
	pPlayer->filter_user_data = user_data;
}
