/**
 * Decoder using Wii U's h264.rpl
 * Created 2020 by GaryOderNichts
 */ 

#ifndef H264_WIIU_H
#define H264_WIIU_H

#include <wut.h>
#include <h264/decode.h>

#include <libswscale/swscale.h>
#include "avcodec.h"

typedef struct
{
    // decoder memory
    void* decoder;

    // framebuffer
    void* framebuffer;
    int framebuffer_size;
} WIIUContext;

#endif /* H264_WIIU_H */