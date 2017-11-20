#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "video_net_recv.h"

struct net_handle *net_open(struct net_param params)
{
    struct net_handle *handle = (struct net_handle *) malloc(sizeof(struct net_handle));
    if (!handle)
    {
        printf("--- malloc net handle failed\n");
        return NULL;
    }
    CLEAR(*handle);
    handle->params.type = params.type;
    handle->params.serip = params.serip;
    handle->params.serport = params.serport;
    handle->params.localport = params.localport;
    if (handle->params.type == TCP)
        handle->sktfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
    else
        // UDP
        handle->sktfd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);

    if (handle->sktfd < 0)
    {
        printf("--- create socket failed\n");
        free(handle);
        return NULL;
    }

    //设置超时
    struct timeval timeout={1,0};//3s
    //int ret=setsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    if(setsockopt(handle->sktfd,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout))<0){
	printf("setsockopt timeout fail");
	}

    //端口复用
    int flag=1;
    if( setsockopt(handle->sktfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) <0)  {  
        fprintf(stderr,"socket setsockopt error\n");
    }  

     //绑定本地端口
    struct sockaddr_in local;  
    local.sin_family=AF_INET;  
    local.sin_port=htons(handle->params.localport);            ///监听端口  
    local.sin_addr.s_addr=INADDR_ANY;       ///本机  
    if(bind(handle->sktfd,(struct sockaddr*)&local,sizeof(local))<0) {
		fprintf(stderr, "udp port bind error\n");
    }
    handle->server_sock.sin_family = AF_INET;
    handle->server_sock.sin_port = htons(handle->params.serport);
    handle->server_sock.sin_addr.s_addr = inet_addr(handle->params.serip);
    handle->sersock_len = sizeof(handle->server_sock);

    int ret = connect(handle->sktfd, (struct sockaddr *) &handle->server_sock,handle->sersock_len);
    if (ret < 0)
    {
        printf("--- connect to server failed\n");
        close(handle->sktfd);
        free(handle);
        return NULL;
    }

    return handle;
}

void net_close(struct net_handle *handle)
{
    close(handle->sktfd);
    free(handle);
}

int net_send(struct net_handle *handle, void *data, int size)
{
    return send(handle->sktfd, data, size, 0);
}

int net_recv(struct net_handle *handle, void *data, int size)
{
    return recv(handle->sktfd, data, size, 0);
}

