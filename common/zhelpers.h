//  =====================================================================
//  zhelpers.h
//
//  Helper header file for example applications.
//  =====================================================================

#ifndef __ZHELPERS_H_INCLUDED__
#define __ZHELPERS_H_INCLUDED__

#include <zmq.h>

#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if (!defined (WIN32))

#   include <sys/time.h>

#endif

#if (defined (WIN32))
#   include <windows.h>
#endif

//  Version checking, and patch up missing constants to match 2.1
#if ZMQ_VERSION_MAJOR == 2
#   error "Please upgrade to ZeroMQ/3.2 for these examples"
#endif


//  On some version of Windows, POSIX subsystem is not installed by default.
//  So define srandom and random ourself.
#if (defined (WIN32))
#   define srandom srand
#   define random rand
#endif

//  Provide random number from 0..(num-1)
#define randof(num)  (int) ((float) (num) * random () / (RAND_MAX + 1.0))

typedef enum {
    TCP,
    UDP
} ProtocolType;

// Function declarations
void *create_context();

ProtocolType get_protocol_type(int socket_type);

void *create_socket(void *context, int socket_type, const char *connection, int timeout, char *socket_group);

int zmq_send_group(void *socket, const char *group, const char *msg, int flags);

char *s_recv(void *socket);

int s_send(void *socket, char *string);

int s_sendmore(void *socket, char *string);

void s_sleep(int msecs);

#endif  //  __ZHELPERS_H_INCLUDED__



