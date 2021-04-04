#include <jni.h>
#include <string>
#include "macro.h"
#include "DNFFmpeg.h"
#include <android/native_window_jni.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER ;

JavaVM *javaVm;
DNFFmpeg *dnfFmpeg;
ANativeWindow* window;


int JNI_OnLoad(JavaVM *vm, void *r) {
    javaVm = vm;
    return JNI_VERSION_1_6;
}


void render(uint8_t *data,int lineSize,int w, int h){
    pthread_mutex_lock(&mutex);
    if (!window){
        pthread_mutex_unlock(&mutex);
        return;
    }
    //设置窗口属性
    ANativeWindow_setBuffersGeometry(window, w,
                                     h,
                                     WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //填充rgb数据给dst_data
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    // stride：一行多少个数据（RGBA） *4
    int dst_linesize = window_buffer.stride * 4;
    //一行一行的拷贝
    for (int i = 0; i < window_buffer.height; ++i) {
        //memcpy(dst_data , data, dst_linesize);
        memcpy(dst_data + i * dst_linesize, data + i * lineSize, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);




}



extern "C"
JNIEXPORT void JNICALL
Java_com_jn_ffmpeg_DNPlayer_native_1prepare(JNIEnv *env, jobject thiz ,jstring dataUrl) {

    LOGE("kkkkkkkkkkkkkk");

    const char * dataResource =  env->GetStringUTFChars(dataUrl,0);

    JavaCallHelper *javaCallHelper = new JavaCallHelper(javaVm,env,thiz);
    dnfFmpeg = new DNFFmpeg(javaCallHelper,dataResource);
    //未雨绸缪,回调出来一帧rgba数据
    dnfFmpeg->setRenderFrameCallback(render);
    dnfFmpeg->prepare();



//    dnfFmpeg->prepare();

}extern "C"
JNIEXPORT void JNICALL
Java_com_jn_ffmpeg_DNPlayer_native_1start(JNIEnv *env, jobject thiz) {

    dnfFmpeg->start();


}extern "C"
JNIEXPORT void JNICALL
Java_com_jn_ffmpeg_DNPlayer_native_1setSurface(JNIEnv *env, jobject thiz, jobject surface) {

    pthread_mutex_lock(&mutex);
    if (window){
        ANativeWindow_release(window);
        window = 0;

    }

    window =   ANativeWindow_fromSurface(env,surface);
    pthread_mutex_unlock(&mutex);





}