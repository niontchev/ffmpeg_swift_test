//
//  AudioFilePlayer.h
//  SmuleFFmpeg
//
//  Created by NI on 18.05.21.
//

#ifndef AudioFilePlayer_h
#define AudioFilePlayer_h

#include "AudioQueuePlayer.h"

struct AudioFilePlayer_t;

typedef struct AudioFilePlayer_t*		H_AUDIO_FILE_PLAYER;

typedef void (*audio_file_filter_callback)(void* user_data, float *input, float *output, int num_samples);

#ifdef __cplusplus
extern "C" {
#endif

H_AUDIO_FILE_PLAYER 	audio_file_player_init(void);
int						audio_file_player_open(H_AUDIO_FILE_PLAYER h, const char* filePath);
void					audio_file_player_destroy(H_AUDIO_FILE_PLAYER h);
void					audio_file_player_start(H_AUDIO_FILE_PLAYER h);
void					audio_file_player_stop(H_AUDIO_FILE_PLAYER h);
void					audio_file_player_pause(H_AUDIO_FILE_PLAYER h);
void					audio_file_player_resume(H_AUDIO_FILE_PLAYER h);
void					audio_file_player_register_filter(H_AUDIO_FILE_PLAYER h, audio_file_filter_callback filter, void* user_data);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* AudioFilePlayer_h */
