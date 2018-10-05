#include <libwebsockets.h>
#include <string.h>
#include "main.h"

#ifndef LWS_MAX_SOCKET_IO_BUF
#define LWS_MAX_SOCKET_IO_BUF 4096
#endif

sem_t sem;

struct DATAPIPE {
    char *blob;
    size_t len;
    char *topic;
    struct DATAPIPE *tail;
};
struct DATAPIPE *datapipehead = NULL;
struct DATAPIPE *datapipenow = NULL;

void sendclients (char *buf, size_t len, int mode, char *topic) {
    struct clientlist *clientlist = findclientlist (topic);
    while(clientlist != NULL) {
        lws_write(clientlist->lws, buf + LWS_SEND_BUFFER_PRE_PADDING, len, mode);
        clientlist = clientlist->tail;
    }
}

void ws_publish(char *blob, size_t len, char *topic) {
    size_t remainlen = len;
    while (1) {
        if (remainlen > LWS_MAX_SOCKET_IO_BUF) {
            char *buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + LWS_MAX_SOCKET_IO_BUF + LWS_SEND_BUFFER_POST_PADDING);
            memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, blob + len - remainlen, LWS_MAX_SOCKET_IO_BUF);
            if (remainlen == len) {
                sendclients (buf, LWS_MAX_SOCKET_IO_BUF, LWS_WRITE_BINARY | LWS_WRITE_NO_FIN, topic);
            } else {
                sendclients (buf, LWS_MAX_SOCKET_IO_BUF, LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN, topic);
            }
            free (buf);
            remainlen -= LWS_MAX_SOCKET_IO_BUF;
        } else {
            char *buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + remainlen + LWS_SEND_BUFFER_POST_PADDING);
            memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, blob + len - remainlen, remainlen);
            if (remainlen == len) {
                sendclients (buf, len, LWS_WRITE_BINARY, topic);
            } else {
                sendclients (buf, len, LWS_WRITE_CONTINUATION, topic);
            }
            free (buf);
            break;
        }
    }
}

void pushdatapipe (const char *blob, size_t len, const char *topic) {
    struct DATAPIPE *datapipe = (struct DATAPIPE*)malloc(sizeof(struct DATAPIPE));
    datapipe->blob = (char*)malloc(len);
    memcpy(datapipe->blob, blob, len);
    datapipe->len = len;
    size_t size = strlen(topic) + 1;
    datapipe->topic = (char*)malloc(size);
    strcpy(datapipe->topic, topic);
    datapipe->tail = NULL;
    if (datapipehead == NULL) {
        datapipehead = datapipe;
        datapipenow = datapipehead;
    } else {
        datapipenow->tail = datapipe;
        datapipenow = datapipenow->tail;
    }
    sem_post(&sem);
}

void *popdatapipe (void *arg) {
    while (1) {
        sem_wait(&sem);
        struct DATAPIPE *datapipe = datapipehead;
        datapipehead = datapipehead->tail;
        ws_publish(datapipe->blob, datapipe->len, datapipe->topic);
        free(datapipe->blob);
        free(datapipe->topic);
        free(datapipe);
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

void websocketd_init () {
    sem_init(&sem, 0, 0);
    pthread_t thread;
    pthread_create(&thread, NULL, popdatapipe, NULL);
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