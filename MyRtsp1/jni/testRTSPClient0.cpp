/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2015, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include <string.h>
#include <jni.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
*/
#include "x264_config.h"

#define LOG_TAG "rtspclient"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData);
// called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData);
// called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration
//(if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL);
// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);
// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

char eventLoopWatchVariable = 0;

int doRtspClient(const char* program, const char* sdp) {
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

  // We need at least one "rtsp://" URL argument:
  if (program == NULL || sdp == NULL) {
    return 1;
  }

  LOGI("              doRtspClient             ");
  // There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
  //for (int i = 1; i <= argc-1; ++i) {
    openURL(*env, program, sdp);
  //}

  // All subsequent activity takes place within the event loop:
  env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

  return 0;

  // If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
  // and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
  // then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
  /*
    env->reclaim(); env = NULL;
    delete scheduler; scheduler = NULL;
  */
}

JNIEnv *_env;

JNIEXPORT void JNICALL Java_com_example_myrtsp0_MainActivity0_RtspClient
 (JNIEnv *env, jclass clz, jstring program, jstring sdp)
 {
 	const char* sdp_title = env->GetStringUTFChars(sdp, NULL);
 	const char* program_title = env->GetStringUTFChars(program, NULL);
 	_env = env;

 	LOGI("              RtspClientStart              ");
 	LOGI("              RtspClientStart              ");
    signed char ret = doRtspClient(program_title, sdp_title);

    LOGI("              RtspClientStart = %d              ", ret);

 	env->ReleaseStringUTFChars(sdp, sdp_title);
 	env->ReleaseStringUTFChars(program, program_title);
 }

//视频录制结束调用
JNIEXPORT void Java_com_example_myrtsp0_MainActivity0_RtspEnd
(JNIEnv *env, jclass clz)
{
	eventLoopWatchVariable = 1;
	LOGI("              RtspClientEnd              ");
	LOGI("              RtspClientEnd              ");
}

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
  StreamClientState();
  virtual ~StreamClientState();

public:
  MediaSubsessionIterator* iter;
  MediaSession* session;
  MediaSubsession* subsession;
  TaskToken streamTimerTask;
  double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient: public RTSPClient {
public:
  static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
				  int verbosityLevel = 0,
				  char const* applicationName = NULL,
				  portNumBits tunnelOverHTTPPortNum = 0);

protected:
  ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
  virtual ~ourRTSPClient();

public:
  StreamClientState scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

class DummySink: public MediaSink {
public:
  static DummySink* createNew(UsageEnvironment& env,
			      MediaSubsession& subsession, // identifies the kind of data that's being received
			      char const* streamId = NULL); // identifies the stream itself (optional)

private:
  DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
    // called only by "createNew()"
  virtual ~DummySink();

  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
				struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			 struct timeval presentationTime, unsigned durationInMicroseconds);

private:
  // redefined virtual functions:
  virtual Boolean continuePlaying();

private:
  u_int8_t* fReceiveBuffer;
  MediaSubsession& fSubsession;
  char* fStreamId;
};

