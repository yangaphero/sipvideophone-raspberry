//   Copyright 2015-2016 Ansersion
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "bcm_host.h"
#include "ilclient.h"
#include "common.h"


COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL, *video_clock = NULL;
COMPONENT_T *list[5];
TUNNEL_T tunnel[4];
ILCLIENT_T *client;
int status = 0;
unsigned int data_len = 0;
char *base_sps, *base_pps;
int debug = 1;

//设置播放速度
int SetSpeed(int speed){
		//speed=1000;正常速度
		OMX_TIME_CONFIG_SCALETYPE scaleType;
		OMX_INIT_STRUCTURE(scaleType);
		scaleType.xScale = (speed << 16) / 1000;//speed=0为暂停
		if(OMX_SetConfig(ILC_GET_HANDLE(video_clock),OMX_IndexConfigTimeScale, &scaleType)!= OMX_ErrorNone)
			printf("[clock]OMX_IndexConfigTimeScale error\n");
		return 0;
}
//设置视频透明度
int SetAlpha(int alpha){
		//if (debug) printf("set alpha:[%d]\n",alpha);
		OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
		OMX_INIT_STRUCTURE(configDisplay);
		configDisplay.nPortIndex =90;
		configDisplay.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_ALPHA );
		configDisplay.alpha = alpha;
		if(OMX_SetConfig(ILC_GET_HANDLE(video_render),OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
			printf("[video_render]OMX_IndexConfigDisplayRegion error\n");
		return 0;
}

//设置窗口位置
int SetRect(int x1,int y1,int x2,int y2){
	//if (debug) printf("set rect:[%d,%d,%d,%d]\n",x1,y1,x2,y2);
	OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
	OMX_INIT_STRUCTURE(configDisplay);
	configDisplay.nPortIndex =90;
	configDisplay.fullscreen = OMX_FALSE;
	configDisplay.noaspect   = OMX_TRUE;
	configDisplay.set                 = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT|OMX_DISPLAY_SET_SRC_RECT|OMX_DISPLAY_SET_FULLSCREEN|OMX_DISPLAY_SET_NOASPECT);
	configDisplay.dest_rect.x_offset  = x1;
	configDisplay.dest_rect.y_offset  = y1;
	configDisplay.dest_rect.width     = x2;
	configDisplay.dest_rect.height    = y2;
	/*
	//其他设置
	configDisplay.set = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_ALPHA | OMX_DISPLAY_SET_TRANSFORM | OMX_DISPLAY_SET_LAYER | OMX_DISPLAY_SET_NUM);
	configDisplay.alpha = 200;
	configDisplay.num = 0;
	configDisplay.layer = 1;
	configDisplay.transform =0;//0正常 1镜像 2旋转180
	
	configDisplay.fullscreen = OMX_FALSE;
	configDisplay.noaspect   = OMX_TRUE;
    
	configDisplay.set                 = (OMX_DISPLAYSETTYPE)(OMX_DISPLAY_SET_DEST_RECT|OMX_DISPLAY_SET_SRC_RECT|OMX_DISPLAY_SET_FULLSCREEN|OMX_DISPLAY_SET_NOASPECT);
	configDisplay.dest_rect.x_offset  =200;
	configDisplay.dest_rect.y_offset  = 0;
	configDisplay.dest_rect.width     = 640;
	configDisplay.dest_rect.height    = 480;

	configDisplay.src_rect.x_offset   =1920;
	configDisplay.src_rect.y_offset   =0;
	configDisplay.src_rect.width      = 1920;
	configDisplay.src_rect.height     =1080;
	*/
	if(OMX_SetConfig(ILC_GET_HANDLE(video_render),OMX_IndexConfigDisplayRegion, &configDisplay) != OMX_ErrorNone)
		printf("[video_render]OMX_IndexConfigDisplayRegion error\n");
	return 0;
}
int omx_init(){
   bcm_host_init();
   memset(list, 0, sizeof(list));
   memset(tunnel, 0, sizeof(tunnel));

   if((client = ilclient_init()) == NULL){
	  status = -21;
	  printf("ilclient_init error\n");
      return status;
   }
   if(OMX_Init() != OMX_ErrorNone){
	  status = -21;
	  printf("OMX_Init error\n");
      ilclient_destroy(client);
      return status;
   }

   // create video_decode
   if(ilclient_create_component(client, &video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0)
      status = -14;
   list[0] = video_decode;

   // create video_render
   if(status == 0 && ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[1] = video_render;

   // create clock
   if(status == 0 && ilclient_create_component(client, &video_clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[2] = video_clock;
   
   OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
   memset(&cstate, 0, sizeof(cstate));
   cstate.nSize = sizeof(cstate);
   cstate.nVersion.nVersion = OMX_VERSION;
   cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
   cstate.nWaitMask = 1;
   if(video_clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(video_clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
      status = -13;

   // create video_scheduler
   if(status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[3] = video_scheduler;

   set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
   set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90);
   set_tunnel(tunnel+2, video_clock, 80, video_scheduler, 12);

   // setup clock tunnel first
   if(status == 0 && ilclient_setup_tunnel(tunnel+2, 0, 0) != 0)
      status = -15;
   else
      ilclient_change_component_state(video_clock, OMX_StateExecuting);

   if(status == 0)
      ilclient_change_component_state(video_decode, OMX_StateIdle);
  
   OMX_VIDEO_PARAM_PORTFORMATTYPE format;
   memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
   format.nVersion.nVersion = OMX_VERSION;
   format.nPortIndex = 130;
   format.eCompressionFormat = OMX_VIDEO_CodingAVC;
   format.xFramerate = 30 * (1<<16);
   
   if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) != OMX_ErrorNone ){
	   status = -21;
   }
  //----------------------------------------------------------------
  
	OMX_PARAM_PORTDEFINITIONTYPE portParam;
	OMX_INIT_STRUCTURE(portParam);
	portParam.nPortIndex = 130;
	if(OMX_GetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamPortDefinition, &portParam)!= OMX_ErrorNone)
		printf("OMX_GetParameter OMX_IndexParamPortDefinition error!\n");

	portParam.nBufferSize=100*1024;//默认81920,不要轻易改动
	//portParam.nBufferCountMin=2;
	//portParam.nBufferCountActual=100;
	//portParam.format.video.nFrameWidth  = 1920;
	//portParam.format.video.nFrameHeight = 1080;
	if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamPortDefinition, &portParam)!= OMX_ErrorNone)
		printf("OMX_SetParameter OMX_IndexParamPortDefinition error\n!");
	if(OMX_GetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamPortDefinition, &portParam)!= OMX_ErrorNone)
		printf("OMX_GetParameter OMX_IndexParamPortDefinition error\n!");
	printf("portParam.nBufferSize=%d\n",portParam.nBufferSize);
   
  //----------------------------------------------------------------
 
    //有效帧开始
    
	OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
	OMX_INIT_STRUCTURE(concanParam);
	concanParam.bStartWithValidFrame = OMX_FALSE;//OMX_FALSE OMX_TRUE
	if(OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam)!= OMX_ErrorNone)
		printf("OMX_SetParameter OMX_IndexParamBrcmVideoDecodeErrorConcealment error!\n");

	//request portsettingschanged on aspect ratio change
	OMX_CONFIG_REQUESTCALLBACKTYPE notifications;
	OMX_INIT_STRUCTURE(notifications);
	notifications.nPortIndex = 131;//OutputPort
	notifications.nIndex = OMX_IndexParamBrcmPixelAspectRatio;
	notifications.bEnable = OMX_TRUE;
	if (OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexConfigRequestCallback, &notifications) != OMX_ErrorNone)
		printf("[video_decode]OMX_SetParameter OMX_IndexConfigRequestCallback error!\n");
	
   //----------------------------------------------------------
	
	OMX_CONFIG_BOOLEANTYPE timeStampMode;
	OMX_INIT_STRUCTURE(timeStampMode);
	timeStampMode.bEnabled = OMX_TRUE;
	if (OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamBrcmVideoTimestampFifo, &timeStampMode) != OMX_ErrorNone)
		printf("[video_decode]OMX_SetParameter OMX_IndexParamBrcmVideoTimestampFifo error\n!");
   
   //----------------------------------------------------------
   
   if(ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) != 0){
	   status = -22;
   }
   if(status==0){
	   ilclient_change_component_state(video_decode, OMX_StateExecuting);
   }
   printf("omx init succefull\n");
   return status;
}



