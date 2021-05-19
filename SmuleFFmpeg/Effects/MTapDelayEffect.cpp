//
//  MTapDelayEffect.cpp
//  SmuleFFmpeg
//
//  Created by NI on 19.05.21.
//

#include "MTapDelayEffect.hpp"

MultiTapDelayEffect::MultiTapDelayEffect() : BaseEffect(),
								delayBufferNumSamples(0),
								delayBufferHead(0)
{
}

MultiTapDelayEffect::MultiTapDelayEffect(std::vector<int> tapSampleOffset) : BaseEffect(),
																delayBufferNumSamples(0),
																delayBufferHead(0) {
	float delay_ms;
	taps = tapSampleOffset;
	// synch the milliseconds delay as well
	for (int& tap : taps) {
		delay_ms = (float)tap * 1000.0 / frequency;
		tapDelay.push_back(delay_ms);
	}
}

MultiTapDelayEffect::MultiTapDelayEffect(std::vector<float>& tapDelay) : BaseEffect(),
															delayBufferNumSamples(0),
															delayBufferHead(0) {
	this->tapDelay = tapDelay;
	recalculateTaps();
}

void MultiTapDelayEffect::process(float *input, float *output, int num_frames) {
	// process the input sample by sample
	for (int i = 0; i < num_frames; ++i) {
		float effectSample = 0.0f;
		std::size_t size = taps.size();
		// For time deficiency in this assignment and simplicity
		// the amplitude attenuation of each sample is calculated
		// as power of 2 based ot sample's index.
		// It would be better if it's user controlled through a parameter
		float attenuation_multiplier = .5f;
		float attenuation = 1.0f; // start attenuation
		// we need the tap index in order to use it in attenuation
		for(std::size_t tapVectorIndex = 0; tapVectorIndex < size; ++tapVectorIndex) {
			int tap = taps[tapVectorIndex];
			// do we have enough samples in the buffer
			if (tap <= delayBufferNumSamples) {
				int tapBufferIdx = (delayBufferHead + delay_buf_sample_capacity - tap) % delay_buf_sample_capacity;
				effectSample += delayBuffer[tapBufferIdx] * attenuation;
			}
			attenuation *= attenuation_multiplier;
		}
	
		// store the current sample and increment the head
		delayBuffer[delayBufferHead] = input[i];
		delayBufferHead = (delayBufferHead + 1) % delay_buf_sample_capacity;
		if (delayBufferNumSamples < delay_buf_sample_capacity)
			++delayBufferNumSamples;
		
		//prepare the output and mix input with processed signal
		if (enabled)
			output[i] = effectSample * wetMix + input[i] * (1.0f - wetMix);
		else
			output[i] = input[i];
	}
}

void MultiTapDelayEffect::setFrequency(float newFrequency) {
	frequency = newFrequency;
	recalculateTaps();
}

// In our specific case parameterBuffer contains a pointer to std::vector of tap
// delays in millisseconds
void MultiTapDelayEffect::setParameters(void* parameterBuffer, int buferLength) {
	if (parameterBuffer && buferLength) {
		tapDelay = *static_cast<std::vector<float>*>(parameterBuffer);
		recalculateTaps();
	}
}

void MultiTapDelayEffect::recalculateTaps() {
	if (taps.size() != tapDelay.size())
		taps.resize(tapDelay.size());
	std::size_t size = tapDelay.size();
	for(std::size_t i = 0; i < size; ++i) {
		taps[i] = tapDelay[i] * frequency / 1000.0f; // tapDelay is in milliseconds, convert to seconds
	}
}
