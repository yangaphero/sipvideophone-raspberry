
#define NONE                      "\033[m"  
#define RED                         "\033[1;31m"
#define GREEN                   "\033[1;32m" 
#define BLUE                       "\033[1;34m"

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP

#define BOOL int
#define TRUE 1
#define FALSE 0
typedef struct rect
{
int      x1;
int      y1;
int       x2;
int       y2;	
}rect;
typedef struct rtp_param
{
char *dest_ip ;
int      dest_port;
int      local_port;
int       thread_exit;
} rtp_param;
typedef struct dec_param
{
int       alpha;
struct  rect videorect;
int       thread_exit;
int        m_settings_changed;
} dec_param;



static inline OMX_TICKS ToOMXTime(int64_t pts)
{
  OMX_TICKS ticks;
  ticks.nLowPart = pts;
  ticks.nHighPart = pts >> 32;
  return ticks;
}
static inline int64_t FromOMXTime(OMX_TICKS ticks)
{
  int64_t pts = ticks.nLowPart | ((uint64_t)(ticks.nHighPart) << 32);
  return pts;
}
