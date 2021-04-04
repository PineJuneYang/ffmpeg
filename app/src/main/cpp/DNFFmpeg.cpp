//
// Created by dell on 2021/1/28.
//
#pragma once

#include <cstring>
#include "DNFFmpeg.h"
#include "macro.h"
#include "AudioChannel.h"
#include "VideoChannel.h"
#include <pthread.h>


//如同子线程的run方法
void *taskPrepare(void *arg) {
    DNFFmpeg *dnfFmpeg = static_cast<DNFFmpeg *>(arg);
    //这里就执行ffmpeg准备工作
    dnfFmpeg->_prepare();
    return 0;

}

DNFFmpeg::DNFFmpeg(JavaCallHelper *javaCallHelper, const char *dataResource) {
    this->javaCallHelper = javaCallHelper;
    //防止dataResource参数,指向的内存被回收
    //strlen获取字符串的长度不包含\0
    this->dataResource = new char[strlen(dataResource) + 1];
    strcpy(this->dataResource, dataResource);

}

DNFFmpeg::~DNFFmpeg() {

    DELETE(dataResource)
    DELETE(javaCallHelper);
}


void DNFFmpeg::prepare() {
    //开启子线程
    pthread_create(&pid, 0, taskPrepare, this);
}


void DNFFmpeg::_prepare() {
    //ffmpeg网络初始化
    avformat_network_init();
//        //1.打开媒体地址(文件地址,直播地址)
//        // AVFormatContext  包含了 视频的 信息(宽、高等)
    avFormatContext =  0;
//        //返回0表示成功
    int avResponse = avformat_open_input(&avFormatContext, dataResource, 0, 0);
    if (avResponse) {
//            LOGE("打开多媒体失败:%s",av_err2str(avResponse));
        javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        return;
    }
    //2.查找媒体中的音视频(给avFormatContext里面的stream成员赋值)
    avResponse = avformat_find_stream_info(avFormatContext, 0);
    //小于0 则失败
    if (avResponse < 0) {
        LOGE("查找媒体中的音视频失败:%s", av_err2str(avResponse));
        javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        return;
    }
    //nb_streams:几个流(几段视频/音频)
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        //可能代表是一个视频,也有可能代表是一个音频
        AVStream *avStream = avFormatContext->streams[i];
        //获取流的参数,这里面有这段流的各种参数信息(宽高,码率,帧率)
        AVCodecParameters *avCodecParameters = avStream->codecpar;
        //无论是视频还是音频首先都需要获取解码器
        //3.通过当前流使用的编码方式,查找解码器
        AVCodec *avCodec = avcodec_find_decoder(avCodecParameters->codec_id);
        if (avCodec == NULL) {
            LOGE("获取媒体中的解码器失败:%s", av_err2str(avResponse));
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            return;
        }
        //2、获得解码器上下文
        AVCodecContext *context = avcodec_alloc_context3(avCodec);
        if (context == NULL) {
            LOGE("创建解码上下文失败:%s", av_err2str(avResponse));
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            return;
        }

        //4 设置上下文的一些参数
        avResponse = avcodec_parameters_to_context(context, avCodecParameters);
        if (avResponse < 0) {
            LOGE("设置生下文一些参数失败:%s", av_err2str(avResponse));
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            return;
        }

        //5打开解码器
        avResponse = avcodec_open2(context, avCodec, 0);
        if (avResponse) {
            LOGE("打开解码器失败:%s", av_err2str(avResponse));
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            return;

        }

        //pts是度量解码后应该间隔的最小时间单位,只不过他的单位就是time_base ,而不是秒 需要乘以time_base转换成秒
        AVRational time_base  =avStream->time_base;
        if (avCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            ////音频流数据包(编码后的数据包)->经过解码后的PCM一帧一帧的脉冲数据包->在C中创建播放器
            audioChannel = new AudioChannel(i, context,time_base);
        } else if (avCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {

            //获得视频帧率fps 单位时间内需要显示多少个图像
            AVRational avRational  =  avStream->avg_frame_rate;
            int fps= av_q2d(avRational);


            //视频流->解码数据包->经过解码后的YUV/RGB一帧一帧的数据包->渲染
            videoChannel = new VideoChannel(i, context,time_base,fps);
            if (audioChannel){
                videoChannel->setAudioChannel(audioChannel);
            }

            if (callback) {
                //将数据回调出去
                videoChannel->setRenderFrameCallback(callback);
            }
        }


    }

    //没有音视频  (很少见)
    if (!audioChannel && !videoChannel) {
        LOGE("没有音视频");
        javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        return;
    }
    // 准备完了 通知java 你随时可以开始播放
    javaCallHelper->onPrepare(THREAD_CHILD);

}
//}

void *preparePlay(void *args) {

    DNFFmpeg *dnfFmpeg = static_cast<DNFFmpeg *>(args);

    dnfFmpeg->_preparePlay();

    //2.解码

    return 0;

}


void DNFFmpeg::_preparePlay() {
    int avResponse;
    //1.读取媒体数据包(音视频数据包)
    while (isPlaying) {
        AVPacket *avPacket = av_packet_alloc();
        //从媒体流读取了一帧数据,给AVPACKET
        avResponse = av_read_frame(avFormatContext, avPacket);
        //等于0成功,其他就是失败
        if (avResponse == 0) {

            if (avPacket->stream_index ==AVMEDIA_TYPE_AUDIO) {
                //如果是音频
                audioChannel->avPackets.push(avPacket);

            } else if (avPacket->stream_index == AVMEDIA_TYPE_VIDEO) {
                videoChannel->avPackets.push(avPacket);

            }


        } else if (avResponse == AVERROR_EOF) {  //因为是异步的读取完成但是还没播放完

        } else {

        }


    }


}


void DNFFmpeg::start() {
    //向队列里塞需要解压的音视频   (因为java层级调用在主线程调用,所以这里还是要开线程)
    isPlaying = 1;
    if (videoChannel) {
        videoChannel->avPackets.setWork(1);
        videoChannel->play();
    }


    //启动声音的解码与播放
    if (audioChannel){
        audioChannel->avPackets.setWork(1);
        audioChannel->play();
    }
    //启动线程
    pthread_create(&pidStart, 0, preparePlay, this);

}


void DNFFmpeg::setRenderFrameCallback(RenderFrameCallback renderFrameCallback) {
    this->callback = renderFrameCallback;

}











