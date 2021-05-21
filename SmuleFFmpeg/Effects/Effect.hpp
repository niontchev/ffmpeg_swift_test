//
//  Effect.hpp
//  SmuleFFmpeg
//
//  Created by NI on 19.05.21.
//

#ifndef Effect_hpp
#define Effect_hpp

#include <stdio.h>

// TODO => use neon or vDSP
#define CLIP(x, min, max)				(x) < (min) ? (min) : ((x) > (max) ? (max) : x)

class BaseEffect {
public:
	BaseEffect() :
				enabled(1),
				frequency(22050.0f),
				wetMix(0.5f)
	{
	}
	// it is virtual, to allow propper destruction of the
	// child classes through the base class pointer
	virtual ~BaseEffect() {};
	// effect processing
	virtual void process(float *input, float *output, int num_frames) = 0;
	// on frequency change
	virtual void setFrequency(float newFrequency) = 0;
	// effect specific parameter change
	// the caller should know the format
	// Unfortunatlly there is no time for this assignment for better solution
	// Should be parameter structure with type definitions
	virtual void setParameters(void* parameterBuffer, int buferLength) = 0;
	// Resets for start of a new stream, clear all the buffers
	// from the current stream
	virtual void reset() = 0;
	
	float		getFrequency() 	{return frequency;}
	void		setMix(float mix) { wetMix = CLIP(mix, 0.0f, 1.0f); }
	float		getMix() 		{return wetMix;}
	void		setEnabled(int enabled) {this->enabled = enabled;}
	int 		isEnabled() {return enabled;}
protected:
	int 						enabled;
	float						frequency;
	float						wetMix;	// 0 - dry, 1 - wet
};


#endif /* Effect_hpp */
