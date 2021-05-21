//
//  MTapDelayEffect_c_bridge.cpp
//  SmuleFFmpeg
//
//  Created by NI on 19.05.21.
//

#include "MTapDelayEffect_c_bridge.h"
#include "MTapDelayEffect.hpp"
#include <vector>


void* mt_delay_init(void) {
	std::vector<float> taps = {200.0f};
	MultiTapDelayEffect* effect = new MultiTapDelayEffect(taps);
	return effect;
}

void mt_delay_destroy(void* mt_handle) {
	delete static_cast<MultiTapDelayEffect*>(mt_handle);
}

void mt_delay_set_frequency(void* mt_handle, float new_frequency) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	effect->setFrequency(new_frequency);
}

void mt_delay_set_taps(void* mt_handle, int number_of_taps, float total_delay_ms) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	if (total_delay_ms <= effect->getMaxTapDelayInMilliseconds() && number_of_taps > 0 && total_delay_ms > 0.0f) {
		std::vector<float> taps;
		float inter_delay = total_delay_ms / (float)(number_of_taps + 1);
		float delay = inter_delay;
		for (int i = 0; i < number_of_taps; ++i, delay += inter_delay)
			taps.push_back(delay);
		effect->setParameters(&taps, 1 /* this is just a hack, indicating there is data
										setParameters needs to be reworked*/);
	}
}

float mt_delay_get_max_delay_in_milliseconds(void* mt_handle) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	return effect->getMaxTapDelayInMilliseconds();
}

int mt_delay_get_tap_number(void* mt_handle) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	return effect->getTapNumber();
}

void mt_delay_set_wet(void* mt_handle, float wet_value) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	effect->setMix(wet_value);
}

float mt_delay_get_wet(void* mt_handle) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	return effect->getMix();
}

void mt_delay_set_enabled(void* mt_handle, int enabled) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	effect->setEnabled(enabled);
}

int mt_delay_get_enabled(void* mt_handle) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	return effect->isEnabled();
}

void mt_delay_set_attenuation(void* mt_handle, float attenuation) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	effect->setAttenuation(attenuation);
}
float mt_delay_get_attenuation(void* mt_handle) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	return effect->getAttenuation();
}

void mt_delay_process(void* mt_handle, float *input, float *output, int num_frames) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	effect->process(input, output, num_frames);
}

void mt_delay_reset(void* mt_handle) {
	MultiTapDelayEffect* effect = static_cast<MultiTapDelayEffect*>(mt_handle);
	effect->reset();
}

void* mt_delay_get_audio_file_filter_callback() {
	return (void*)mt_delay_process;
}

void* null_pointer() {
	return 0;
}
