

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
#include <malloc.h>
#include "video_rtp_recv.h" 
#include "omx_decode.h"
#include "queue.h"

	
#define  RTP_HEADLEN 12
int  UnpackRTPH264(unsigned char *bufIn,int len,unsigned char *bufout,video_frame *videoframe)
{
int outlen=0;
     if  (len  <  RTP_HEADLEN){
         return  -1 ;
    }

    unsigned  char *src  =  (unsigned  char * )bufIn  +  RTP_HEADLEN;
    unsigned  char  head1  =   * src; // 获取第一个字节
    unsigned  char  nal  =  head1  &   0x1f ; // 获取FU indicator的类型域，
    unsigned  char  head2  =   * (src + 1 ); // 获取第二个字节
    unsigned  char  flag  =  head2  &   0xe0 ; // 获取FU header的前三位，判断当前是分包的开始、中间或结束
    unsigned  char  nal_fua  =  (head1  &   0xe0 )  |  (head2  &   0x1f ); // FU_A nal
	//这里可以获取CSRC/sequence number/timestamp/SSRC/CSRC等rtp头部信息.
	//是否可以定义rtp头部struct,memcopy缓存20个字节到rtp头部即可获取．
    videoframe->seq_no=bufIn[2] << 8|bufIn[3] << 0;//序号，用于判断rtp乱序
    videoframe->timestamp=bufIn[4] << 24|bufIn[5] << 16|bufIn[6] << 8|bufIn[7] << 0;//时间戳，送到解码中
    videoframe->flag= src[1]&0xe0;//判断包是开始　结束
    videoframe->nal= src[0]&0x1f;//是否是fu-a,还是单个包
    videoframe->frametype= src[1]&0x1f;//判断帧类型Ｉ,还有错误


	if  (nal == 0x1c ){//fu-a＝28
		if  (flag == 0x80 ) // 开始=128
		{
			//printf("s ");
			bufout[0]=0x0;
			bufout[1]=0x0;
			bufout[2]=0x0;
			bufout[3]=0x1;
			bufout[4]=nal_fua;
			outlen  = len - RTP_HEADLEN -2+5;//-2跳过前2个字节，+5前面前导码和类型码，+5
			memcpy(bufout+5,src+2,outlen);
		}
		else   if (flag == 0x40 ) // 结束=64
		{
			//printf("e ");
			outlen  = len - RTP_HEADLEN -2 ;
			memcpy(bufout,src+2,len-RTP_HEADLEN-2);
		}
		else // 中间
		{
			//printf("c ");
			outlen  = len - RTP_HEADLEN -2 ;
			memcpy(bufout,src+2,len-RTP_HEADLEN-2);
		}
	}
	else {//当个包，1,7,8
		//printf("*[%d]* ",nal);
		bufout[0]=0x0;
		bufout[1]=0x0;
		bufout[2]=0x0;
		bufout[3]=0x1;
		memcpy(bufout+4,src,len-RTP_HEADLEN);
		outlen=len-RTP_HEADLEN+4;
	}
	return  outlen;
}





CircleQueue bufferqueue;
frame  in_avframe,out_avframe;
pthread_mutex_t mut;

void* dec_thread(void *arg){	
	struct video_param_recv *video_recv_param=arg;
	while((video_recv_param->recv_quit==1)){
		if(IsQueueEmpty(&bufferqueue))  	{continue;}
		if(bufferqueue.count<5){ usleep(1000);continue;}
		pthread_mutex_lock(&mut);
		out_avframe=DeQueue(&bufferqueue);//从队列中读取数据
		pthread_mutex_unlock(&mut);
		//printf("[DeQueue]:[%d-%d]=%2x %2x %2x %2x\n",bufferqueue.count,out_avframe.seqnum,out_avframe.data[4],out_avframe.data[5],out_avframe.data[6],out_avframe.data[7]);
		//printf("[DeQueue]:[%d-%d]=%s\n",bufferqueue.count,out_avframe.seqnum,out_avframe.data);
		//if((out_avframe.seqnum) % 30==0) 
		printf("[DeQueue]:[%d]:[%d]\n",bufferqueue.count,out_avframe.seqnum);
		if(omx_decode(out_avframe.data,out_avframe.len,out_avframe.timestamp)!=0) printf("omx_decode fail\n");
	}
	return NULL;
}


