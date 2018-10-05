#include <libwebsockets.h>
#include "main.h"

struct wssendbuflist {
    char *buf;
    size_t len;
    int write_mode;
    struct wssendbuflist *tail;
};

void addwssendbuflist (struct wssendbuflist **bufhead, struct wssendbuflist *buf) {
    if (*bufhead == NULL) {
        *bufhead = buf;
    } else {
        struct wssendbuflist *tmp = *bufhead;
        while (tmp->tail != NULL) {
            tmp = tmp->tail;
        }
        tmp->tail = buf;
    }
}

void ws_publish(char *blob, size_t len, char *topic) {
    struct wssendbuflist *bufhead = NULL;
    size_t remainlen = len;
    size_t max_fds = getdtablesize();
    while (1) {
        if (remainlen > max_fds) {
            struct wssendbuflist *buf = (struct wssendbuflist*)malloc(sizeof(struct wssendbuflist));
            buf->buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + max_fds + LWS_SEND_BUFFER_POST_PADDING);
            memcpy(buf->buf + LWS_SEND_BUFFER_PRE_PADDING, blob + len - remainlen, max_fds);
            buf->len = max_fds;
            if (remainlen == len) {
                buf->write_mode = LWS_WRITE_BINARY;
            } else {
                buf->write_mode = LWS_WRITE_CONTINUATION;
            }
            buf->tail = NULL;
            addwssendbuflist (&bufhead, buf);
            remainlen -= max_fds;
        } else {
            struct wssendbuflist *buf = (struct wssendbuflist*)malloc(sizeof(struct wssendbuflist));
            buf->buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + remainlen + LWS_SEND_BUFFER_POST_PADDING);
            memcpy(buf->buf + LWS_SEND_BUFFER_PRE_PADDING, blob + len - remainlen, remainlen);
            buf->len = remainlen;
            if (remainlen == len) {
                buf->write_mode = LWS_WRITE_BINARY | LWS_WRITE_NO_FIN;
            } else {
                buf->write_mode = LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN;
            }
            buf->tail = NULL;
            addwssendbuflist (&bufhead, buf);
            break;
        }
    }
    struct clientlist *clientlist = findclientlist (topic);
    while(clientlist != NULL) {
        struct wssendbuflist *tmp = bufhead;
        while (tmp != NULL) {
            lws_write(clientlist->lws, tmp->buf + LWS_SEND_BUFFER_PRE_PADDING, tmp->len, tmp->write_mode);
            tmp = tmp->tail;
        }
        clientlist = clientlist->tail;
    }
    while (bufhead != NULL) {
        struct wssendbuflist *tmp = bufhead;
        free(tmp->buf);
        free(tmp);
        bufhead = bufhead->tail;
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
    struct lws_context_creation_info info;
    struct lws_protocols protocol;
    struct lws_context *context;
    memset(&info, 0, sizeof(info));
    memset(&protocol, 0, sizeof(protocol));
    protocol.callback = ws_service_callback; //这里设置回调函数
    info.port = 8002; //websocket的端口号
    info.protocols = &protocol;
    context = lws_create_context(&info);
    while(1) {
        lws_service(context, 0x7fffffff);
    }
    lws_context_destroy(context);
}