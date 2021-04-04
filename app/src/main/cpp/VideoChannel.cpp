//
// Created by Administrator on 2018/9/5.
//


#include "VideoChannel.h"


extern "C"{
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}



void dropAvPacket(queue<AVPacket*> &q){

    while (!q.empty()){

       AVPacket *avPacket =  q.front();
        if (avPacket->flags!=AV_PKT_FLAG_KEY){
            BaseChannel::releaseAVPacket(avPacket);
            q.pop();
        } else{
            break;
        }

    }

}


void dropAVFrame(queue<AVFrame*> &q){
    if (!q.empty()){
        AVFrame *avFrame =  q.front();
        BaseChannel::releaseFrame(avFrame);
        q.pop();
    }
}


VideoChannel::VideoChannel(int index, AVCodecContext *context, AVRational time_base,int fps) : BaseChannel(index, context,time_base) {

    this->fps = fps;
    //使用函数指针操作 主要是考虑到线程安全
//    avPackets.setSyncHandle(dropAvPacket);
    frames.setSyncHandle(dropAVFrame);

}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
this->audioChannel = audioChannel;

}


VideoChannel::~VideoChannel() {

    frames.clear();
}

void *decode_task(void *args) {


    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);

    videoChannel->_decode();
    return 0;

}
void *render_task(void *arge) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(arge);
    videoChannel->render();
    return 0;
}

void VideoChannel::play() {
    //这里开始取出需要待解码的数据包
    isPlaying = 1;
    frames.setWork(1);
    //1.解码
    pthread_create(&pid_decode, 0, decode_task, this);
    //2.渲染
    //这里已经有一副图像了 这里再开一个线程来渲染出来
    //开一个线程解决流畅度,二是保证音视频同步
    pthread_create(&pid_render, 0, render_task, this);


}




void VideoChannel::_decode() {
    //1.解码
    AVPacket *avPacket = 0;
    while (isPlaying) {
        int ret = avPackets.pop(avPacket);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            //没有取出成功继续下一次取出
            continue;
        }
        //把数据包丢给解码器(解码器也就像一个黑箱管道)
        ret = avcodec_send_packet(avCodecContext, avPacket);
        releaseAVPacket(avPacket);

        if (ret && ret != AVERROR(EAGAIN)) {
            break;
        }
        //avframe 代表一帧图像
        AVFrame *frame = av_frame_alloc();
        //从解码器中读取解码后的图像
        ret = avcodec_receive_frame(avCodecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret) {
            break;
        }


        frames.push(frame);


    }

    releaseAVPacket(avPacket);


}


void VideoChannel::render() {
    //将YUV->RGBA
    SwsContext *  context=  sws_getContext(avCodecContext->width,avCodecContext->height,
            avCodecContext->pix_fmt,avCodecContext->width,avCodecContext->height,
                AV_PIX_FMT_RGBA,SWS_BILINEAR,0,0,0

            );
    //每个画面刷新的间隔
     double frame_delay = 1.0/fps;
    AVFrame* avFrame = 0;
    //指针数组
    uint8_t * dst[4];
    int dstStride[4];

     av_image_alloc(dst, dstStride,
                   avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA, 16);

    while (isPlaying){

       int ret =  frames.pop(avFrame);

       if (!isPlaying){
           break;
       }
       sws_scale(context,avFrame->data,avFrame->linesize,0,avCodecContext->height,dst,dstStride);


        double clock =  avFrame->best_effort_timestamp*av_q2d(time_base);

        //额外的间隔时间 ,当在解码的时候  应该延迟  this signals how much the picture must be delayed.
//        double extra_delay = avFrame->repeat_pict / (2 * fps);
        double extra_delay = 0;
        // 真实需要的间隔时间
        double delays = extra_delay + frame_delay;
        if (!audioChannel){

            av_usleep(delays*1000*1000);

        }else{

            if (clock==0){

                av_usleep(delays*1000*1000);


            } else{

                //拿到音频的Clock
                int audioDelayClock = audioChannel->clock;

                double  diff = clock-audioDelayClock;
                //大于0表示视频比较快
                //小于0 表示音频比较快
                if (diff>0){

                    av_usleep((delays+diff)*1000*1000);

                } else if(diff<0){
                    //不睡了  直接渲染 但是有可能还是追不上音频
                    //所以如果还是追不上  ,那就需要视频丢包
                    if (fabs(diff)>=0.05){
                        //丢包  1.可以丢avPacket数据包,也有可以选择avframe

                        frames.sync();
                        continue;

                    }



                }
            }



        }



       //休眠  解决不会卡顿

       callback(dst[0],dstStride[0],avCodecContext->width, avCodecContext->height);
       releaseFrame(avFrame);
    }
    av_freep(&dst[0]);
    releaseFrame(avFrame);

}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback frameCallback) {
    this->callback = frameCallback;
}

