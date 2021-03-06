

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include "video_rtp_send.h"
#include "omx_decode.h"
#include "omxcam/omxcam.h"
#include "rtpavcsend.h"



struct sockaddr_in s_peer;
static RtpSend* rtpsock;
static int udpsock;

static int sock_create(const char* peer_addr, int port)
{
	int s;

	memset(&s_peer, 0, sizeof(s_peer));
	s_peer.sin_family = AF_INET;
	s_peer.sin_addr.s_addr = inet_addr(peer_addr);
	s_peer.sin_port = htons(port);

	if (s_peer.sin_addr.s_addr == 0 || s_peer.sin_addr.s_addr == 0xffffffff) {
		fprintf(stderr, "Invalid address(%s)\n", peer_addr);
		return -1;
	}
	if (port <= 0 || port > 65535) {
		fprintf(stderr, "Invalid port(%d)\n", port);
		return -1;
	}

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) { perror("socket"); return -1; }

	return s;
}


void *video_send(void *videosendparam){

	video_send_param=videosendparam;
	omxcam_video_settings_t videoset = {};

	//摄像头/编码器初始化
	omxcam_video_init(&videoset);
	//videoset.on_data = video_encoded;
	//摄像头参数设置
	videoset.camera.width = 1920;
	videoset.camera.height = 1080;
	videoset.camera.framerate = 30;
	videoset.camera.rotation=180;
	//H264编码器设置
	videoset.h264.bitrate = 2*1000*1000; //12Mbps
	videoset.h264.idr_period = 10;	//30 IDR间隔
	videoset.h264.inline_headers = OMXCAM_TRUE; // SPS/PPS头插入
	videoset.h264.profile=OMXCAM_H264_AVC_PROFILE_BASELINE;

	printf("video_send_param:%s:%d\n",video_send_param->dest_ip, video_send_param->dest_port);
	//网络初始化
	//udpsock = sock_create(video_send_param->dest_ip, video_send_param->dest_port);
	udpsock = sock_create("192.168.1.114", 8888);
	if (udpsock < 0) return NULL;
	  
	// RTP初期化
	rtpopen(&rtpsock, 10110825/*SSRC*/, 96/*payload_type*/, udpsock, &s_peer);


	omxcam_video_start_npt(&videoset);
	
 	while (video_send_param->send_quit==1){
		omxcam_buffer_t buffer;
		if (omxcam_video_read_npt (&buffer, 0)) return NULL;
		AvcAnalyzeAndSend(rtpsock, buffer.data, buffer.length);
		//rtpSend(pRtpSession, (char *)buff.data, buff.length); //使用ortp库
		printf("encoded len=%d\n",buffer.length);
	}
	
	omxcam_video_stop_npt();
	rtpclose(rtpsock);
	//fprintf(stderr,BLUE"[%s@%s]:" NONE"video send thread exit\n", __func__, __FILE__);
	return NULL;
	}	

