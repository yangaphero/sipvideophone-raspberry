


#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef unsigned int U32;
#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define UNUSED(expr) do { (void)(expr); } while (0)



enum net_t
{
    UDP = 0, TCP
};

struct audio_net_param
{
        enum net_t type;		// UDP or TCP?
        char * serip;			// server ip, eg: "127.0.0.1"
        int serport;			// server port, eg: 8000
	int localport;
};
struct audio_net_handle
{
    int sktfd;
    struct sockaddr_in server_sock;
    int sersock_len;

    struct audio_net_param params;
};
struct audio_net_handle;

struct audio_net_handle *audio_net_open(struct audio_net_param params);

int audio_net_send(struct audio_net_handle *handle, void *data, int size);

int audio_net_recv(struct audio_net_handle *handle, void *data, int size);

void audio_net_close(struct audio_net_handle *handle);


