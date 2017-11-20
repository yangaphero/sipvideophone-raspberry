#include "alsa/asoundlib.h"

//11KHz支持，发声时间加长，音速变慢
#define  SAMPLE_RATE  48000
//#define  SAMPLE_RATE  11000

//的确是两个声道交叉响
#define  CHANNELS    1
//#define  CHANNELS    2

//如果latency过小，会使得snd_pcm_writei()丢发声数据，产生short write现象
#define  LATENCY   (1000000)//1sec

#define NBLOCKS 16
#define BLOCK_SIZE 1024
unsigned char buffer[NBLOCKS*BLOCK_SIZE];     /* Here:some random da
ta :Future: User Layer Sound Data*/

static char *device = "default";   /* playback device */

#define  RANDOM_PLAY 1
#define  COMMON_PLAY 2

static unsigned char * get_user_layer_pcm_data(int flag)
{
    int i;
    for (i = 0; i < sizeof(buffer); i++) {
        if(flag == RANDOM_PLAY)
            buffer[i] = random() & 0xff;
        else
            buffer[i] = i & 0xff;
    }
    return buffer;
}

int main(int argc, char ** argv)
{
    int i;
    int err, ret_code = 0;
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;

    if(argc == 2)
        get_user_layer_pcm_data(RANDOM_PLAY);//Here Future to get pcm data from user layer
    else
        get_user_layer_pcm_data(COMMON_PLAY);//Here Future to get pcm data from user layer

    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("Playback open error: %s/n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((err = snd_pcm_set_params(handle,
                    SND_PCM_FORMAT_U8,
                    SND_PCM_ACCESS_RW_INTERLEAVED,/* snd_pcm_readi/snd_pcm_writei access */
                    CHANNELS, //Channels
                    SAMPLE_RATE, //sample rate in Hz
                    1, //soft_resample
                    LATENCY)) < 0) {   /* latency: us 0.5sec */
        printf("Playback open error: %s/n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 10; i++) { //play 10 times, every play 16KB snd size
        frames = snd_pcm_writei(handle, buffer, sizeof(buffer));
        if (frames < 0)
            frames = snd_pcm_recover(handle, frames, 0);
        if (frames < 0) {
            printf("snd_pcm_writei failed: %s/n", snd_strerror(err));
            ret_code = -1;
            break;
        }
        if (frames > 0 && frames < (long)sizeof(buffer))
            printf("Short write (expected %li, wrote %li)/n", (long)sizeof(buffer), frames);
    }

    snd_pcm_close(handle);

    return ret_code;
}
