//
//  MTapDelayEffect.hpp
//  SmuleFFmpeg
//
// A multi tap delay applies multiple delay amounts to an input signal before delivering it to the output.
// the effect can be described as following
//
// y[n] = x[n - taps[0]] + x[n - taps[1]] + ... x[n - taps[N]]
//  Created by NI on 19.05.21.
//

#ifndef MTapDelayEffect_hpp
#define MTapDelayEffect_hpp

#include <vector>
#include "Effect.hpp"

// put some constraints, 5 second delay should be Ok
#define MAX_TAP_DELAY_SECONDS				5
#define MAX_TAP_DELAY_MILLISECONDS			(MAX_TAP_DELAY_SECONDS * 1000)
#define MAX_FREQUENCY						96000

static const int delay_buf_sample_capacity =(MAX_TAP_DELAY_SECONDS * MAX_FREQUENCY);
static const int delay_buf_size =			(delay_buf_sample_capacity * sizeof(float));

class MultiTapDelayEffect : public BaseEffect {
public:
	MultiTapDelayEffect();
	// tap[n] describes a delay amount in samples.
	// delay input by each element in taps and sum into the output. fractional delay not required.
	MultiTapDelayEffect(std::vector<int> tapSampleOffset);
	// This is added as a more natural specification for taps as delay in milliseconds
	MultiTapDelayEffect(std::vector<float>& tapDelay);
	// Destructor is not required here, in a time-critical project could be removed
	virtual 		~MultiTapDelayEffect() {}
	
	void 			process(float *input, float *output, int num_frames);
	
	virtual void 	setFrequency(float newFrequency);
	
	virtual void 	setParameters(void* parameterBuffer, int buferLength);
	float			getMaxTapDelayInMilliseconds() {return MAX_TAP_DELAY_MILLISECONDS;}
	float			getTapNumber() {return tapDelay.size();}
private:
	void			recalculateTaps();
private:
	std::vector<float> 			tapDelay;		// tap delay in milliseconds
	std::vector<int> 			taps;			// tap delay in number of samples used for efficiency in process
	//  This is our sample buffer
	int							delayBufferNumSamples;
	int							delayBufferHead;
	float						delayBuffer[delay_buf_size];
};


#endif /* MTapDelayEffect_hpp */
