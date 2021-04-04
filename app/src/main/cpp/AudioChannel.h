//
// Created by Administrator on 2018/9/5.
//

#ifndef PLAYER_AUDIOCHANNEL_H
#define PLAYER_AUDIOCHANNEL_H


//#include "BaseChannel.h"

#include "BaseChannel.h"

class AudioChannel: public BaseChannel {
public:
    AudioChannel(int index,AVCodecContext*context, AVRational time_base);
    ~AudioChannel();

    void play();

    void audioDecode();
    void audioPlay();

    int getPcm();


private:

    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

    int out_channels;
    int out_samplesize;
    int out_sample_rate;


};





#endif //PLAYER_AUDIOCHANNEL_H
