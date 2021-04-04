package com.jn.ffmpeg;

import android.app.Application;

public class FfmpegApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();

        try {
            Thread.sleep(3000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }


    }
}
