//
// Created by dell on 2021/2/7.
//

#ifndef FFMPEG_BASECHANNEL_H
#define FFMPEG_BASECHANNEL_H


#include "safe_queue.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
};

class BaseChannel {

public:
    BaseChannel(int index, AVCodecContext *avCodecContext, AVRational time_base);

    virtual ~BaseChannel();

public:

    int id;

    AVCodecContext *avCodecContext;

    SafeQueue<AVPacket *> avPackets;
    SafeQueue<AVFrame *> frames;


    static void releaseAVPacket(AVPacket *&packet);

    static void releaseFrame(AVFrame *&avFrame);

    //纯虚函数  相当于抽象方法
    virtual void play() = 0;

    //解码因为是不停循环的取数据 ,所以也是有个标志
    bool isPlaying;


    SLObjectItf engineObject = 0;
    SLEngineItf engineInterface = 0;
    SLObjectItf outputMixObject = 0;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = 0;
    const SLEnvironmentalReverbSettings reverbSettings =
            SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
    SLObjectItf bqPlayerObject = NULL;
    SLPlayItf bqPlayerInterface = 0;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueueInterface = 0;
    SwrContext *swrContext=0;
    uint8_t * data;
    int fps;
    AVRational time_base;
    double clock;
};


#endif //FFMPEG_BASECHANNEL_H
