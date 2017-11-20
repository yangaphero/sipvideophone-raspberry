/*
 * Copyright (c) 2014 Andy Huang <andyspider@126.com>
 *
 * This file is part of Camkit.
 *
 * Camkit is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Camkit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Camkit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


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

struct net_param
{
        enum net_t type;		// UDP or TCP?
        char * serip;			// server ip, eg: "127.0.0.1"
        int serport;			// server port, eg: 8000
	int localport;
};
struct net_handle
{
    int sktfd;
    struct sockaddr_in server_sock;
    int sersock_len;

    struct net_param params;
};
struct net_handle;

struct net_handle *net_open(struct net_param params);

int net_send(struct net_handle *handle, void *data, int size);

int net_recv(struct net_handle *handle, void *data, int size);

void net_close(struct net_handle *handle);


