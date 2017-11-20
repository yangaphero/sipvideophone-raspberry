#define NONE                      "\033[m"  
#define RED                         "\033[1;31m"
#define GREEN                   "\033[1;32m" 
#define BLUE                       "\033[1;34m"

typedef struct video_param_send
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
} video_param_send;

struct video_param_send *video_send_param;

void *video_send(void *videosendparam);


