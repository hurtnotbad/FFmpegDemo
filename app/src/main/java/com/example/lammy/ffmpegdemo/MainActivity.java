package com.example.lammy.ffmpegdemo;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.annotation.RequiresApi;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

import static com.example.lammy.ffmpegdemo.FFmpegUtil.decode;

public class MainActivity extends AppCompatActivity {


    private  SurfaceView surfaceView;
    private int permissionRequestCode = 1;
    private String inputFilePath = new File(Environment.getExternalStorageDirectory()+ "/a-lammytest/","test.mp4").getAbsolutePath();
    private String outputFilePath = new File(Environment.getExternalStorageDirectory()+ "/a-lammytest/","output_yuv420p.yuv").getAbsolutePath();
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surface_view);
//        requestPermissions();


    }

    private String permissions[] = new String[]{
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE,

    };

    protected void play(View view){

        new Thread(new Runnable() {
            @Override
            public void run() {
                FFmepgVideoPlayer fFmepgVideoPlayer = new FFmepgVideoPlayer(surfaceView);
                fFmepgVideoPlayer.play(inputFilePath);
            }
        }).start();


    }


    private void requestPermissions(){
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            for(int i =0; i < permissions.length ; i ++) {

                if( checkSelfPermission(permissions[0]) == PackageManager.PERMISSION_DENIED){
                    requestPermissions(permissions ,permissionRequestCode );
                    return;
     //               Toast.makeText(this , "请打开"+permissions[0]+"权限",Toast.LENGTH_LONG).show();
                }
            }
            decode(inputFilePath, outputFilePath);
        }else{
            decode(inputFilePath, outputFilePath);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {

        if(requestCode ==permissionRequestCode ){

            for(int result:grantResults){
                if(result == PackageManager.PERMISSION_DENIED){
                    return;
                }
            }

            decode(inputFilePath, outputFilePath);

        }
    }
}
