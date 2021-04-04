//
// Created by Administrator on 2018/9/5.
//
#include "BaseChannel.h"
#include "AudioChannel.h"

#ifndef PLAYER_VIDEOCHANNEL_H
#define PLAYER_VIDEOCHANNEL_H


typedef void (*RenderFrameCallback)(uint8_t *,int , int, int);

class VideoChannel:public BaseChannel {
public:
    VideoChannel(int index,AVCodecContext * context, AVRational time_base,int fps);
    ~VideoChannel();

    void play();


    void _decode();


    void render();

    void setAudioChannel(AudioChannel *audioChannel);

    void  setRenderFrameCallback(RenderFrameCallback frameCallback);
    RenderFrameCallback callback;
private:

    pthread_t pid_decode;
    pthread_t pid_render;


    AudioChannel *audioChannel = 0;


};


#endif //PLAYER_VIDEOCHANNEL_H
