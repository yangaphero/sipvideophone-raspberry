#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bcm_host.h"
#include "ilclient.h"
#include "common.h"
int omx_init();
int omx_decode(unsigned char *videobuffer,int videobuffer_len,unsigned long timestamp);
int omx_deinit();
int SetAlpha(int alpha);
int SetSpeed(int speed);
int SetRect(int x1,int y1,int x2,int y2);
