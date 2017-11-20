

#include <stdio.h>  
#include <stdlib.h>  
#include <stdint.h>
#include <netinet/in.h>  
#include <sys/socket.h>  
#include <arpa/inet.h>
#include <sys/types.h>  
#include <pthread.h>
#include <osip2/osip_mt.h> 
#include <eXosip2/eXosip.h>  
#include "audio_rtp_send.h"
#include "audio_rtp_recv.h"
#include "video_rtp_send.h"
#include "video_rtp_recv.h"




eXosip_event_t *je;  
osip_message_t *reg = NULL;  
osip_message_t *invite = NULL;  
osip_message_t *ack = NULL;  
osip_message_t *info = NULL;  
osip_message_t *message = NULL;  
osip_message_t *answer = NULL;
sdp_message_t *remote_sdp = NULL;
sdp_connection_t * con_req = NULL;
sdp_media_t * md_audio_req = NULL; 
sdp_media_t * md_video_req = NULL; 

struct osip_thread *event_thread;
struct osip_thread *audio_thread_send;
struct osip_thread *audio_thread_recv;
struct osip_thread *video_thread_send;
struct osip_thread *video_thread_recv;

int call_id, dialog_id,calling ;  
int i,flag;  
int quit_flag = 1;  
int id;  
char command;  
char tmp[4096];  
char localip[128];  

//下面是需要预定义的参数
char *identity = "sip:1009@192.168.1.114";  
char *registerer = "sip:192.168.1.114";  
char *source_call = "sip:1009@192.168.1.114";  
char *proxy="sip:192.168.1.114"; 
char *dest_call ="sip:192.168.1.133:5062";// "sip:1001@192.168.1.114";
char *fromuser="sip:1009@192.168.1.114";  //"sip:1008@192.168.1.114";  
char *userid="1009";
char *passwd="12345";
struct audio_param_send audiosendparam={0/*audio_rtp_socket*/,"plughw:1,0"/*"hw:1,0"*/,NULL,0,54000,1,1};//音频线程初始化默认值，树莓派下usb声卡hw:1,0
struct audio_param_recv audiorecvparam={0/*audio_rtp_socket*/,"hw:1,0",NULL,0,54000,1};//音频线程初始化默认值，树莓派下usb声卡hw:1,0
struct video_param_send videosendparam={0/*video_rtp_socket*/,"/dev/video0",NULL,0,54002,1280,720,30,4000,1,1,1,1};//视频线程初始化默认值
struct video_param_recv videorecvparam={0/*video_rtp_socket*/,"/dev/video0",NULL,0,54002,640,480,15,8000,1,1,1,1};//视频线程初始化默认值

int open_audio_socket(){
	//音频socket
	int    audio_rtp_socket;
	struct sockaddr_in server;
	int len = sizeof(server);
	server.sin_family = AF_INET;
	server.sin_port = htons(audiosendparam.dest_port);
	server.sin_addr.s_addr = inet_addr(audiosendparam.dest_ip);
	audio_rtp_socket = socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK,0);
	//设置超时
	struct timeval timeout={1,0};//1s
	//int ret=setsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
	if(setsockopt(audio_rtp_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout))<0){
	printf("setsockopt timeout fail");
	return -1;
	}
	//端口复用
	int flag=1;
	if( setsockopt(audio_rtp_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1)  {  
	fprintf(stderr, RED "[%s@%s,%d]："NONE"socket setsockopt error\n", __func__, __FILE__, __LINE__);
	return -1;
	}  
	//绑定本地端口
	struct sockaddr_in local;  
	local.sin_family=AF_INET;  
	local.sin_port=htons(audiosendparam.local_port);            ///监听端口  
	local.sin_addr.s_addr=INADDR_ANY;       ///本机  
	if(bind(audio_rtp_socket,(struct sockaddr*)&local,sizeof(local))==-1) {
	fprintf(stderr, RED "[%s@%s,%d]："NONE"udp port bind error\n",__func__, __FILE__, __LINE__);
	return -1;
	}
	connect(audio_rtp_socket, (struct sockaddr *)&server, len);
	audiosendparam.audio_rtp_socket=audiorecvparam.audio_rtp_socket=audio_rtp_socket;
	fprintf(stderr,GREEN"[%s]:"NONE"open_audio_socket!\n",__FILE__);
	return 0;
}

