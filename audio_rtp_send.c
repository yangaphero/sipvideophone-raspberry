/*

This example reads from the default PCM device
and G711 code and rtp send.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h> 

#include <alsa/asoundlib.h>
#include <math.h>
#include "g711codec.h"
#include "audio_rtp_send.h"

#define BUFFERSIZE 4096
#define PERIOD_SIZE 1024
#define PERIODS 2
#define SAMPLE_RATE 8000
#define CHANNELS 1
#define FSIZE 2*CHANNELS
/*
 * Warning: rate is not accurate (requested = 16000Hz, got = 44100Hz)
 * 从字面意思来看意思是我使用的采样率不准确，希望得到16K采样，结果却的到了44.1K采样，其实问题不是我们的采样率不准确，而是我们的声卡是USB外置声卡，需要使用plughw:1,0这样的方法来标识。更改之后就正常了。
 * 把人害死了！！！！！！！！！
*/
#define ALSA_PCM_NEW_HW_PARAMS_API

void *audio_send(void *AudioSendParam) 
{
	struct audio_param_send *audiosendparam=AudioSendParam;
	//char *audio_hw, char *dest_ip, int dest_port
	
	fprintf(stderr,GREEN"[%s]:"NONE"param:audio_hw=%s,dest_ip=%s,dest_port=%d\n",__FILE__,audiosendparam->audio_hw,audiosendparam->dest_ip,audiosendparam->dest_port);

	int rc;	//return code.
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
 	int err;

	/* Open PCM device for recording (capture). */
	err = snd_pcm_open(&handle, audiosendparam->audio_hw , SND_PCM_STREAM_CAPTURE, 0);
	if (err < 0) {
		fprintf(stderr,RED "[%s@%s,%d]：" NONE "unable to open pcm device: %s\n",__func__, __FILE__, __LINE__,snd_strerror(err));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	err=snd_pcm_hw_params_any(handle, params);
		if (err < 0) {
		    fprintf(stderr, RED "[%s@%s,%d]：" NONE "Can not configure this PCM device: %s\n",__func__, __FILE__, __LINE__,snd_strerror(err));
		    exit(1);
		}
	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	err=snd_pcm_hw_params_set_access(handle, params,SND_PCM_ACCESS_RW_INTERLEAVED);
		if (err < 0) {
		    fprintf(stderr,  RED   "[%s@%s,%d]：" NONE "Failed to set PCM device to interleaved: %s\n", __func__, __FILE__, __LINE__,snd_strerror(err));
		    exit(1);
		}
	/* Signed 16-bit little-endian format */
	//err=snd_pcm_hw_params_set_format(handle, params,SND_PCM_FORMAT_MU_LAW);
	err=snd_pcm_hw_params_set_format(handle, params,SND_PCM_FORMAT_S16_LE);
		if (err < 0) {
		    fprintf(stderr,RED  "[%s@%s,%d]：" NONE "Failed to set PCM device to 16-bit signed PCM: %s\n", __func__, __FILE__, __LINE__,snd_strerror(err));
		    exit(1);
		}
	/* One channels (mono) */
	err=snd_pcm_hw_params_set_channels(handle, params, CHANNELS);
		if (err < 0) {
		    fprintf(stderr,RED "[%s@%s,%d]：" NONE "Failed to set PCM device to mono: %s\n",__func__, __FILE__, __LINE__,snd_strerror(err));
		    exit(1);
		}
	/* 8000 bits/second sampling rate (CD quality) */
	val = 8000;//这里修改为8000
	//val = 44100; 
	err=snd_pcm_hw_params_set_rate_near(handle, params,&val, &dir);
		if (err < 0) {
		    fprintf(stderr, RED "[%s@%s,%d]：" NONE "Failed to set PCM device to sample rate =%d: %s\n",__func__, __FILE__, __LINE__,val,snd_strerror(err));
		    exit(1);
		}
	/* Set period size to 32 frames. */
	


	// Set buffer time 500000.
	
	unsigned int buffer_time,period_time;
	snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
	if ( buffer_time >500000) 		
	buffer_time = 80000;//这里可修改
	period_time = buffer_time/4;//这里可修改  size = frames * FSIZE;
	err = snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, 0);
		if (err < 0) {
		    fprintf(stderr, RED "[%s@%s,%d]：" NONE "Failed to set PCM device to buffer time =%d: %s\n",  __func__, __FILE__, __LINE__,buffer_time,snd_strerror(err));
		    exit(1);
		}

	err = snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, 0);
		if (err < 0) {
		    fprintf(stderr, RED  "[%s@%s,%d]：" NONE "Failed to set PCM device to period time =%d: %s\n",__func__, __FILE__, __LINE__,period_time,snd_strerror(err));
		    exit(1);
		}



	err = snd_pcm_hw_params(handle, params);
		if (err < 0) {
			fprintf(stderr,RED  "[%s@%s,%d]：" NONE "unable to set hw parameters: %s\n",__func__, __FILE__, __LINE__,snd_strerror(err));
			exit(1);
		}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params,&frames, &dir);
	size = frames * FSIZE; /* 2 bytes/sample, 1 channels *///这里应该=320  FSIZE=2    frames=160
	buffer = (char *) malloc(size);
	
	fprintf(stderr,GREEN "[%s@%s]：" NONE "read buffer size = %d\n",__func__, __FILE__,size);
	fprintf(stderr,GREEN "[%s@%s]：" NONE "period size = %d frames\n",__func__, __FILE__,(int)frames);


	/*print alsa config parameter*/
	snd_pcm_hw_params_get_period_time(params,&val, &dir);
	fprintf(stderr,GREEN "[%s@%s]：" NONE "period time is: %d\n",__func__, __FILE__,val);
	snd_pcm_hw_params_get_buffer_time(params, &val, &dir);
	fprintf(stderr,GREEN "[%s@%s]：" NONE "buffer time = %d us\n",__func__, __FILE__,val);
	snd_pcm_hw_params_get_buffer_size(params, (snd_pcm_uframes_t *) &val);
	fprintf(stderr,GREEN "[%s@%s]：" NONE "buffer size = %d frames\n",__func__, __FILE__,val);
	snd_pcm_hw_params_get_periods(params, &val, &dir);
	fprintf(stderr,GREEN "[%s@%s]：" NONE "periods per buffer = %d frames\n",__func__, __FILE__,val);

    int M_bit=1;

    char sendbuf[1500];
    memset(sendbuf,0,1500);
    unsigned short seq_num = 0;
    RTP_FIXED_HEADER        *rtp_hdr;

	unsigned int timestamp_increse = 0,ts_current = 0;
	timestamp_increse = 160;
	while (audiosendparam->send_quit==1) {
		//1.采集
		rc = snd_pcm_readi(handle, buffer, frames);//采集音频数据
		if (rc == -EPIPE) {
			fprintf(stderr, RED "[%s@%s,%d]：overrun occurred\n",__func__, __FILE__, __LINE__);
			err=snd_pcm_prepare(handle);
			if( err <0){
				fprintf(stderr, RED  "[%s@%s,%d]：Failed to recover form overrun : %s\n",__func__, __FILE__, __LINE__,
				snd_strerror(err));
				exit(1);
			}
		}
		else if (rc < 0) {
			fprintf(stderr,RED "[%s@%s,%d]：" NONE "error from read: %s\n",__func__, __FILE__, __LINE__,snd_strerror(rc));
			exit(1);
		} 
		else if (rc != (int)frames) {
			fprintf(stderr, RED  "[%s@%s,%d]：" NONE "short read, read %d frames\n", __func__, __FILE__, __LINE__,rc);
		}
		
		//2.编码
		rc = PCM2G711u( (char *)buffer, (char *)&sendbuf[12], size, 0 );//pcm转g711a
		if(rc<0)  fprintf(stderr,RED "[%s@%s,%d]：" NONE "PCM2G711u error:rc=%d\n",__func__, __FILE__, __LINE__,rc);
		

		//3.打包
		rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0];
		rtp_hdr->payload     = 0;  //负载类型号，
		rtp_hdr->version     = 2;  //版本号，此版本固定为2
		if(1 == M_bit)	{
			rtp_hdr->marker    = 1;   //标志位，由具体协议规定其值。
			M_bit = 0;
		}
		else{
			rtp_hdr->marker    = 0;   //标志位，由具体协议规定其值。
		}
		rtp_hdr->ssrc        = htonl(10);    //随机指定为10，并且在本RTP会话中全局唯一
		rtp_hdr->seq_no = htons(seq_num ++);//rtp包序号
		ts_current = ts_current+timestamp_increse;
		rtp_hdr->timestamp=htonl(ts_current);//rtp传输时间戳，增量为timestamp_increse=160
		
		//4.发送
		rc = send( audiosendparam->audio_rtp_socket, sendbuf, rc+12, 0 );//开始发送rtp包，+12是rtp的包头+g711荷载
		
		if(rc<0) {
			//对方呼叫结束产生错误
			//fprintf(stderr , RED "[%s@%s,%d]:" NONE "net send error=%d\n", __func__, __FILE__, __LINE__,rc);
			break;
		}
        memset(sendbuf,0,1500);//清空sendbuf；此时会将上次的时间戳清空，因此需要ts_current来保存上次的时间戳值

	}
	//shutdown(rtp_socket,2);
	close(audiosendparam->audio_rtp_socket);
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
	return NULL;
}
