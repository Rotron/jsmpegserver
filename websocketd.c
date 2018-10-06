#include <string.h>
#include "websocketd.h"
#include "main.h"

#ifndef LWS_MAX_SOCKET_IO_BUF
#define LWS_MAX_SOCKET_IO_BUF 4096
#endif

void sendclients (char *buf, size_t len, int mode, struct clientlist *clientlist) {
    static size_t total = 0;
    total += len;
    printf("total:%d\n", total);
    while(clientlist != NULL) {
        lws_write(clientlist->lws, buf + LWS_SEND_BUFFER_PRE_PADDING, len, mode);
        clientlist = clientlist->tail;
    }
}

void ws_publish (struct DATAPIPE *datapipestart, char *topic) {
    struct DATAPIPE *datapipe = datapipestart;
    struct topiclist *topiclist = findtopicobj (topic);
    struct clientlist *clientlist = topiclist->clientlist;
    while (datapipe != NULL) {
        size_t remainlen = datapipe->len;
        while (1) {
            if (remainlen > LWS_MAX_SOCKET_IO_BUF) {
                char *buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + LWS_MAX_SOCKET_IO_BUF + LWS_SEND_BUFFER_POST_PADDING);
                memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, datapipe->blob + datapipe->len - remainlen, LWS_MAX_SOCKET_IO_BUF);
                if (datapipe == datapipestart && remainlen == datapipe->len) { // 第一个数据包
                    sendclients (buf, LWS_MAX_SOCKET_IO_BUF, LWS_WRITE_BINARY | LWS_WRITE_NO_FIN, clientlist);
                } else {
                    sendclients (buf, LWS_MAX_SOCKET_IO_BUF, LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN, clientlist);
                }
                free (buf);
                remainlen -= LWS_MAX_SOCKET_IO_BUF;
            } else if (datapipe->tail == NULL) { // 最后一个数据包且remainlen小于LWS_MAX_SOCKET_IO_BUF
                char *buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + remainlen + LWS_SEND_BUFFER_POST_PADDING);
                memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, datapipe->blob + datapipe->len - remainlen, remainlen);
                if (datapipe == datapipestart && remainlen == datapipe->len) { // 第一个数据包
                    sendclients (buf, remainlen, LWS_WRITE_BINARY, clientlist);
                } else {
                    sendclients (buf, remainlen, LWS_WRITE_CONTINUATION, clientlist);
                }
                free (buf);
                break;
            } else {
                char *buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + remainlen + LWS_SEND_BUFFER_POST_PADDING);
                memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, datapipe->blob + datapipe->len - remainlen, remainlen);
                if (datapipe == datapipestart && remainlen == datapipe->len) { // 第一个数据包
                    sendclients (buf, remainlen, LWS_WRITE_BINARY | LWS_WRITE_NO_FIN, clientlist);
                } else {
                    sendclients (buf, remainlen, LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN, clientlist);
                }
                free (buf);
                break;
            }
        }
        struct DATAPIPE *tmp = datapipe;
        datapipe = datapipe->tail;
        free(tmp->blob);
        free(tmp);
        usleep(10*1000);
    }
}

void pushdatapipe (const char *blob, size_t len, const char *topic) {
    struct topiclist *topiclist = findtopicobj (topic);
    struct DATAPIPE *datapipe = (struct DATAPIPE*)malloc(sizeof(struct DATAPIPE));
    datapipe->blob = (char*)malloc(len);
    memcpy(datapipe->blob, blob, len);
    datapipe->len = len;
    size_t size = strlen(topic) + 1;
    datapipe->tail = NULL;
    if (topiclist->datapipehead == NULL) {
        topiclist->datapipehead = datapipe;
        topiclist->datapipenow = datapipe;
    } else {
        topiclist->datapipenow->tail = datapipe;
        topiclist->datapipenow = topiclist->datapipenow->tail;
    }
}

void *popdatapipe (void *arg) {
    struct topiclist *topiclist = gettopiclisthead ();
    while (topiclist != NULL) {
        printf("%d\n", __LINE__);
        if (topiclist->datapipehead) {
            struct DATAPIPE *datapipe = topiclist->datapipehead;
            topiclist->datapipehead = NULL;
            ws_publish(datapipe, topiclist->topic);
        }
        topiclist = topiclist->tail;
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

void planthread() {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread, &attr, popdatapipe, NULL);
    pthread_attr_destroy(&attr);
}

void websocketd_init () {
    signal(SIGALRM, planthread);
    struct itimerval itv;
    itv.it_value.tv_sec = itv.it_interval.tv_sec = 10;
    itv.it_value.tv_usec = itv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);
    struct lws_context_creation_info info;
    struct lws_protocols protocol;
    memset(&info, 0, sizeof(info));
    memset(&protocol, 0, sizeof(protocol));
    protocol.name = "www.worldflying.cn"; 
    protocol.callback = ws_service_callback; //这里设置回调函数
    protocol.rx_buffer_size = 128*1024*1024;
    info.port = 8002; //websocket的端口号
    info.protocols = &protocol;
    struct lws_context *context = lws_create_context(&info);
    while(1) {
        lws_service(context, 0x7fffffff);
    }
    lws_context_destroy(context);
}