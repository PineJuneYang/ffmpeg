//
// Created by dell on 2021/1/28.
//

#ifndef FFMPEG_DNFFMPEG_H
#define FFMPEG_DNFFMPEG_H


#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"

extern "C" {
#include <libavformat/avformat.h>
}

typedef void (*RenderFrameCallback)(uint8_t *,int , int, int);
class DNFFmpeg {


public:
    DNFFmpeg(JavaCallHelper *javaCallHelper, const char *dataResource);

    ~DNFFmpeg();


    void prepare();

    void _prepare();

    void start();

    void _preparePlay();

    void setRenderFrameCallback(RenderFrameCallback renderFrameCallback);


private:
    char *dataResource;
    pthread_t pid;
    pthread_t pidStart;
    AVFormatContext *avFormatContext;
    JavaCallHelper *javaCallHelper;

    AudioChannel *audioChannel = NULL;
    VideoChannel *videoChannel = NULL;
    bool isPlaying;

    RenderFrameCallback callback;


};


#endif //FFMPEG_DNFFMPEG_H
