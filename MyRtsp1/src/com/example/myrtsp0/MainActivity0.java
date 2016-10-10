package com.example.myrtsp0;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.util.Queue;

import android.app.Activity;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.MediaController;
import android.widget.VideoView;

public class MainActivity0 extends Activity implements 
MediaPlayer.OnErrorListener, MediaPlayer.OnCompletionListener,SurfaceHolder.Callback  {
 
	static {
		System.loadLibrary("x264");
		System.loadLibrary("live555");
		System.loadLibrary("rtspclient");
	}
	
	public native void RtspClient(String program, String sdp);
	public native void RtspEnd();
	
	private Surface surface;
	private SurfaceView surfaceView;
	MediaCodec  mediaCodecd;
	private int width, height;
	private Button startBtn;
	private Button stopBtn;
	VideoView videoView;
	
	public static boolean istarted;
	public static boolean istarted0;
	private MediaController mMediaController;
    public static PacketQueue av_queue;
    public Frame av_frame_w;
	public Frame av_frame_r, av_frame_r0;
	
	VideoThread videoThread;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
        
		surfaceView = (SurfaceView) findViewById(R.id.camera_surface);
		surface = surfaceView.getHolder().getSurface(); 
		
		startBtn = (Button) findViewById(R.id.conver_btn);
		startBtn.setOnClickListener(new OnClickListener() {
			@Override
		    public void onClick(View v){
				// Do something in response to button click
				Log.w("MainActivity", "--------------start video view------------");
				//videoView.start();
				//rtsp://218.204.223.237:554/live/1/67A7572844E51A64/f68g2mj7wjua3la7.sdp
				final String sdppath = "rtsp://218.204.223.237:554/live/1/67A7572844E51A64/f68g2mj7wjua3la7.sdp";
				final String programname = "rtspclient";
				
				MediaCodecDecodeInit();
				istarted = true;
				MainActivity0.istarted0 = false;
				
				videoThread = new VideoThread();
	    	    videoThread.start();
	    	    
				new Thread(new Runnable() { 
					@Override 
		            public void run() { 
		                // TODO Auto-generated method stub  
						Log.w("MainActivity0", " start thread for RtspClient  ");
						RtspClient(programname, sdppath);  
		            } 
		        }).start();
		    }
		});
		
		stopBtn = (Button) findViewById(R.id.conver_btn0);
		stopBtn.setOnClickListener(new OnClickListener() {
			@Override
		    public void onClick(View v){
				// Do something in response to button click
				Log.w("MainActivity0", "--------------pause video view------------");
				//videoView.pause();
				istarted = false;
				RtspEnd();
				close();
				if(videoThread != null){
				  videoThread.VideoStop();
				}
		    }
		});
		
		videoView = (VideoView)this.findViewById(R.id.conver_vv);
		//PlayRtspStream();
	}

	private void PlayRtspStream(){
		String rtspUrl = "rtsp://218.204.223.237:554/live/1/67A7572844E51A64/f68g2mj7wjua3la7.sdp";

		 //Create media controller
        mMediaController = new MediaController(MainActivity0.this);
        videoView.setMediaController(mMediaController);
        
		videoView.setVideoURI(Uri.parse(rtspUrl));
		videoView.requestFocus();
	}
	
	public void MediaCodecDecodeInit(){
		Log.w("MainActivity0", "--------------MediaCodecDecodeInit--------------");
		String type = "video/avc";
		mediaCodecd = MediaCodec.createDecoderByType(type);  
		MediaFormat mediaFormat = MediaFormat.createVideoFormat(type, 352, 288);  
		mediaCodecd.configure(mediaFormat, surface, null, 0);  
		mediaCodecd.start(); 
	}	
	
	public void close() {
	    try {
	    	if(mediaCodecd != null){
	    	   mediaCodecd.stop();
	    	   mediaCodecd.release();
	    	   mediaCodecd = null;
	    	   Log.w("MainActivity0", "--------------close--------------");
	    	}
	    } catch (Exception e){ 
	        e.printStackTrace();
	    }
	}
	
	class VideoThread extends Thread
	{
		  boolean _istarted;
		  public VideoThread()
		  {
			  _istarted = true;  
		  }
		  
		  public void VideoStop(){
			  _istarted = false;
		  }
		  
		  @Override
		  public void run(){
			  while(_istarted)
			  {
				  //if(MainActivity0.istarted0){
					synchronized (av_queue)
					{
						try {
							av_queue.wait();
						} catch (InterruptedException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
						if(!av_queue.isEmpty()){
							av_frame_r = (Frame) av_queue.get();
						    Log.w("MainActivity0", "video "+av_frame_r.frame_size);
		 				    if(av_frame_r.frame_size>0){
		 				       onFrame0(av_frame_r.frame, av_frame_r.frame_size);   
		 				    }
						}
					}
					try {
						//Log.w("MainActivity0", "----------Thread.sleep(10) 00--------");
						Thread.sleep(10);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				 //}
			  }
		  }			   
	}
	
	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		close();
	}
	
	//监听MediaPlayer上报的错误信息

	@Override
	public boolean onError(MediaPlayer mp, int what, int extra) {
	  // TODO Auto-generated method stub
	  return false;
	}

	class Frame{	
		public byte[] frame;
		public int frame_size;
		
		public Frame(byte[] frame, int frame_size){
			this.frame = frame;
			this.frame_size = frame_size;
		}
	}
	
	/*
	   LinkedList queue = new LinkedList();  
	    Object object = "";  
	    queue.add(object);  
	    Object o = queue.removeFirst();  
	    queue = (LinkedList) Collections.synchronizedList(queue);  
	*/
	
	//将LinkedList转换成ArrayList
	//ArrayList<String> arrayList = new ArrayList<String>(linkedList);  
	
	//将LinkedList全部换成ConcurrentLinkedQueue试试，LinkedList是线程不安全的。
	//List<String> linkedList = Collections.synchronizedList(new LinkedList<String>())
	class PacketQueue {  
		  private LinkedList<Object> list = new LinkedList<Object>();  
		  
		  public void put(Object v) {  
		    list.addFirst(v);  
		  }  
		  
		  public Object get() {  
		    return list.removeLast();  
		  } 
		  
		  public boolean isEmpty() {  
		    return list.isEmpty();  
		  } 
		  
		  public int size() {  
		    return list.size();  
		  }  
		  
		  public boolean destory() {  
			 int size = list.size();
			 for(int i=0; i<size; i++){
				 list.remove(i);
			 }
			 //list.clear();
			 //list.peek();
			 //list.remove();
			 return true;
		  }  
	} 
	
	public class MyQueue<T> {
	    private Queue<T> storage = new LinkedList<T>();

	    /** 将指定的元素插入队尾 */
	    public void offer(T v) {
	        storage.offer(v);
	    }

	    /** 检索，但是不移除队列的头，如果此队列为空，则返回 null */
	    public T peek() {
	        return storage.peek();
	    }

	    /** 检索，但是不移除此队列的头 */
	    /** 此方法与 peek 方法的惟一不同是，如果此队列为空，它会抛出一个异常 */
	    public T element() {
	        return storage.element();
	    }

	    /** 检索并移除此队列的头，如果队列为空，则返回 null */
	    public T poll() {
	        return storage.poll();
	    }

	    /** 检索并移除此队列的头 */
	    /** 此方法与 poll 方法的不同在于，如果此队列为空，它会抛出一个异常 */
	    public T remove() {
	        return storage.remove();
	    }

	    /** 队列是否为空 */
	    public boolean empty() {
	        return storage.isEmpty();
	    }

	    /** 打印队列元素 */
	    public String toString() {
	        return storage.toString();
	    }
	}
	
	private int mCount;
	private final static int FRAME_RATE = 15; 
	// decoder
    public boolean onFrame(byte[] buf, int length) {   
    	   Log.w("MainActivity0", "onFrame start "+ length);
    	   synchronized(av_queue)
    	   {
		 
    	   if(av_queue==null){
    		   Log.w("MainActivity0", " -------------create queue---------------");
    		   av_queue = new PacketQueue();
    		   //MainActivity0.istarted0 = true;
    	   }
    	   av_frame_w = new Frame(buf, length);
    	   av_queue.put(av_frame_w);
    	   
    	   Log.w("MainActivity0", "onFrame end "+ av_queue.size());
    	   //MainActivity0.istarted0 = true;
    	   av_queue.notify();
    	   //av_frame_r0 = (Frame) av_queue.get();
		   //Log.w("MainActivity0", "onFrame "+av_frame_r0.frame_size+", "+av_queue.size());
    	   }
	       return true;
	}
	 
    public synchronized  boolean onFrame0(byte[] buf, int length) {   
 	   Log.w("MainActivity0", "onFrame0 start"+ length);
 	   ByteBuffer[] inputBuffers = mediaCodecd.getInputBuffers();  
	       int inputBufferIndex = mediaCodecd.dequeueInputBuffer(-1);  
	       if (inputBufferIndex >= 0) {  
	            ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];  
	            inputBuffer.clear();  
	            inputBuffer.put(buf, 0, length);  
	            mediaCodecd.queueInputBuffer(inputBufferIndex, 0, length,
	            		mCount * 1000000 / FRAME_RATE, 0);  
	            mCount++;  
	       }  
	       Log.w("MainActivity0", "onFrame0 = "+mCount);
	       
	       MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();  
	       int outputBufferIndex = mediaCodecd.dequeueOutputBuffer(bufferInfo,0);  
	       while (outputBufferIndex >= 0) {  
	           mediaCodecd.releaseOutputBuffer(outputBufferIndex, true);  
	           outputBufferIndex = mediaCodecd.dequeueOutputBuffer(bufferInfo, 0);  
	       }
	       return true;
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
	
	@Override
	protected void onDestroy() {
	    if(istarted){
	    	Log.w("MainActivity0", "--------------onDestroy-----------");
			istarted = false;
			RtspEnd();
	    }
	    super.onDestroy();
	}
}
