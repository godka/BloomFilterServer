#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/event.h"
#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/keyvalq_struct.h"
#ifdef WIN32
#include <Winsock2.h>
#pragma comment(lib,"ws2_32")
#endif
#include <stdlib.h>
#include <stdio.h>
#include "BloomFliter.h"
void* bf_buf;
#define CMP(x) strcmp(header->key, x) == 0
#ifdef WIN32
int init_win_socket()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return -1;
	}
	return 0;
}
#endif

void generic_handler(struct evhttp_request *req, void *arg)
{
	struct evbuffer *buf = evbuffer_new();
	if (!buf)
	{
		puts("failed to create response buffer \n");
		return;
	}
	const char*  reqbuf = evhttp_request_get_uri(req);
	//puts(reqbuf);
	struct evkeyvalq headers = { 0 };
	struct evkeyval *header;
	evhttp_parse_query_str(&reqbuf[1], &headers);

	for (header = headers.tqh_first; header;
		header = header->next.tqe_next) {
		if (CMP("add")){
			BF_Add(bf_buf, header->value, strlen(header->value));
			evbuffer_add(buf, "ok", 2);
			evhttp_send_reply(req, HTTP_OK, "ok", buf);
		}
		else if (CMP("contain")){
			if (BF_Contains(bf_buf, header->value, strlen(header->value))){
				evbuffer_add(buf, "true", 4);
				evhttp_send_reply(req, HTTP_OK, "ok", buf);
			}
			else{
				evbuffer_add(buf, "false", 5);
				evhttp_send_reply(req, HTTP_OK, "ok", buf);
			}
		}
		else{
			evhttp_send_reply(req, HTTP_BADREQUEST, "Error Command", buf);
		}
	}
	evbuffer_free(buf);
}

int main(int argc, char* argv [])
{
#ifdef WIN32
	init_win_socket();
#endif

	short          http_port = 6381;
	char          *http_addr = "0.0.0.0";

	struct event_base * base = event_base_new();

	struct evhttp * http_server = evhttp_new(base);
	if (!http_server){
		return -1;
	}

	int ret = evhttp_bind_socket(http_server, http_addr, http_port);
	if (ret != 0)
	{
		return -1;
	}

	evhttp_set_gencb(http_server, generic_handler, NULL);
	bf_buf = BF_Create(1024 * 1024);
	printf("Bloom Fliter Server start OK! \nServer Start at %d\n",http_port);

	event_base_dispatch(base);

	evhttp_free(http_server);
#ifdef WIN32
	WSACleanup();
#endif
	return 0;
}