/*
static int sws_flags = SWS_BICUBIC;

class CDecodeCB
{
  public:
      virtual void videoCB(int width, int height, uint8_t* buff, int len)=0;
};

class CFfmpegDecode
{
public:
	 CFfmpegDecode();
	 ~CFfmpegDecode();
	 int initFFMPEG();
	 int openDecoder(int width, int height, CDecodeCB* pCB);
	 int closeDecoder();
	 int decode_rtsp_frame(uint8_t* input,int nLen,bool bWaitIFrame);

private:
	 bool m_bInit;
	 AVCodec *decode_codec;
	 AVCodecContext *decode_c;
	 AVFrame *decode_picture;
	 struct SwsContext *img_convert_ctx;
	 CDecodeCB* m_pCB;
	 int m_nWidth;
	 int m_nHeight;
};

CFfmpegDecode::CFfmpegDecode()
{
    m_bInit = false;
    img_convert_ctx = NULL;
	LOGI("              CFfmpegDecode              ");
}

CFfmpegDecode::~CFfmpegDecode()
{
    av_lockmgr_register(NULL);
    closeDecoder();
	LOGI("              ~CFfmpegDecode              ");
}

int CFfmpegDecode::initFFMPEG()
{
    //m_state = RC_STATE_INIT;
    avcodec_register_all();
    av_register_all();
    //avformat_network_init();
    //if (av_lockmgr_register(lockmgr))
    {
       // m_state = RC_STATE_INIT_ERROR;
     //   return -1;
    }
    LOGI("              initFFMPEG              ");
    return 0;
}

int CFfmpegDecode::openDecoder(int width, int height,CDecodeCB* pCB)
{
    m_nWidth = width;
    m_nHeight = height;
    m_pCB = pCB;
    if (m_bInit)
        return -1;
    decode_codec = avcodec_find_decoder(CODEC_ID_H264);
    if (!decode_codec)
    {
        fprintf(stderr, "codec not found\n");
        return -2;
    }
    LOGI("              openDecoder              ");
    decode_c= avcodec_alloc_context3(decode_codec);
    decode_c->codec_id= CODEC_ID_H264;
    decode_c->codec_type = AVMEDIA_TYPE_VIDEO;
    decode_c->pix_fmt = PIX_FMT_YUV420P;
    decode_picture= avcodec_alloc_frame();
    if (avcodec_open2(decode_c, decode_codec, NULL) < 0)
    {
     //  fprintf(stderr, "could not open codec\n");
       return -3;
    }
    LOGI("           openDecoder successful         ");
    m_bInit = true;
    return 0;
}

int CFfmpegDecode::closeDecoder()
{
    if(decode_c) {
        avcodec_close(decode_c);
        av_free(decode_c);
    }
    if(decode_picture)
        av_free(decode_picture);
    m_bInit = false;
}

int CFfmpegDecode::decode_rtsp_frame(uint8_t* input,int nLen,bool bWaitIFrame)
{
    if(!m_bInit)
        return -1;
    if(input == NULL || nLen <= 0)
        return -2;
    try{
        int got_picture;
        int size = nLen;
        AVPacket avpkt;
        av_init_packet(&avpkt);
        avpkt.size = size;
        avpkt.data = input;
        // while (avpkt.size > 0)
        {
            int len = avcodec_decode_video2(decode_c, decode_picture, &got_picture, &avpkt);
            if(len == -1){
                return -3;
            }
            if (got_picture)
            {
                int w = decode_c->width;
                int h = decode_c->height;
                int numBytes=avpicture_get_size(PIX_FMT_RGB24, w,h);
                uint8_t * buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

                AVFrame *pFrameRGB = av_frame_alloc;//avcodec_alloc_frame();
                avpicture_fill((AVPicture *)pFrameRGB, buffer,PIX_FMT_RGB24,  w, h);

                img_convert_ctx = sws_getCachedContext(img_convert_ctx,
                                            w, h, (PixelFormat)(decode_picture->format), w, h,PIX_FMT_RGB24, sws_flags, NULL, NULL, NULL);
                if (img_convert_ctx == NULL)
                {
                    fprintf(stderr, "Cannot initialize the conversion context\n");
                    LOGI("           img_convert_ctx NULL         ");
                    //exit(1);
                    return -4;
                }
                sws_scale(img_convert_ctx, decode_picture->data, decode_picture->linesize,
                    0, h, pFrameRGB->data, pFrameRGB->linesize);
                if (m_pCB)
                {
                    m_pCB->videoCB(w, h, pFrameRGB->data[0], numBytes*sizeof(uint8_t));
                }

                LOGI("    decode_rtsp_frame %d   ",avpkt.stream_index);
                av_free(buffer);
                av_free(pFrameRGB);
                av_packet_unref(&avpkt);
                return 0;
                if (avpkt.data)
                {
                    avpkt.size -= len;
                    avpkt.data += len;
                }
            }
            else
            {
                return -5;
            }
            //return 0;
        }
        //return 0;
    }
    catch(...)
    {
    	LOGI("           decode_rtsp_frame error         ");
    }
    return -6;
}
*/
// by default, print verbose output from each "RTSPClient"
#define RTSP_CLIENT_VERBOSITY_LEVEL 1
// Counts how many streams (i.e., "RTSPClient"s) are currently in use.
static unsigned rtspClientCount = 0;

