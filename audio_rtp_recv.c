#define ALSA_PCM_NEW_HW_PARAMS_API
#include <fcntl.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "audio_rtp_recv.h"
#include "g711codec.h"

snd_pcm_t *handle;

int audio_recv_init(){
	int err;
    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("Playback open error: %s/n", snd_strerror(err));
        exit(1);
    }
    if ((err = snd_pcm_set_params(handle,
                    SND_PCM_FORMAT_S16_LE,//SND_PCM_FORMAT_U8,
                    SND_PCM_ACCESS_RW_INTERLEAVED,/* snd_pcm_readi/snd_pcm_writei access */
                    1, //Channels
                    8000, //sample rate in Hz
                    1, //soft_resample
                    500000)) < 0) {////如果latency过小，会使得snd_pcm_writei()丢发声数据，产生short write现象 1000000=1sec
        printf("Playback open error: %s/n", snd_strerror(err));
        exit(1);
    }
	return 0;
}

int audio_recv_close(){
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	return 0;
}


void *audio_recv(void *AudioParam){
	int rc;
	struct audio_param_recv *audiorecvparam=AudioParam;
	char recvbuffer[256];
    char outbuffer[320];
	int recv_len;
	int frames=160;//注意定义alsa中frames
	
	audio_recv_init();
  while (audiorecvparam->recv_quit==1) {
	  	bzero(recvbuffer, sizeof(recvbuffer));
	  	usleep(100); //防止cpu过高
        recv_len = recv(audiorecvparam->audio_rtp_socket, recvbuffer, sizeof(recvbuffer), 0 );
        if(recv_len<0) 	continue;
        //printf("audio recv_len=%d,seq=%d\n",recv_len,recvbuffer[2] << 8|recvbuffer[3] << 0);
        rc=G711u2PCM(&recvbuffer[12], outbuffer, 160, 0);//应该返回值为320
		if(rc<0)  fprintf(stderr,RED "[%s]：" NONE "G711u2PCM error:rc=%d\n",__FILE__,rc);
		
		//送到pcm解码播放
		rc = snd_pcm_writei(handle, outbuffer, frames);
		if (rc == -EPIPE) {
		fprintf(stderr, RED "[%s]：" NONE "underrun occurred\n",__FILE__);
		snd_pcm_prepare(handle);
		} else if (rc < 0) {
		fprintf(stderr,RED "[%s]：" NONE "error from writei: %s\n",__FILE__,snd_strerror(rc));
		} else if (rc != (int)frames) {
		fprintf(stderr,RED "[%s]：" NONE "short write, write %d frames,not %d\n",__FILE__, rc,frames);
		}
  }
	audio_recv_close();
	close(audiorecvparam->audio_rtp_socket);
	return 0;
}

/*
int fd = open ("recv.pcm", O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);//保存解码接收后的声音文件
if (pwrite (fd, outbuffer, rc, 0) == -1) fprintf (stderr, "error: pwrite\n");//写入到文件用于测试
close (fd);//关闭句柄
*/



