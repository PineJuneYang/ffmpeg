//
// Created by dell on 2021/2/7.
//

#include "BaseChannel.h"

extern "C"{
#include <libavcodec/avcodec.h>
}
BaseChannel::BaseChannel(int index, AVCodecContext* context,AVRational time_base) {
    this->id = index;
    this->avCodecContext =context;
    avPackets.setReleaseHandle(releaseAVPacket);
    frames.setReleaseHandle(releaseFrame);
    this->time_base = time_base;
}

BaseChannel::~BaseChannel() {
    //设置回调

    avPackets.clear();

}

void BaseChannel::releaseAVPacket(AVPacket*&packet) {
    if (packet){
        //为什么用指针的指针?
        //为的是修改指针的指向
        av_packet_free(&packet);
        packet=0;
    }

}


void BaseChannel::releaseFrame(AVFrame*&avFrame) {
    if (avFrame){
        //为什么用指针的指针?
        //为的是修改指针的指向
        av_frame_free(&avFrame);
        avFrame=0;
    }

}

void BaseChannel::play() {


}




