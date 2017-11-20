

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
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "video_rtp_send.h"
#include "omx_decode.h"
#include "omxcam/omxcam.h"
#include "rtpavcsend.h"



struct sockaddr_in s_peer;
static RtpSend* rtpsock;
static int udpsock;

static int sock_create(const char* peer_addr, int port,int localport)
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
	s = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);
	if (s < 0) { perror("socket"); return -1; }
	
	//端口复用
	int flag=1;
	if( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1)  {  
	fprintf(stderr, RED "[%s@%s,%d]："NONE"socket setsockopt error\n", __func__, __FILE__, __LINE__);
	return -1;
	}  
	//绑定本地端口
	struct sockaddr_in local;  
	local.sin_family=AF_INET;  
	local.sin_port=htons(localport);            ///监听端口  
	local.sin_addr.s_addr=INADDR_ANY;       ///本机  
	if(bind(s,(struct sockaddr*)&local,sizeof(local))==-1) {
	fprintf(stderr, RED "[%s@%s,%d]："NONE"udp port bind error\n",__func__, __FILE__, __LINE__);
	return -1;
	}
	return s;
}



void *video_send(void *videosendparam){

	video_param_send *video_send_param=videosendparam;
	//使用video_send_param->video_rtp_socket这个socket不发送数据，暂时还是再创建socket
	omxcam_video_settings_t videoset = {};

	//摄像头/编码器初始化
	omxcam_video_init(&videoset);
	//摄像头参数设置
	videoset.camera.width = video_send_param->width;
	videoset.camera.height = video_send_param->height;
	videoset.camera.framerate = video_send_param->fps;
	videoset.camera.rotation=180;
	//H264编码器设置
	videoset.h264.bitrate = (video_send_param->bitrate)*1000; //12Mbps
	videoset.h264.idr_period = 10;	//30 IDR间隔
	videoset.h264.inline_headers = OMXCAM_TRUE; // SPS/PPS头插入
	videoset.h264.profile=OMXCAM_H264_AVC_PROFILE_HIGH;//OMXCAM_H264_AVC_PROFILE_BASELINE;

	fprintf(stderr,BLUE"[%s]:" NONE"video_send_param:%s:%d\n", __FILE__,video_send_param->dest_ip, video_send_param->dest_port);
	fprintf(stderr,BLUE"[%s]:" NONE"camera_param:%dX%d,fps:%d,%d\n", __FILE__,video_send_param->width, video_send_param->height,video_send_param->fps,video_send_param->bitrate);
	//网络初始化
	udpsock = sock_create(video_send_param->dest_ip, video_send_param->dest_port,video_send_param->local_port);
	//udpsock = sock_create("192.168.1.114", 8888);//用于调试，可将视频发送到第三个设备上，比如电脑上vlc接收
	// RTP初始化
	rtpopen(&rtpsock, 10110825/*SSRC*/, 96/*payload_type*/, udpsock/*video_send_param->video_rtp_socket*/, &s_peer);

	// 开始发送数据－－－videoset.on_data = video_encoded;
	omxcam_video_start_npt(&videoset);
	omxcam_buffer_t buffer;
	while (video_send_param->send_quit==1){
		if (omxcam_video_read_npt (&buffer, 0)) return NULL;
		AvcAnalyzeAndSend(rtpsock, buffer.data, buffer.length);
		//printf("encoded len=%d\n",buffer.length);
	}
	
	omxcam_video_stop_npt();

	rtpclose(rtpsock);
	//fprintf(stderr,BLUE"[%s]:" NONE"video send thread exit\n", __func__, __FILE__);
	return NULL;
	}	


