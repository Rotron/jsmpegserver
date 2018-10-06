#include <string.h>
#include "websocketd.h"
#include "main.h"

#ifndef LWS_MAX_SOCKET_IO_BUF
#define LWS_MAX_SOCKET_IO_BUF 4096
#endif

void sendclients (char *buf, size_t len, int breakpoint, struct clientlist *clientlist) {
    static size_t total = 0;
    total += len;
    while(clientlist != NULL) {
        int mode = 0;
        if (clientlist->mode) {
            mode |= LWS_WRITE_CONTINUATION;
        } else {
            mode |= LWS_WRITE_BINARY;
            clientlist->mode = 1;
        }
        if (breakpoint) {
            clientlist->mode = 0;
        } else {
            mode |= LWS_WRITE_NO_FIN;
        }
        lws_write(clientlist->lws, buf + LWS_SEND_BUFFER_PRE_PADDING, len, mode);
        clientlist = clientlist->tail;
    }
}

void ws_publish (const char *blob, size_t len, const char *topic) {
    struct topiclist *topiclist = findtopicobj (topic);
    if (topiclist == NULL) {
        return;
    }
    struct clientlist *clientlist = topiclist->clientlist;
    size_t remainlen = len;
    while (1) {
        if (remainlen > LWS_MAX_SOCKET_IO_BUF) {
            char *buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + LWS_MAX_SOCKET_IO_BUF + LWS_SEND_BUFFER_POST_PADDING);
            memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, blob + len - remainlen, LWS_MAX_SOCKET_IO_BUF);
            if (topiclist->breakpoint) {
                sendclients (buf, LWS_MAX_SOCKET_IO_BUF, 1, clientlist);
                topiclist->breakpoint = 0;
            } else {
                sendclients (buf, LWS_MAX_SOCKET_IO_BUF, 0, clientlist);
            }
            free (buf);
            remainlen -= LWS_MAX_SOCKET_IO_BUF;
        } else {
            char *buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + remainlen + LWS_SEND_BUFFER_POST_PADDING);
            memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, blob + len - remainlen, remainlen);
            if (topiclist->breakpoint) {
                sendclients (buf, remainlen, 1, clientlist);
                topiclist->breakpoint = 0;
            } else {
                sendclients (buf, remainlen, 0, clientlist);
            }
            free (buf);
            break;
        }
    }
}

char *getlwstopic(struct lws *lws) {
    char *topic = NULL;
    int length = lws_hdr_total_length(lws, WSI_TOKEN_GET_URI) + 1;
    if (length > 1) { // websocket连接通常借助get提交握手，else部分只是以防万一。
        topic = (char*)malloc(length);
        lws_hdr_copy(lws, topic, length, WSI_TOKEN_GET_URI);
    } else {
        length = lws_hdr_total_length(lws, WSI_TOKEN_POST_URI) + 1;
        if (length > 1) {
            topic = (char*)malloc(length);
            lws_hdr_copy(lws, topic, length, WSI_TOKEN_POST_URI);
        } else {
            length = lws_hdr_total_length(lws, WSI_TOKEN_OPTIONS_URI) + 1;
            if (length > 1) {
                topic = (char*)malloc(length);
                lws_hdr_copy(lws, topic, length, WSI_TOKEN_POST_URI);
            }
        }
    }
    return topic;
}

int ws_service_callback(struct lws *lws, enum lws_callback_reasons reason, void *user, void *buffer, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: // 有新连接，可用
            addnewclient(lws, getlwstopic(lws));
            showtopology (__FILE__, __LINE__);
            break;
        case LWS_CALLBACK_RECEIVE:
            break; // 接收到新消息
        case LWS_CALLBACK_CLOSED:
            removeclient(lws);
            showtopology (__FILE__, __LINE__);
            break; // 有连接丢失
    }
    return 0;
}

void setbreakpoint() {
    struct topiclist *topiclist = gettopiclisthead ();
    while (topiclist != NULL) {
        topiclist->breakpoint = 1;
        topiclist = topiclist->tail;
    }
}

void websocketd_init () {
    signal(SIGALRM, setbreakpoint);
    struct itimerval itv;
    itv.it_value.tv_sec = itv.it_interval.tv_sec = 0;
    itv.it_value.tv_usec = itv.it_interval.tv_usec = 50*1000;
    setitimer(ITIMER_REAL, &itv, NULL);
    struct lws_context_creation_info info;
    struct lws_protocols protocol;
    memset(&info, 0, sizeof(info));
    memset(&protocol, 0, sizeof(protocol));
    protocol.name = "www.worldflying.cn"; 
    protocol.callback = ws_service_callback; //这里设置回调函数
    protocol.rx_buffer_size = 512*1024*1024;
    info.port = 8002; //websocket的端口号
    info.protocols = &protocol;
    struct lws_context *context = lws_create_context(&info);
    while(1) {
        lws_service(context, 0x7fffffff);
    }
    lws_context_destroy(context);
}