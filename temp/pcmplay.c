#include <alsa/asoundlib.h>
#include<stdio.h>


snd_pcm_t *handle;
snd_pcm_sframes_t frames;


int PcmOpen()
{

    if ( snd_pcm_open(&handle, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0) < 0 )
    {
        printf("pcm open error");
        return 0;
    }

    if (snd_pcm_set_params(handle, SND_PCM_FORMAT_U8, SND_PCM_ACCESS_RW_INTERLEAVED, 1, 8000, 1, 500000) < 0)   //0.5sec 500000
    {
        printf("pcm set error");
        return 0;
    }

    return 1;
}



void Play(unsigned char* buffer, int length)
{
    frames = snd_pcm_writei(handle, buffer, length);
    if(frames < 0)
    {
        frames = snd_pcm_recover(handle, frames, 0);
    }
}




int PcmClose()
{
    snd_pcm_close(handle);
    return 1;
}
