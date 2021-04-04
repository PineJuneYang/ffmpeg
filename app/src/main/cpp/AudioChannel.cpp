//
// Created by Administrator on 2018/9/5.
//


#include "AudioChannel.h"



AudioChannel::AudioChannel(int index,AVCodecContext*context, AVRational time_base) : BaseChannel(index,context,time_base) {


    //声道数
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    //采样位
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    //采样率
    out_sample_rate = 44100;

    //44100个16位 44100 * 2
    // 44100*(双声道)*(16位)
    data = static_cast<uint8_t *>(malloc(out_sample_rate * out_channels * out_samplesize));
    memset(data,0,out_sample_rate * out_channels * out_samplesize);



}

AudioChannel::~AudioChannel() {
    if(data){
        free(data);
        data = 0;
    }
}


void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context){

    AudioChannel *audioChannel  = static_cast<AudioChannel *>(context);
    //获得PCM数据
    int  dataSize =   audioChannel->getPcm();
    if (dataSize>0){
        //
        (*bq)->Enqueue(bq,audioChannel->data,dataSize);
    }




}



int AudioChannel::getPcm() {

    int data_size = 0;
    AVFrame *avFrame;
   int ret =  frames.pop(avFrame);
    if (!isPlaying){
        if (ret){
            releaseFrame(avFrame);
        }
        return  data_size;
    }

    //重采样.因为我们设置的播放器的声音信息 是44100 ,双声道 16位  这就有可能跟我们的解码后的脉冲
    // 信号格式不一致,所以需要重采样 例如:48000 8位-> 44100 16位


    // 假设我们输入了10个数据 ，swrContext转码器 这一次处理了8个数据
    // todo 那么如果不加delays(上次没处理完的数据) , 积压  就会一直积压在转码器,最终会导致蹦栈
    int64_t delays = swr_get_delay(swrContext,avFrame->sample_rate);
//    // 将 nb_samples 个数据 由 sample_rate采样率转成 44100 后 返回多少个数据
//    // 10  个 48000 = nb 个 44100
//    // AV_ROUND_UP : 向上取整 1.1 = 2
    int64_t max_samples =  av_rescale_rnd(delays+avFrame->nb_samples,out_sample_rate,avFrame->sample_rate,AV_ROUND_UP);
//
//    //上下文+输出缓冲区+输出缓冲区接收的最大数据量+输入数据+输入数据
//    @return number of samples output per channel, negative value on error
    int samples = swr_convert(swrContext, &data, max_samples,
                              (const uint8_t **)avFrame->data, avFrame->nb_samples);
//    //获得   samples 个   * 2 声道 * 2字节（16位）
    data_size =  samples * out_channels * out_samplesize ;
    //获得frame
    //pts 用于度量解码后应该什么时候被播放出来,单位是timeBase,相对于开始播放
    clock = avFrame->pts*av_q2d(time_base);


    return data_size;


}



void* audio_decode(void* args){
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->audioDecode();
    return 0;
}
void* audio_play(void* args){
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->audioPlay();
    return 0;
}

void AudioChannel::play() {

    //设置为播放状态
    frames.setWork(1);
    isPlaying =1;


    //swrContext的创建就是为了后面对解码后的脉冲数据进行重采样
    //0+输出通道+输出采样位+输出采样率+ 输入通道+输入采样位+输入采样率,0,0
    swrContext = swr_alloc_set_opts(0,AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_S16,44100,
                                    avCodecContext->channel_layout,avCodecContext->sample_fmt,avCodecContext->sample_rate,0,0
    );
    //注意这里千万不要忘记初始化
    swr_init(swrContext);

    //1.解码

    pthread_create(&pid_audio_decode,0,audio_decode,this);
    //2.播放
    pthread_create(&pid_audio_play,0,audio_play,this);



}

void AudioChannel::audioDecode() {

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

void AudioChannel::audioPlay() {
    //这里进行openSLES对脉冲数据PCM播放  (卧槽感觉高大上了)

    /**
        * 1、创建引擎并获取引擎接口
        */
    SLresult result;
    // 1.1 创建引擎 SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.2 初始化引擎  init
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.3 获取引擎接口SLEngineItf engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }


    /**
   * 2、设置混音器
   */
    // 2.1 创建混音器SLObjectItf outputMixObject
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
                                                 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 2.2 初始化混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
//    //todo 2.3 设置混音器  注意 不要设置混音器
//    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
//                                              &outputMixEnvironmentalReverb);
//    if (SL_RESULT_SUCCESS != result) {
//        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
//                outputMixEnvironmentalReverb, &reverbSettings);
//        if (SL_RESULT_SUCCESS != result) {
//            return;
//        }
//
//
////        return;
//    }



    /**
     * 3、创建播放器
     */
    //3.1 配置输入声音信息
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    //pcm数据格式
    //pcm+2(双声道)+44100(采样率)+ 16(采样位)+16(数据的大小)+LEFT|RIGHT(双声道)+小端数据
    //小端数据:
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};

    //3.2  配置音轨(输出)
    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    //需要的接口  操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
//    //3.3 创建播放器
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &slDataSource,
                                          &audioSnk, 1,
                                          ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);


    /**
     * 4、设置播放回调函数
     */
    //获取播放器队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueueInterface);
    //设置回调
    (*bqPlayerBufferQueueInterface)->RegisterCallback(bqPlayerBufferQueueInterface,
                                                      bqPlayerCallback, this);
    /**
     * 5、设置播放状态
     */
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    /**
     * 6、手动激活一下这个回调
     */
    bqPlayerCallback(bqPlayerBufferQueueInterface, this);




}