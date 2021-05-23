//
//  MTapDelayEffect_c_bridge.h
//  SmuleFFmpeg
//
//	This is C bridge to MultiTapDelayEffect required for Swift interoperability
//
//  Created by NI on 19.05.21.
//

#ifndef MTapDelayEffect_c_bridge_h
#define MTapDelayEffect_c_bridge_h

#ifdef __cplusplus
extern "C" {
#endif

void*			mt_delay_init(void);
void			mt_delay_destroy(void* mt_handle);
void			mt_delay_set_frequency(void* mt_handle, float new_frequency);
void			mt_delay_set_taps(void* mt_handle, int number_of_taps, float total_delay_ms);
float			mt_delay_get_max_delay_in_milliseconds(void* mt_handle);
void 			mt_delay_process(void* mt_handle, float *input, float *output, int num_frames);
void			mt_delay_reset(void* mt_handle);
void*			mt_delay_get_audio_file_filter_callback();
int				mt_delay_get_tap_number(void* mt_handle);
void			mt_delay_set_wet(void* mt_handle, float wet_value);
float			mt_delay_get_wet(void* mt_handle);
void			mt_delay_set_enabled(void* mt_handle, int enabled);
int				mt_delay_get_enabled(void* mt_handle);
void			mt_delay_set_attenuation(void* mt_handle, float attenuation); // 0.25 to 1.0
float			mt_delay_get_attenuation(void* mt_handle);
void			mt_delay_set_enable_compressor(void* mt_handle, int enable);
int				mt_delay_is_compressor_enabled(void* mt_handle);

// this returns NULL to make Swift compiler happy
void*			null_pointer();

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* MTapDelayEffect_c_bridge_h */
