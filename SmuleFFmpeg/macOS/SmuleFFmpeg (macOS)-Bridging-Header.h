//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#include "MTapDelayEffect_c_bridge.h"

void decompressAudioFile(const char* filePath);

// Audio File Player
struct AudioFilePlayer_t* 	audio_file_player_init(void);
int  audio_file_player_open(struct AudioFilePlayer_t* h, const char* filePath);
void audio_file_player_destroy(struct AudioFilePlayer_t* h);
void audio_file_player_start(struct AudioFilePlayer_t* h);
void audio_file_player_stop(struct AudioFilePlayer_t* h);
void audio_file_player_pause(struct AudioFilePlayer_t* h);
void audio_file_player_resume(struct AudioFilePlayer_t* h);
void audio_file_player_register_filter(struct AudioFilePlayer_t* h, void* filter, void* user_data);
