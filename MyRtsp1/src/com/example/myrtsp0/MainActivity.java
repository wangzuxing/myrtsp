package com.example.myrtsp0;

import java.io.File;
import java.io.IOException;

import android.app.Activity;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.MediaController;
import android.widget.VideoView;

public class MainActivity extends Activity implements MediaPlayer.OnErrorListener, MediaPlayer.OnCompletionListener  {

	static {
		System.loadLibrary("x264");	
		System.loadLibrary("rtsp");
	}
	
	
	public native void RtspMp4Start(String sdp, String mp4);
	public native void RtspMp4End();
	
	private Button startBtn;
	private Button stopBtn;
	VideoView videoView;
	
	private MediaController mMediaController;
	 
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
        
		startBtn = (Button) findViewById(R.id.conver_btn);
		startBtn.setOnClickListener(new OnClickListener() {
			@Override
		    public void onClick(View v){
				// Do something in response to button click
				Log.w("MainActivity", "--------------start video view------------");
				videoView.start();
				
				final String sdppath = "rtsp://218.204.223.237:554/live/1/67A7572844E51A64/f68g2mj7wjua3la7.sdp";
				final String mp4Path = Environment.getExternalStorageDirectory() + "/rtsptest0.mp4";
				
				File f = new File(Environment.getExternalStorageDirectory(), "rtsptest_0.mp4");
				try {
					 if(!f.exists()){
					    f.createNewFile();
					 }else{
						if(f.delete()){
						   Log.w("Mp4Activity", " mp4 file create again! ");
						   f.createNewFile();
						}
					}
				} catch (IOException e) {
					 e.printStackTrace();
				}
				
				
				new Thread(new Runnable() { 
					@Override 
		            public void run() { 
		                // TODO Auto-generated method stub  
						Log.w("Mp4Activity", " start thread for RtspMp4Start  ");
						RtspMp4Start(sdppath, mp4Path);
		            } 
		        }).start();
			             
		    }
		});
		
		stopBtn = (Button) findViewById(R.id.conver_btn0);
		stopBtn.setOnClickListener(new OnClickListener() {
			@Override
		    public void onClick(View v){
				// Do something in response to button click
				Log.w("MainActivity", "--------------pause video view------------");
				videoView.pause();
				RtspMp4End();
		    }
		});
		
		videoView = (VideoView)this.findViewById(R.id.conver_vv);
		PlayRtspStream();
	}

	private void PlayRtspStream(){
		String rtspUrl = "rtsp://218.204.223.237:554/live/1/67A7572844E51A64/f68g2mj7wjua3la7.sdp";

		 //Create media controller
        mMediaController = new MediaController(MainActivity.this);
        videoView.setMediaController(mMediaController);
        
		videoView.setVideoURI(Uri.parse(rtspUrl));
		videoView.requestFocus();
	}
	
	//监听MediaPlayer上报的错误信息

	@Override
	public boolean onError(MediaPlayer mp, int what, int extra) {
	  // TODO Auto-generated method stub
	  return false;
	}

	 

	//Video播完的时候得到通知

	@Override
	public void onCompletion(MediaPlayer mp) {
	   //this.finish();
	}
	 
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
}