// step 1 RTSPClient
void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL) {
  // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
  // to receive (even if more than stream uses the same "rtsp://" URL).
  RTSPClient* rtspClient = ourRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
  if (rtspClient == NULL) {
    LOGI("Failed to create a RTSP client for URL %s , %s\n", rtspURL,  env.getResultMsg());
    return;
  }

  LOGI("openURL URL %s\n", rtspURL);
  LOGI("openURL URL %s\n", env.getResultMsg());
  ++rtspClientCount;

  // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
  // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
  // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
  rtspClient->sendDescribeCommand(continueAfterDESCRIBE); 
}

// step 2  MediaSession
// Implementation of the RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      LOGI("Failed to get a SDP description: %s \n", resultString);
      delete[] resultString;
      break;
    }

    char* const sdpDescription = resultString;
    LOGI("Got a SDP description: %s\n" ,sdpDescription);

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore

    if (scs.session == NULL) {
      LOGI("Failed to create a MediaSession object from the SDP description: %s \n", env.getResultMsg());
      break;
    } else if (!scs.session->hasSubsessions()) {
      LOGI("This session has no media subsessions \n");
      break;
    }

    LOGI("create a MediaSession object from the SDP description: %s \n", env.getResultMsg());

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(rtspClient);
}

// step 3
// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient* rtspClient) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  
  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL) {
    if (!scs.subsession->initiate()) {
      LOGI("Failed to initiate the scs.subsession, subsession: %s\n", env.getResultMsg());
      setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
    } else {
      //LOGI("Initiated the scs.subsession, subsession\n");
      LOGI("scs.subsession = %s, %s", scs.subsession->mediumName(),scs.subsession->codecName());
      if (scs.subsession->rtcpIsMuxed()) {
    	  LOGI("client port %d \n", scs.subsession->clientPortNum());
      } else {
    	  LOGI("client ports %d, %d \n", scs.subsession->clientPortNum() , scs.subsession->clientPortNum()+1);
      }
      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL) {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  } else {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}

// step 4
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      LOGI("Failed to set up the scs.subsession, subsession: %s \n", resultString);
      break;
    }

    LOGI("Set up the scs.subsession & subsession: %s\n", resultString);
    if (scs.subsession->rtcpIsMuxed()) {
       LOGI("client port %d \n", scs.subsession->clientPortNum());
    } else {
       LOGI("client ports %d , %d\n", scs.subsession->clientPortNum() ,scs.subsession->clientPortNum()+1);
    }

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

    scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) {
    	LOGI("Failed to create a data sink for the scs.subsession, subsession: %s \n", env.getResultMsg());
        break;
    }
    LOGI("Created a data sink for the scs.subsession&subsession： %s \n",env.getResultMsg());
    scs.subsession->miscPtr = rtspClient;
    // a hack to let subsession handler functions get the "RTSPClient" from the subsession
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

// step 5
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;

  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      LOGI("Failed to start playing session: %s \n", resultString);
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay,
    		  (TaskFunc*)streamTimerHandler, rtspClient);
    }

    LOGI("Started playing session");
    if (scs.duration > 0) {
    	LOGI("for up to %f seconds \n", scs.duration);
    }

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}

// step 6
// Implementation of the other event handlers:
void subsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  LOGI("   subsessionAfterPlaying   \n");

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

// step 7
void subsessionByeHandler(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  LOGI("Received RTCP BYE on subsession, subsession\n");

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

// step 8
void streamTimerHandler(void* clientData) {
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias

  scs.streamTimerTask = NULL;
  LOGI("    streamTimerHandler   \n");
  // Shut down the stream:
  shutdownStream(rtspClient);
}

// shutdown
void shutdownStream(RTSPClient* rtspClient, int exitCode) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) { 
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) {
       if (subsession->sink != NULL) {
	     Medium::close(subsession->sink);
	     subsession->sink = NULL;

	     if (subsession->rtcpInstance() != NULL) {
	       subsession->rtcpInstance()->setByeHandler(NULL, NULL);
	       // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	     }
	     someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  LOGI("Closing the stream \n");
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  if (--rtspClientCount == 0) {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
    exit(exitCode);
  }
}

// Implementation of "ourRTSPClient":
ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
}

ourRTSPClient::~ourRTSPClient() {
}

// Implementation of "StreamClientState":
StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}

