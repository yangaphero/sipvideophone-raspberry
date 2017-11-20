#define NONE                      "\033[m"  
#define RED                         "\033[1;31m"
#define GREEN                   "\033[1;32m" 
#define BLUE                       "\033[1;34m"

typedef struct
{
    /** byte 0 */
    unsigned char csrc_len:4;        /** expect 0 */
    unsigned char extension:1;       /** expect 1, see RTP_OP below */
    unsigned char padding:1;         /** expect 0 */
    unsigned char version:2;         /** expect 2 */
    /** byte 1 */
    unsigned char payload:7;         /** stream type */
    unsigned char marker:1;          /** when send the first framer,set it */
    /** bytes 2, 3 */
    unsigned short seq_no;
    /** bytes 4-7 */
    unsigned  long timestamp;
    /** bytes 8-11 */
    unsigned long ssrc;              /** stream number is used here. */
} RTP_FIXED_HEADER;


typedef struct audio_param_send
{
int    audio_rtp_socket;
char *audio_hw;
char *dest_ip ;
int      dest_port;
int      local_port;
int         recv_quit;
int         send_quit;
} audio_param_send;

void *audio_send(void *AudioSendParam) ;