int open_video_socket(){
	//视频socket
	int    video_rtp_socket;
	struct sockaddr_in server;
	int len = sizeof(server);
	server.sin_family = AF_INET;
	server.sin_port = htons(videosendparam.dest_port);
	server.sin_addr.s_addr = inet_addr(videosendparam.dest_ip);
	video_rtp_socket = socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK,0);
	//设置超时
	struct timeval timeout={1,0};//1s
	//int ret=setsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
	if(setsockopt(video_rtp_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout))<0){
	printf("setsockopt timeout fail");
	return -1;
	}
	
	//端口复用
	int flag=1;
	if( setsockopt(video_rtp_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1)  {  
	fprintf(stderr, RED "[%s@%s,%d]："NONE"socket setsockopt error\n", __func__, __FILE__, __LINE__);
	return -1;
	}  
	
	//设置接收缓存
	int recv_size=20*1024*1024;
	if(setsockopt(video_rtp_socket,SOL_SOCKET,SO_RCVBUF,(char*)&recv_size,sizeof(recv_size))<0){
	printf("setsockopt recv_buff fail");
	return -1;
	}
	//设置发送缓存
	int send_size=20*1024*1024;
	if(setsockopt(video_rtp_socket,SOL_SOCKET,SO_SNDBUF,(char*)&send_size,sizeof(send_size))<0){
	printf("setsockopt send_buff fail");
	return -1;
	}

	//绑定本地端口
	struct sockaddr_in local;  
	local.sin_family=AF_INET;  
	local.sin_port=htons(videosendparam.local_port);            ///监听端口  
	local.sin_addr.s_addr=INADDR_ANY;       ///本机  
	if(bind(video_rtp_socket,(struct sockaddr*)&local,sizeof(local))==-1) {
	fprintf(stderr, RED "[%s@%s,%d]："NONE"udp port bind error\n",__func__, __FILE__, __LINE__);
	return -1;
	}
	connect(video_rtp_socket, (struct sockaddr *)&server, len);
	videosendparam.video_rtp_socket=videorecvparam.video_rtp_socket=video_rtp_socket;
	fprintf(stderr,GREEN"[%s]:"NONE"open_video_socket!\n",__FILE__);
	return 0;
}

 void  *sipEventThread() 
{  
  eXosip_event_t *je;  

  for (;;)  
  {  
      je = eXosip_event_wait (0, 100);  
      eXosip_lock();  
      eXosip_automatic_action ();  //401,407错误
      eXosip_unlock();  
  
      if (je == NULL)  
        continue;  
  
      switch (je->type)  
     {  
		 /* REGISTER related events 1-4*/
		case EXOSIP_REGISTRATION_NEW:  
			printf("received new registration\n");
			break;  

		case EXOSIP_REGISTRATION_SUCCESS:   
			printf( "registrered successfully\n");
			break;  

		case EXOSIP_REGISTRATION_FAILURE:
			printf("EXOSIP_REGISTRATION_FAILURE!\n");
			break;
			
		case EXOSIP_REGISTRATION_REFRESHED:
			printf("REGISTRATION_REFRESHED\n");
			break;

		case EXOSIP_REGISTRATION_TERMINATED:  
			printf("Registration terminated\n");
			break;  
		/* INVITE related events within calls */
		case EXOSIP_CALL_INVITE:  
			 printf ("Received a INVITE msg from %s:%s, UserName is %s, password is %s\n",je->request->req_uri->host,
			je->request->req_uri->port, je->request->req_uri->username, je->request->req_uri->password);
			calling = 1;
			eXosip_lock();
			eXosip_call_send_answer (je->tid, 180, NULL);
			i = eXosip_call_build_answer (je->tid, 200, &answer);
			if (i != 0)
			{
			printf ("This request msg is invalid!Cann't response!/n");
			eXosip_call_send_answer (je->tid, 400, NULL);
			}
			else
			{
			char localip[128]; 
			eXosip_guess_localip(AF_INET, localip, 128); 
			snprintf (tmp, 4096,
					"v=0\r\n"
					"o=- 0 0 IN IP4 %s\r\n"
					"s=No Name\r\n"
					"c=IN IP4 %s\r\n"
					"t=0 0\r\n"
					"m=audio %d RTP/AVP 0\r\n"
					"a=rtpmap:0 PCMU/8000\r\n"
					"m=video 54002 RTP/AVP 96\r\n"
					//"b=AS:4096\r\n"
					"a=rtpmap:96 H264/90000\r\n"
					//"a=fmtp:96 packetization-mode=1;\r\n"
					,localip, localip,audiosendparam.local_port
			   );

			//设置回复的SDP消息体,下一步计划分析消息体
			//没有分析消息体，直接回复原来的消息，这一块做的不好。
			osip_message_set_body (answer, tmp, strlen(tmp));
			osip_message_set_content_type (answer, "application/sdp");
			eXosip_call_send_answer (je->tid, 200, answer);
			printf ("send 200 over!\n");
			}
			eXosip_unlock ();
     
			printf ("the INFO is :\n");
			//得到消息体,该消息就是SDP格式.
			remote_sdp = eXosip_get_remote_sdp (je->did);
			con_req = eXosip_get_audio_connection(remote_sdp);
			md_audio_req = eXosip_get_audio_media(remote_sdp); 
			md_video_req = eXosip_get_video_media(remote_sdp); 
			char *remote_sdp_str=NULL;
			sdp_message_to_str(remote_sdp,&remote_sdp_str);
			printf("remote_sdp_str=======================\n%s\n",remote_sdp_str);
			
			char *payload_str; 
			int pos = 0;
			printf("audio info:----------------\n");
			while (!osip_list_eol ( (const osip_list_t *)&md_audio_req->m_payloads, pos))
			{
				payload_str = (char *)osip_list_get(&md_audio_req->m_payloads, pos);//获取媒体的pt（0,8）
				sdp_attribute_t *at;
				at = (sdp_attribute_t *) osip_list_get ((const osip_list_t *)&md_audio_req->a_attributes, pos);
				printf("payload_str=%s,m_media=%s\n",payload_str,at->a_att_value);
				pos++;
			}
			
			printf("video info:----------------\n");
			 pos = 0;
			while (!osip_list_eol ( (const osip_list_t *)&md_video_req->m_payloads, pos))
			{
				payload_str = (char *)osip_list_get(&md_video_req->m_payloads, pos);//获取媒体的pt（96）
				sdp_attribute_t *at;
				at = (sdp_attribute_t *) osip_list_get ((const osip_list_t *)&md_video_req->a_attributes, pos);
				printf("payload_str=%s,m_media=%s\n",payload_str,at->a_att_value);
				pos++;
			}
			printf("--------------------------------------------------\n");
			printf("SDP:conn_add=%s,audio_port=%s,video_port=%s\n",con_req->c_addr,md_audio_req->m_port,md_video_req->m_port);
			printf("--------------------------------------------------\n");
			
			char ip[20];
			strcpy(ip,con_req->c_addr);//这个地方不知道什么原因 
			//传入音频线程参数
			audiosendparam.dest_ip=ip; 
			audiorecvparam.dest_ip=ip; 
			audiosendparam.dest_port=audiorecvparam.dest_port=atoi(md_audio_req->m_port);
			//audioparam.audio_hw = "default";
			//传入视频线程参数
			videosendparam.dest_ip=ip;
			videorecvparam.dest_ip=ip;
			videosendparam.dest_port=videorecvparam.dest_port=atoi(md_video_req->m_port);
		
			sdp_message_free(remote_sdp);
			remote_sdp = NULL;
			break;  
		case EXOSIP_CALL_REINVITE:
			printf("REINVITE\n");
			break;
		case EXOSIP_CALL_NOANSWER:
			break;
		case EXOSIP_CALL_PROCEEDING:  
			printf ("proceeding!\n");  
			break;  
		case EXOSIP_CALL_RINGING:  
			printf ("ringing!\n");  
			printf ("call_id is %d, dialog_id is %d \n", je->cid, je->did);  
			break;  
		case EXOSIP_CALL_ANSWERED:  
			printf ("ok! connected!\n");  
			call_id = je->cid;  
			dialog_id = je->did;  
			printf ("call_id is %d, dialog_id is %d \n", je->cid, je->did);  
			eXosip_call_build_ack (je->did, &ack);  
			eXosip_call_send_ack (je->did, ack);  
			
			break;  
		case EXOSIP_CALL_REDIRECTED:
			break;
		case EXOSIP_CALL_REQUESTFAILURE:
			break;
		case EXOSIP_CALL_SERVERFAILURE:
			break;
		case EXOSIP_CALL_GLOBALFAILURE:
			break;
		case EXOSIP_CALL_CANCELLED:
			break;
		case EXOSIP_CALL_TIMEOUT:
			break;
		case EXOSIP_CALL_CLOSED:  
			printf ("the call sid closed!\n");  //呼叫结束
			videorecvparam.recv_quit=0;
			videosendparam.send_quit=0;
			audiorecvparam.recv_quit=0;
			audiosendparam.send_quit=0;
			int thread_rc=-1;
			thread_rc=osip_thread_join(audio_thread_send);
			if (thread_rc==0) fprintf(stderr,GREEN"[%s]:"NONE"audio_send_thread exit\n",__FILE__);
			thread_rc=osip_thread_join(audio_thread_recv);
			if (thread_rc==0) fprintf(stderr,GREEN"[%s]:"NONE"audio_recv_thread exit\n",__FILE__);
			thread_rc=osip_thread_join(video_thread_send);
			if (thread_rc==0) fprintf(stderr,GREEN"[%s]:"NONE"video_send_thread exit\n",__FILE__);
			thread_rc=osip_thread_join(video_thread_recv);
			if (thread_rc==0) fprintf(stderr,GREEN"[%s]:"NONE"video_thread_recv exit\n",__FILE__);
			break;  
		case EXOSIP_CALL_ACK:  
			printf ("ACK received!\n");  
			call_id = je->cid;  
			dialog_id = je->did;  
			//获取到远程的sdp信息后，分别建立音频 视频４个线程
			videorecvparam.recv_quit=1;
			videosendparam.send_quit=1;
			audiorecvparam.recv_quit=1;
			audiosendparam.send_quit=1;
			if(open_audio_socket()<0) break;//打开音频发送接收socket
			//音频发送线程
			
			eXosip_lock();  
			printf("conn_add=%s,audio_port=%d\n",audiosendparam.dest_ip,audiosendparam.dest_port);
			audio_thread_send = osip_thread_create (20000, &audio_send , &audiosendparam);//开启音频线程
			if (audio_thread_send==NULL){
				fprintf(stderr,RED"[%s]:"NONE"audio_send_thread_create failed\n",__FILE__);
			}
			else{
				fprintf(stderr,GREEN"[%s]:"NONE"audio_send_thread created!\n",__FILE__);
			}
			 eXosip_unlock();
			 
			//音频接收线程
			
			eXosip_lock();  
			audio_thread_recv = osip_thread_create (20000, &audio_recv ,&audiorecvparam);//开启音频线程
			if (audio_thread_recv==NULL){
				fprintf(stderr,RED"[%s]:"NONE"audio_thread_recv failed\n",__FILE__);
			}
			else{
				fprintf(stderr,GREEN"[%s]:"NONE"audio_thread_recv created!\n",__FILE__);
			}
			 eXosip_unlock(); 
			
			
			if(open_video_socket()<0) break;//打开视频发送接收socket
			//视频发送线程
			//=======================================
			
			eXosip_lock();  
			video_thread_send = osip_thread_create (20000, video_send , &videosendparam);
			if (video_thread_send==NULL){
				fprintf(stderr,RED"[%s]:"NONE"video_thread_send failed\n",__FILE__);
			}
			else{
				fprintf(stderr,GREEN"[%s]:"NONE"video_thread_send created!\n",__FILE__);
			}
			 eXosip_unlock(); 
			 
			//视频接收线程
			
			eXosip_lock();  
			video_thread_recv = osip_thread_create (20000, video_recv,&videorecvparam);
			if (video_thread_recv==NULL){
				fprintf(stderr,RED"[%s]:"NONE"video_thread_recv failed\n",__FILE__);
			}
			else{
				fprintf(stderr,GREEN"[%s]:"NONE"video_thread_recv created!\n",__FILE__);
			}
			eXosip_unlock(); 
			
			break;  
		case EXOSIP_MESSAGE_NEW:
			printf("EXOSIP_MESSAGE_NEW:");
			
			if (MSG_IS_OPTIONS (je->request)) //如果接受到的消息类型是OPTIONS
			{
				printf("options\n");
			}
			if (MSG_IS_MESSAGE (je->request))//如果接受到的消息类型是MESSAGE
			{
				osip_body_t *body;
				osip_message_get_body (je->request, 0, &body);
				printf ("message: %s\n", body->body);
			}
				eXosip_message_build_answer (je->tid, 200,&answer);//收到OPTIONS,必须回应200确认，否则无法callin，返回500错误
				eXosip_message_send_answer (je->tid, 200,answer);
			break;
        case EXOSIP_CALL_MESSAGE_NEW:
                        printf("EXOSIP_CALL_MESSAGE_NEW\n");
                        //osip_body_t *msg_body;
                        //osip_message_get_body (je->request, 0, &msg_body);
                        //printf ("call_message: %s\n", msg_body->body);
                        //eXosip_message_build_answer (je->tid, 200,&answer);//收到OPTIONS,必须回应200确认，否则无法callin，返回500错误
			//eXosip_message_send_answer (je->tid, 200,answer);
            break;
		case EXOSIP_CALL_MESSAGE_PROCEEDING:
			break;
		case EXOSIP_MESSAGE_ANSWERED:      /**< announce a 200ok  */
		case EXOSIP_MESSAGE_REDIRECTED:     /**< announce a failure. */
		case EXOSIP_MESSAGE_REQUESTFAILURE:  /**< announce a failure. */
		case EXOSIP_MESSAGE_SERVERFAILURE:  /**< announce a failure. */
		case EXOSIP_MESSAGE_GLOBALFAILURE:    /**< announce a failure. */
			break;
		default: 
			printf ("other response:type=%d\n",je->type);   
			break;  
      }  
      eXosip_event_free(je);  
  }  
}  
 
 
 
 
  
