//
//  Decompressor.h
//  SmuleFFmpeg (iOS)
//
//  Created by NI on 18.05.21.
//

#ifndef Decompressor_h
#define Decompressor_h

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void decompressAudioFile(const char* filePath);

#ifdef __cplusplus
}
#endif

#endif /* Decompressor_h */
