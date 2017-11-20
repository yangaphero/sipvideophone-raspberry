#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "audio_rtp_recv.h"



	long loops;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
  
int audio_recv_init(){


	/* Open PCM device for playback. */
	rc = snd_pcm_open(&handle, "default",SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		fprintf(stderr,"unable to open pcm device: %s\n",snd_strerror(rc));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params,SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params,SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, 2);

	/* 44100 bits/second sampling rate (CD quality) */
	//val = 44100;
	val = 8000;
	snd_pcm_hw_params_set_rate_near(handle, params,&val, &dir);

	/* Set period size to 32 frames. */
	frames = 16;
	snd_pcm_hw_params_set_period_size_near(handle,params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr,"unable to set hw parameters: %s\n",snd_strerror(rc));
		exit(1);
	}

	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames,&dir);
	size = frames * 4; /* 2 bytes/sample, 2 channels */
	buffer = (char *) malloc(size);

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params,&val, &dir);	

	return 0;
}

int audio_recv_close(){
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
}


void *audio_recv(void *AudioParam){
	
	struct audio_param_recv *audiorecvparam=AudioParam;
	unsigned char buffer[2048];
	unsigned char outbuffer[2048];
	



audio_recv_init();

//FILE *fp = fopen("sh.pcm", "rb");

  while (audiorecvparam->recv_quit==1) {
	  	bzero(buffer, sizeof(buffer));
		//recv_len = audio_net_recv(audionethandle, buffer, sizeof(buffer));
		//printf("audio recv_len=%d\n",recv_len);
	  
	/*
    loops--;
    rc = read(fp, buffer, size);
    if (rc == 0) {
      fprintf(stderr, "end of file on input\n");
      break;
    } else if (rc != size) {
      fprintf(stderr,"short read: read %d bytes\n", rc);
    }
    rc = snd_pcm_writei(handle, buffer, frames);
    if (rc == -EPIPE) {
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,"error from writei: %s\n",snd_strerror(rc));
    }  
   
    else if (rc != (int)frames) {
      fprintf(stderr,
              "short write, write %d frames\n", rc);
    }
    */
  }

audio_recv_close();
//audio_net_close(audionethandle);
  return 0;
}
