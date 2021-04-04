package com.jn.ffmpeg;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {


    private DNPlayer player;
    private TextView tv;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tv = findViewById(R.id.play);
        SurfaceView surfaceView = findViewById(R.id.surfaceview);

        player = new DNPlayer(surfaceView);

        player.setDataUrl("rtmp://58.200.131.2:1935/livetv/cctv1");

        player.setListener(new DNPlayer.OnprepareListener() {
            @Override
            public void onPrepare() {

                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        tv.setText("FFMPEG准备完毕");
                        Toast.makeText(MainActivity.this,"FFMPEG准备完毕",Toast.LENGTH_LONG).show();
                        player.start();
                    }
                });

            }
        });
        player.prepare();
    }



    public void play(View view) {


        player.start();

    }
}
