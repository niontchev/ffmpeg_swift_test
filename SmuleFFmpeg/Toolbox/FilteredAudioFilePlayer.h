//
//  FilteredAudioFilePlayer.h
//  SmuleFFmpeg
//
//  Created by NI on 18.05.21.
//

#ifndef FilteredAudioFilePlayer_h
#define FilteredAudioFilePlayer_h

#include "AudioQueuePlayer.h"

struct FilteredAudioFilePlayer_t;

typedef struct FilteredAudioFilePlayer_t*		H_FAUDIO_FILE_PLAYER;

typedef void (*faudio_file_filter_callback)(float *input, float *output, int num_samples, void* user_data);

#ifdef __cplusplus
extern "C" {
#endif

H_FAUDIO_FILE_PLAYER 	faudio_file_player_init(void);
void					faudio_file_player_open(H_FAUDIO_FILE_PLAYER h, const char* filePath);
void					faudio_file_player_destroy(H_FAUDIO_FILE_PLAYER h);
void					faudio_file_player_start(H_FAUDIO_FILE_PLAYER h);
void					faudio_file_player_stop(H_FAUDIO_FILE_PLAYER h);
void					faudio_file_player_pause(H_FAUDIO_FILE_PLAYER h);
void					faudio_file_player_resume(H_FAUDIO_FILE_PLAYER h);

void					faudio_file_player_register_filter(H_FAUDIO_FILE_PLAYER h, faudio_file_filter_callback filter, void* user_data);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* FilteredAudioFilePlayer_h */
