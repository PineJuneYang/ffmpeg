package com.jn.ffmpeg;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class DNPlayer implements SurfaceHolder.Callback {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private final SurfaceHolder holder;



    private String dataUrl;


    public void setDataUrl(String dataUrl) {
        this.dataUrl = dataUrl;
    }

    private OnprepareListener listener;


    public void setListener(OnprepareListener listener) {
        this.listener = listener;
    }

    public DNPlayer(SurfaceView surfaceView) {
        holder = surfaceView.getHolder();
        holder.addCallback(this);
    }


    public void onError(int errorCode){
        System.out.println("Java接到回调:"+errorCode);
    }


    public void onPrepare(){
        if (null != listener){
            listener.onPrepare();
        }
    }


    public void prepare(){


        native_prepare(dataUrl);

    }

    public void start(){
        native_start();
    }


    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {




    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

        native_setSurface(surfaceHolder.getSurface());


    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    public interface OnprepareListener{
        void onPrepare();
    }


    native void native_prepare(String dataUrl);
    native void native_start();

    native void native_setSurface(Surface surface);
}