#define LUANXU 0 //乱序后重新排序
#define NOLUANXU 0//乱序不排序
void *video_recv(void *videorecvparam){
	unsigned char buffer[2048];
	unsigned char outbuffer[2048];
	unsigned char *in_buffer;//
	video_frame videoframe;

	unsigned short last_seq_no=0;//上一个包的序号
#if LUANXU
	int i;
	int count=0;
	unsigned short start_seq_no=0;//开始乱序的序号，eg.123546  =3
	#define  BUFCOUNT 20
	frame  luan_avframe[BUFCOUNT];
	//memset(&luan_avframe, 0, sizeof(luan_avframe)); 
	
#endif
	struct video_param_recv *video_recv_param=videorecvparam;
	memset(&videoframe, 0, sizeof(videoframe)); 
	int outbuffer_len;
	pthread_t decthread;
		
	if(omx_init()!=0){
		fprintf(stderr,RED"[%s]:" NONE"omx_init fail\n",__FILE__);
		return NULL;
	}
	
	InitQueue(&bufferqueue);
	//if(pthread_create(&decthread,NULL,dec_thread,videorecvparam)<0) printf ("Create dec pthread error!\n");	
	while((video_recv_param->recv_quit==1)){
		usleep(100);
		int recv_len;
		//outbuffer=(unsigned char *)malloc(2048);
		bzero(buffer, sizeof(buffer));
		recv_len = recv(video_recv_param->video_rtp_socket, buffer, sizeof(buffer),0);
		if(recv_len<0) 	continue; 
		outbuffer_len=UnpackRTPH264(buffer,recv_len,outbuffer,&videoframe);
		//fprintf(stderr,BLUE"[%s]:" NONE"seq_no[%d],flage[%d],video_recv_len[%d],outbuffer_len[%d]\n",__FILE__,videoframe.seq_no,videoframe.flag,recv_len,outbuffer_len);
		//printf("frame:seq_no[%d],nal[%d],type[%d],flage[%d]\n",videoframe.seq_no,videoframe.nal,videoframe.frametype,videoframe.flag);
		//in_buffer=(unsigned char *)malloc(outbuffer_len);//分配内存，保存到队列中去
		//memcpy(in_buffer,outbuffer,outbuffer_len);
#if LUANXU		
		//这里注意rtp乱序
		if((videoframe.seq_no!=0 && videoframe.seq_no!=(last_seq_no+1))){//乱序
			if(start_seq_no==0)start_seq_no=last_seq_no;
			 printf("rtp seq error:S[%d][%d]F[%d]C[%d]\n",start_seq_no,count,last_seq_no,videoframe.seq_no);
				luan_avframe[videoframe.seq_no-start_seq_no].data=in_buffer;
				luan_avframe[videoframe.seq_no-start_seq_no].len=outbuffer_len;
				luan_avframe[videoframe.seq_no-start_seq_no].timestamp=videoframe.timestamp;
				luan_avframe[videoframe.seq_no-start_seq_no].seqnum=videoframe.seq_no;
				last_seq_no=videoframe.seq_no;//保存上一个包的时间戳
				//count++;//乱包计数//注释掉，可以跳过乱包，不存入队列
				continue;
		 }else{//正确序
				if(start_seq_no!=0){
					for(i=0;i<count;i++){
						pthread_mutex_lock(&mut);
						//EnQueue(&bufferqueue,luan_avframe[i+1]);//将乱包重新排序后存入队列
						pthread_mutex_unlock(&mut);
						//printf("[EnQueue]:[%d-%d]=%2x %2x %2x %2x\n",bufferqueue.count,luan_avframe[i+1].seqnum,luan_avframe[i+1].data[4],luan_avframe[i+1].data[5],luan_avframe[i+1].data[6],luan_avframe[i+1].data[7]);
					}
					start_seq_no=0;
					count=0;
				}
				in_avframe.data=in_buffer;
				in_avframe.len=outbuffer_len;
				in_avframe.timestamp=videoframe.timestamp;
				in_avframe.seqnum=videoframe.seq_no;
			 if(!IsQueueFull(&bufferqueue)){ 
				 pthread_mutex_lock(&mut);
				 EnQueue(&bufferqueue,in_avframe);	
				 pthread_mutex_unlock(&mut);	
				// printf("[EnQueue]:[%d-%d]=%2x %2x %2x %2x\n",bufferqueue.count,in_avframe.seqnum,in_avframe.data[4],in_avframe.data[5],in_avframe.data[6],in_avframe.data[7]);
				 }
		 }
		last_seq_no=videoframe.seq_no;//保存上一个包的时间戳
		if(outbuffer_len==0) continue;
#endif
#if NOLUANXU
		if((videoframe.seq_no!=0 && videoframe.seq_no!=(last_seq_no+1))){//乱序
			 printf("rtp seq error:F[%d]C[%d]\n",last_seq_no,videoframe.seq_no);
		 }
		last_seq_no=videoframe.seq_no;//保存上一个包的时间戳
		in_avframe.data=in_buffer;
		in_avframe.len=outbuffer_len;
		in_avframe.timestamp=videoframe.timestamp;
		in_avframe.seqnum=videoframe.seq_no;
		
		if(!IsQueueFull(&bufferqueue)){
			pthread_mutex_lock(&mut);
			EnQueue(&bufferqueue,in_avframe);
			pthread_mutex_unlock(&mut);
		}
		//printf("[EnQueue]:[%d-%d]=%2x %2x %2x %2x\n",bufferqueue.count,in_avframe.seqnum,in_avframe.data[4],in_avframe.data[5],in_avframe.data[6],in_avframe.data[7]);
		//printf("[EnQueue]:[%d-%d]=%s\n",bufferqueue.count,in_avframe.seqnum,in_avframe.data);
#endif
		
		//直接解码
		if((videoframe.seq_no!=0 && videoframe.seq_no!=(last_seq_no+1))){//乱序
			printf("rtp seq error:F[%d]C[%d]\n",last_seq_no,videoframe.seq_no);
		}
		last_seq_no=videoframe.seq_no;//保存上一个包的时间戳
		if(omx_decode(outbuffer,outbuffer_len,videoframe.timestamp)!=0) printf("omx_decode fail\n");//直接解码
		
	}

	if(omx_deinit()!=0){ printf("omx_deinit fail\n");}
	close(video_recv_param->video_rtp_socket);
	return NULL;
}