float pts,recpts;
unsigned long start_timestamp;
int first_packet = 1;
int omx_decode(unsigned char *videobuffer,int videobuffer_len,unsigned long timestamp){
//printf("[omx_decode]videobuffer_len=%d\n",videobuffer_len);
usleep(0);//防止cpu占用100%，不知道原因
   if(status == 0){
	    
	    //pts获取
		OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
		OMX_INIT_STRUCTURE(timeStamp);
		timeStamp.nPortIndex =80;//OMX_IndexConfigTimeCurrentMediaTime
		if(OMX_GetConfig(ILC_GET_HANDLE(video_clock),OMX_IndexConfigTimeCurrentMediaTime, &timeStamp)== OMX_ErrorNone){
			pts = (double)FromOMXTime(timeStamp.nTimestamp);
			//if (debug)printf("pts:%.2f [%.0f]--recpts:%.2f [%.0f]\n",  (double)pts* 1e-6, (double)pts,(double)recpts* 1e-6, (double)recpts);
		}
	   
	   
      OMX_BUFFERHEADERTYPE *buf;
      int port_settings_changed = 0;//每次为０可以在切换码流时候再次port_settings_changed
      

	if((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL){
		if(buf->nAllocLen<videobuffer_len)	printf("buf-nAllocLen=%d,videobuffer_len=%d\n",buf->nAllocLen,videobuffer_len);
		memcpy(buf->pBuffer,videobuffer,videobuffer_len);
		//free(videobuffer);//对应的是video_rtp_recv.c中的in_buffer=(unsigned char *)malloc(outbuffer_len);
		buf->nFilledLen = videobuffer_len;
        buf->nOffset = 0;
		recpts=((double)timestamp-(double)start_timestamp)/90000;//注意计算方式不一样
		float video_fifo=recpts-(double)pts* 1e-6;
		//printf("[pts]:encode[%.2f]-decode[%.2f]=[%.2f]\n",  recpts,(double)pts* 1e-6, video_fifo);
		//调节速度
		/*
		if(video_fifo<0.19){
			SetSpeed(995);
		}else if(video_fifo>0.21){
			SetSpeed(1010);
		}
		*/
		//-------------------------------------------------------------------------------
		if(port_settings_changed == 0 &&(videobuffer_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0)){
			printf("-------------***-port_settings_changed--***---------------\n");
			first_packet = 1;//***在视频切换或改变的时候，解码器的时间戳继续按原来的走，还不知道怎么把它初始化为０（pts的值）
			port_settings_changed = 1;
			if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)         status = -7;
			ilclient_change_component_state(video_scheduler, OMX_StateExecuting);
			if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0)       status = -12;
			ilclient_change_component_state(video_render, OMX_StateExecuting);
		}
			
         
         if(first_packet){
			/*
			//设置开始时间戳,从rtp包中获取到时间戳,但是不正确,采取时间戳相减
			OMX_TIME_CONFIG_TIMESTAMPTYPE sClientTimeStamp;
			OMX_INIT_STRUCTURE(sClientTimeStamp);
			if(OMX_GetConfig(ILC_GET_HANDLE(video_clock), OMX_IndexConfigTimeCurrentWallTime, &sClientTimeStamp)!=OMX_ErrorNone)
			printf("OMX_GetConfig OMX_IndexConfigTimeClientStartTime error\n!");
			sClientTimeStamp.nPortIndex=80;
			sClientTimeStamp.nTimestamp=ToOMXTime(timestamp);//设置开始时间戳
			if( OMX_SetConfig(ILC_GET_HANDLE(video_clock), OMX_IndexConfigTimeClientStartTime, &sClientTimeStamp)!=OMX_ErrorNone)
			printf("OMX_SetConfig OMX_IndexConfigTimeClientStartTime error\n!");
			*/
			start_timestamp=timestamp;
			//if (debug)printf("start_timestamp=%d\n",timestamp);
            buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
            //buf->nTimeStamp=pts;
            first_packet = 0;
         }
         else{
            buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
		    //buf->nTimeStamp.nLowPart=0;
		    //buf->nTimeStamp.nHighPart=0;
		 }
         if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)      status = -6;
      
      //ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0,ILCLIENT_BUFFER_FLAG_EOS, 10000);
      //ilclient_flush_tunnels(tunnel, 0);
	}
    }

	return status;
}

int omx_deinit(){
	pts=0;
	recpts=0;
	start_timestamp=0;
	first_packet = 1;//一定要注意，否则第二次呼入不解码，不显示图像
	ilclient_disable_tunnel(tunnel);
	ilclient_disable_tunnel(tunnel+1);
	ilclient_disable_tunnel(tunnel+2);
	ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
	ilclient_teardown_tunnels(tunnel);
	ilclient_state_transition(list, OMX_StateIdle);
	ilclient_state_transition(list, OMX_StateLoaded);
	ilclient_cleanup_components(list);
	OMX_Deinit();
	ilclient_destroy(client);
	return 0;
}