int   main (int argc, char *argv[])  
{  
 
	int regid=0;
	osip_message_t *reg = NULL;
	printf("r     向服务器注册\n");  
	printf("c     取消注册\n");  
	printf("i     发起呼叫请求\n");  
	printf("h     挂断\n");  
	printf("q     退出程序\n");  
	printf("s     执行方法INFO\n");  
	printf("m     执行方法MESSAGE\n");  


  if (eXosip_init() != 0)  {  
      printf ("Couldn't initialize eXosip!\n");  
      return -1;  
    }  

  if ( eXosip_listen_addr (IPPROTO_UDP, NULL, 5062, AF_INET, 0) != 0)  {  
      eXosip_quit ();  
      fprintf (stderr, "Couldn't initialize transport layer!\n");  
      return -1;  
    }  
    


  event_thread = osip_thread_create (20000, sipEventThread,NULL);
  if (event_thread==NULL){
      fprintf (stderr, "event_thread_create failed");
      exit (1);
    }
    else{
		 fprintf (stderr, "event_thread created!\n");
	}

   
    

  while (quit_flag)   {  
      printf ("please input the comand:\n");  
        
      scanf ("%c", &command);  
      getchar();  
        
      switch (command)  
    {  
	//--------------------注册----------------------------
    case 'r':  
		printf ("use [%s] start register!\n",fromuser); 
		eXosip_add_authentication_info (fromuser, userid, passwd, NULL, NULL);
		regid = eXosip_register_build_initial_register(fromuser, proxy,NULL, 3600, &reg);
		eXosip_register_send_register(regid, reg);
		quit_flag = 1;  
		break;  
	//--------------------呼叫----------------------------
    case 'i':/* INVITE */  
      i = eXosip_call_build_initial_invite (&invite, dest_call, source_call, NULL, "This si a call for a conversation");  
      if (i != 0)  
        {  
          printf ("Intial INVITE failed!\n");  
          break;  
        }  
      char localip2[128]; 
	  eXosip_guess_localip(AF_INET, localip2, 128); 
      snprintf (tmp, 4096,  
                    "v=0\r\n"
                    "o=- 0 0 IN IP4 %s\r\n"
                    "s=No Name\r\n"
                    "c=IN IP4 %s\r\n"
                    "t=0 0\r\n"
                    "m=audio 54000 RTP/AVP 8\r\n"
                    "a=rtpmap:8 PCMA/8000\r\n"
                    "m=video 54002 RTP/AVP 96\r\n"
                    "a=rtpmap:96 H264/90000\r\n",
                    localip2,localip2
            );  
      osip_message_set_body (invite, tmp, strlen(tmp));  
      osip_message_set_content_type (invite, "application/sdp");  
        
      eXosip_lock ();  
      i = eXosip_call_send_initial_invite (invite);  
      eXosip_unlock ();  
      break;  
	//--------------------挂断----------------------------
    case 'h':  
      printf ("Holded !\n");  
      eXosip_lock ();  
      eXosip_call_terminate (call_id, dialog_id);  
      eXosip_unlock ();  
      break;  
	//------------------注销------------------------
    case 't':
	videosendparam.send_quit=0;
	videorecvparam.recv_quit=0;
	break;
      case 'c':  
      printf ("This modal isn't commpleted!\n");  
/*//注销是时间为0
		eXosip_lock ();  
		i = eXosip_register_build_register (regid, 0, NULL);  
		if (i < 0)  
		{  
		eXosip_unlock ();  
		break; 
		}  
		eXosip_register_send_register (regid, reg);  
		eXosip_unlock ();  
*/
      break;  
      //-------------------消息---------------------
    case 's':  
      printf ("send info\n");  
      eXosip_call_build_info (dialog_id, &info);  
      snprintf (tmp , 4096,  "hello,aphero");  
      osip_message_set_body (info, tmp, strlen(tmp));  
      osip_message_set_content_type (info, "text/plain");  
      eXosip_call_send_request (dialog_id, info);  
      break;  
      //-----------------短信-------------------------
    case 'm':  
      printf ("send message\n");  
      eXosip_message_build_request (&message, "MESSAGE",  dest_call,source_call, NULL);  
      snprintf (tmp, 4096,"hello aphero");  
      osip_message_set_body (message, tmp, strlen(tmp));  
      osip_message_set_content_type (message, "text/plain");  
      eXosip_message_send_request (message);  
      break;  
	//--------------------退出---------------------
    case 'q': 
      quit_flag=0; 
      printf ("Exit the setup!\n");  
      break;  
    }  
    }  

	eXosip_quit ();  
  return (0);  
}  
