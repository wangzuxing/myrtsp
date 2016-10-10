#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include "x264_config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <jni.h>
#include <android/log.h> 

#define LOG_TAG  "librtsp0"
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))

static AVFormatContext *i_fmt_ctx;
static AVStream *i_video_stream;

static AVFormatContext *o_fmt_ctx;
static AVStream *o_video_stream;

signed char bStop = 0;

signed char rtsp2mp4(const char *psdp, const char *pfile)
{
	LOGI("              Start              ");
	LOGI("              Start              ");
	avcodec_register_all();
    av_register_all();
    avformat_network_init();

    /* should set to NULL so that avformat_open_input() allocate a new one */
    i_fmt_ctx = NULL;

    //char rtspUrl[] = "rtsp://admin:12345@192.168.10.76:554";
    unsigned i;
    int ret;
    const char *rtspUrl = psdp;
    const char *filename = pfile; //"1.mp4";
    
    if (avformat_open_input(&i_fmt_ctx, rtspUrl, NULL, NULL)!=0)
    {
        fprintf(stderr, "could not open input file\n");
        LOGI("             could not open input file              ");
        return -1;
    }

    if (avformat_find_stream_info(i_fmt_ctx, NULL)<0)
    {
        fprintf(stderr, "could not find stream info\n");
        LOGI("             could not find stream info              ");
        return -1;
    }

    LOGI("              start 00               ");
    //av_dump_format(i_fmt_ctx, 0, argv[1], 0);

    /* find first video stream */
    for (i=0; i<i_fmt_ctx->nb_streams; i++)
    {
        if (i_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            i_video_stream = i_fmt_ctx->streams[i];
            break;
        }
    }
    if (i_video_stream == NULL)
    {
        fprintf(stderr, "didn't find any video stream\n");
        LOGI("             didn't find any video stream              ");
        return -1;
    }

    LOGI("              start 11               ");
    avformat_alloc_output_context2(&o_fmt_ctx, NULL, NULL, filename);

    /*
    * since all input files are supposed to be identical (framerate, dimension, color format, ...)
    * we can safely set output codec values from first input file
    */
    o_video_stream = avformat_new_stream(o_fmt_ctx, NULL);

    LOGI("              start 22               ");
    {
        AVCodecContext *c;
        c = o_video_stream->codec;
        c->bit_rate = 400000;
        c->codec_id = i_video_stream->codec->codec_id;
        c->codec_type = i_video_stream->codec->codec_type;
        c->time_base.num = i_video_stream->time_base.num;
        c->time_base.den = i_video_stream->time_base.den;
        fprintf(stderr, "time_base.num = %d time_base.den = %d\n", c->time_base.num, c->time_base.den);
        c->width = i_video_stream->codec->width;
        c->height = i_video_stream->codec->height;
        c->pix_fmt = i_video_stream->codec->pix_fmt;
        printf("%d %d %d", c->width, c->height, c->pix_fmt);
        c->flags = i_video_stream->codec->flags;
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
        c->me_range = i_video_stream->codec->me_range;
        c->max_qdiff = i_video_stream->codec->max_qdiff;

        c->qmin = i_video_stream->codec->qmin;
        c->qmax = i_video_stream->codec->qmax;

        c->qcompress = i_video_stream->codec->qcompress;

        LOGI("     c->codec_id = %d, num = %d, den = %d   ", c->codec_id, c->time_base.num, c->time_base.den);
    }

    LOGI("              start 33               ");
    avio_open(&o_fmt_ctx->pb, filename, AVIO_FLAG_WRITE);

    ret = avformat_write_header(o_fmt_ctx, NULL);

    LOGI("    c->width = %d, c->height = %d, c->pix_fmt= %d  ", i_video_stream->codec->width,
    		i_video_stream->codec->height,
			i_video_stream->codec->pix_fmt);

    LOGI("             rtsp to mp4 start              ");

    int last_pts = 0;
    int last_dts = 0;

    int64_t pts, dts;
    int num_rec = 0;

    while (bStop==0)
    {
        AVPacket i_pkt;
        av_init_packet(&i_pkt);
        i_pkt.size = 0;
        i_pkt.data = NULL;
        if (av_read_frame(i_fmt_ctx, &i_pkt) <0 )
            break;
        /*
        * pts and dts should increase monotonically
        * pts should be >= dts
        */
        i_pkt.flags |= AV_PKT_FLAG_KEY;
        pts = i_pkt.pts;
        i_pkt.pts += last_pts;
        dts = i_pkt.dts;
        i_pkt.dts += last_dts;
        i_pkt.stream_index = 0;

        //printf("%lld %lld\n", i_pkt.pts, i_pkt.dts);
        static int num = 1;
        //printf("frame %d\n", num++);
        num++;
        if(++num_rec>40){
           num_rec = 0;
           LOGI("             num = %d            ", num);
        }

        av_interleaved_write_frame(o_fmt_ctx, &i_pkt);
        //av_free_packet(&i_pkt);
        //av_init_packet(&i_pkt);
        usleep(25000);
    }
    last_dts += dts;
    last_pts += pts;

    avformat_close_input(&i_fmt_ctx);

    av_write_trailer(o_fmt_ctx);

    avcodec_close(o_fmt_ctx->streams[0]->codec);
    av_freep(&o_fmt_ctx->streams[0]->codec);
    av_freep(&o_fmt_ctx->streams[0]);

    avio_close(o_fmt_ctx->pb);
    av_free(o_fmt_ctx);

    LOGI("              End              ");
    LOGI("              End              ");

    return 0;
}

JNIEXPORT void JNICALL Java_com_example_myrtsp0_MainActivity_RtspMp4Start
 (JNIEnv *env, jclass clz, jstring sdp, jstring mp4)
 {
 	const char* sdp_title = (*env)->GetStringUTFChars(env, sdp, NULL);
 	const char* mp4_title = (*env)->GetStringUTFChars(env, mp4, NULL);

 	LOGI("              RtspMp4Start              ");
 	LOGI("              RtspMp4Start              ");
 	bStop = 0;
    signed char ret = rtsp2mp4(sdp_title, mp4_title);

    LOGI("              RtspMp4 = %d              ", ret);

 	(*env)->ReleaseStringUTFChars(env, sdp, sdp_title);
 	(*env)->ReleaseStringUTFChars(env, mp4, mp4_title);
 }

//视频录制结束调用
JNIEXPORT void Java_com_example_myrtsp0_MainActivity_RtspMp4End
(JNIEnv *env, jclass clz)
{
	bStop = 1;
	LOGI("              RtspMp4End              ");
	LOGI("              RtspMp4End              ");
}

