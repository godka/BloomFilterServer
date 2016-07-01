#pragma once
#if (defined _WIN32) || (defined WIN32)
#	define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"

#if (defined _WIN32) || (defined WIN32)
#	include <WinSock2.h>
#	pragma comment(lib,"ws2_32")
#	pragma comment(lib,"libevent.lib")
#	pragma comment(lib,"libevent_core.lib")
#	pragma comment(lib,"libevent_extras.lib")
#else
#	include <wchar.h>
#	include <unistd.h>
#	include <assert.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <arpa/inet.h>
#	include <netinet/in.h>
#	include <time.h>
#	include <unistd.h>
#	include <fcntl.h>
#endif