// Implementation of "DummySink":
// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
  return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env),
    fSubsession(subsession) {
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink::~DummySink() {
  delete[] fReceiveBuffer;
  delete[] fStreamId;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  DummySink* sink = (DummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

jclass clazz;
jobject _obj;
jmethodID methodid;

/*
 JNIEnv和jobject对象都不能跨线程使用
    对于jobject，解决办法是
    a、m_obj = m_env->NewGlobalRef(obj);//创建一个全局变量
    b、jobject obj = m_env->AllocObject(m_cls);//在每个线程中都生成一个对象
    对于JNIEnv，解决办法是在每个线程中都重新生成一个env
    JNIEnv *env;
    m_jvm->AttachCurrentThread((void **)&env, NULL);
 * */
jboolean doDecode(jbyteArray data, jint size) {
	if(methodid==0){
		    // 1.找到java的MainActivity的class
		    clazz = _env->FindClass("com/example/myrtsp0/MainActivity0");
			if (clazz == 0) {
				LOGI("can't find clazz");
			}
			LOGI(" find clazz");

			_obj = _env->AllocObject(clazz);

			//2 找到class 里面的方法定义
			//methodid = _env->GetMethodID(clazz, "onFrame","([BI)V");
			methodid = _env->GetMethodID(clazz, "onFrame","([BI)Z");
			if (methodid == 0) {
				LOGI("can't find methodid");
				return false;
			}
			LOGI(" find methodid");
	}
	//3 .调用方法
	//_env->CallVoidMethod(_obj, methodid, data, size);
	jboolean flag = _env->CallBooleanMethod(_obj, methodid, data, size);
	return flag;
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  // We've just received a frame of data.  (Optionally) print out information about it:
  /*
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL)
	  LOGI("Stream ,fStreamId: %s \n", fStreamId);
      LOGI("fSubsession.mediumName() = %s, %s, %d", fSubsession.mediumName(),fSubsession.codecName() ,frameSize);
  if (numTruncatedBytes > 0)
	  LOGI("with = %d  bytes truncated \n",numTruncatedBytes);

  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  LOGI("Presentation time: %d.%s ", (int)presentationTime.tv_sec,uSecsStr);

  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
	  LOGI("!!!!!!"); // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  LOGI("tNPT: %d", fSubsession.getNormalPlayTime(presentationTime));
#endif

#endif
  */
  int size = frameSize+4;
  unsigned char *buffer = (unsigned char *)malloc(size);
  buffer[0] = 0x0;
  buffer[1] = 0x0;
  buffer[2] = 0x0;
  buffer[3] = 0x1;
  memcpy(buffer+4, fReceiveBuffer, frameSize);

  //fReceiveBuffer
  jbyte *jy=(jbyte*)buffer;
  jbyteArray jbarray = _env->NewByteArray(size);//jbarray
  //jbyte *jy=(jbyte*)fReceiveBuffer;  //BYTE=>Jbyte
  _env->SetByteArrayRegion(jbarray, 0, size, jy);

  int len = _env->GetArrayLength(jbarray);

  //doDecode(jbarray, size);
  jboolean flag = false;
  if(methodid == 0){
  	 clazz = _env->FindClass("com/example/myrtsp0/MainActivity0");
  	 if (clazz == 0) {
  		LOGI("can't find clazz");
  	 }
   	 LOGI("find clazz");
  	 _obj = _env->AllocObject(clazz);

  	 //methodid = _env->GetMethodID(clazz, "onFrame","([BI)V");
  	 methodid = _env->GetMethodID(clazz, "onFrame","([BI)Z");
  	 if (methodid == 0) {
  		LOGI("can't find methodid");
  	 }
  	 LOGI(" find methodid");
  }

  if(methodid != 0){
      LOGI("frameSize = %d, len = %d, size = %d,\n", frameSize, len, size);
      flag = _env->CallBooleanMethod(_obj, methodid, jbarray, size);
  }

  //_env->ReleaseByteArrayElements(jbarray, jy, 0); //
  free(buffer);
  // Then continue, to request the next frame of data:

  //LOGI("      usleep(25000)    ");
  usleep(10000);
  continuePlaying();
}

Boolean DummySink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.
  // "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}


#ifdef __cplusplus
}
#endif
