#include "AudioQueuePlayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define AUDIO_QUEUE_PLAYER_NUM_BUFFERS				2

typedef enum AudioQueuePlayerState_t {
	AudioQueuePlayerStateStoped,
	AudioQueuePlayerStatePlaying,
	AudioQueuePlayerStateStopping
} AudioQueuePlayerState;

typedef struct AudioQueuePlayer_t {
	AudioQueueRef 						queue;
	AudioQueuePlayerState				state;
	AudioStreamBasicDescription 		dataFormat;
	AudioQueueBufferRef					buffers[AUDIO_QUEUE_PLAYER_NUM_BUFFERS];
	void*								userData;
	audio_queue_player_callback_t		callback;
	void*								stoppedCallbackUserData;
	AudioQueuePlayerHasStoppedCallback 	stoppedCallback;
} AudioQueuePlayer;

static void _audio_queue_player_callback(void* userData, AudioQueueRef autoQueue, AudioQueueBufferRef buffer) {
	AudioQueuePlayer* player = (AudioQueuePlayer*)userData;
	if (player->state == AudioQueuePlayerStatePlaying) {
		player->callback(player->userData, buffer->mAudioData, buffer->mAudioDataBytesCapacity / 4);
		buffer->mAudioDataByteSize = buffer->mAudioDataBytesCapacity;
		AudioQueueEnqueueBuffer(player->queue, buffer, 0, NULL);
	}
	else if (player->state == AudioQueuePlayerStateStopping) {
		player->state = AudioQueuePlayerStateStoped;
		AudioQueueFlush(player->queue);
		AudioQueueStop(player->queue, 1);
		if (player->stoppedCallback)
			player->stoppedCallback(player->stoppedCallbackUserData);
	}
}

H_AUDIO_QUEUE_PLAYER audio_queue_player_init(float sample_rate, int buffer_size,
											 audio_queue_player_callback_t callback, void* user_data) {
	H_AUDIO_QUEUE_PLAYER h = (H_AUDIO_QUEUE_PLAYER)malloc(sizeof(AudioQueuePlayer));
	
	h->state = AudioQueuePlayerStateStoped;
	h->userData = user_data;
	h->callback = callback;
	h->stoppedCallback = 0;
	
	h->dataFormat.mBitsPerChannel = 32;
	h->dataFormat.mBytesPerFrame = 4;
	h->dataFormat.mBytesPerPacket = 4;
	h->dataFormat.mChannelsPerFrame = 1;
	h->dataFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
	h->dataFormat.mFormatID = kAudioFormatLinearPCM;
	h->dataFormat.mFramesPerPacket = 1;
	h->dataFormat.mReserved = 0;
	h->dataFormat.mSampleRate = sample_rate;
	
	OSStatus error = AudioQueueNewOutput(&h->dataFormat, _audio_queue_player_callback, h, NULL, NULL, 0, &h->queue);
	if (error == noErr) {
		for (int i = 0; i < AUDIO_QUEUE_PLAYER_NUM_BUFFERS && error == noErr; ++i) {
			error = AudioQueueAllocateBuffer(h->queue, buffer_size * sizeof(float), &h->buffers[i]);
			if (error == noErr)
				memset(h->buffers[i]->mAudioData, 0, h->buffers[i]->mAudioDataBytesCapacity);
		}
	}
	else {
		free(h);
		h = 0;
	}
	return h;
}

void audio_queue_player_destroy(H_AUDIO_QUEUE_PLAYER h) {
	if (h) {
		if (h->queue) {
			AudioQueueStop(h->queue, 1);
			AudioQueueDispose(h->queue, TRUE);
			h->queue = 0;
		}
		free(h);
	}
}

void audio_queue_player_start(H_AUDIO_QUEUE_PLAYER h) {
	h->state = AudioQueuePlayerStatePlaying;
	for (int i = 0; i < AUDIO_QUEUE_PLAYER_NUM_BUFFERS; ++i) {
		h->callback(h->userData, h->buffers[i]->mAudioData, h->buffers[i]->mAudioDataBytesCapacity / 4);
		h->buffers[i]->mAudioDataByteSize = h->buffers[i]->mAudioDataBytesCapacity;
		AudioQueueEnqueueBuffer(h->queue, h->buffers[i], 0, NULL);
	}
	AudioQueuePrime(h->queue, 0, NULL);
	AudioQueueStart(h->queue, NULL);
}

void audio_queue_player_stop(H_AUDIO_QUEUE_PLAYER h) {
	h->state = AudioQueuePlayerStateStopping;
}

int audio_queue_player_is_playing(H_AUDIO_QUEUE_PLAYER h) {
	return h && h->state == AudioQueuePlayerStatePlaying;
}

void audio_queue_player_register_stopped_callback(H_AUDIO_QUEUE_PLAYER h, AudioQueuePlayerHasStoppedCallback callback, void* user) {
	if (h) {
		h->stoppedCallback = callback;
		h->stoppedCallbackUserData = user;
	}
}
