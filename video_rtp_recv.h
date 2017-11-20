#define NONE                      "\033[m"  
#define RED                         "\033[1;31m"
#define GREEN                   "\033[1;32m" 
#define BLUE                       "\033[1;34m"

typedef struct video_param_recv
{
int    video_rtp_socket;
char *video_hw;
char *dest_ip ;
int      dest_port;
int      local_port;
int       width;
int       height;
int       fps;
int        bitrate;
int        recv_thread;
int         recv_quit;
int         send_thread;
int         send_quit;
} video_param_recv;



typedef struct video_frame
{
unsigned char nal;
unsigned char frametype;
unsigned char flag;
unsigned long timestamp;
unsigned short seq_no;
} video_frame;

void *video_recv(void *videorecvparam);
