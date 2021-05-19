//
//  FilteredAudioFilePlayer.h
//  Copyright, Nikolay Iontchev
//
//  Low system level audio player using Audio Toolbox
//  audio queue for playing audio streams.
//  The audio system calls audio_queue_player_callback_t
//  when more data is required to be forwarded to the audio queue.
//
//  Created by NI on 18.05.21.
//

#ifndef __AudioQueuePlayer_h_
#define __AudioQueuePlayer_h_

#include <AudioToolbox/AudioToolbox.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

struct AudioQueuePlayer_t;

typedef void (*AudioQueuePlayerHasStoppedCallback)(void* user_data);

typedef struct AudioQueuePlayer_t*		H_AUDIO_QUEUE_PLAYER;
typedef void (*audio_queue_player_callback_t)(void* user_data, float* out_buffer, int out_buffer_length);

H_AUDIO_QUEUE_PLAYER 	audio_queue_player_init(float sample_rate, int buffer_size,
												audio_queue_player_callback_t callback, void* user_data);
void					audio_queue_player_destroy(H_AUDIO_QUEUE_PLAYER h);

void					audio_queue_player_start(H_AUDIO_QUEUE_PLAYER h);
void					audio_queue_player_stop(H_AUDIO_QUEUE_PLAYER h);
int						audio_queue_player_is_playing(H_AUDIO_QUEUE_PLAYER h);
void					audio_queue_player_register_stopped_callback(H_AUDIO_QUEUE_PLAYER h, AudioQueuePlayerHasStoppedCallback callback, void* user);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__AudioQueuePlayer_h_
