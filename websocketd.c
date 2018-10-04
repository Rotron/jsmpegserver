#include <libwebsockets.h>

void ws_publish(char *str, size_t len, char *topic) {
/*
    char *buf = (char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING, __FILE__, __LINE__);
    memcpy(buf + LWS_SEND_BUFFER_PRE_PADDING, str, len);
    PWS *pws = wshead;
    while(pws != NULL) {
        lws_write(pws->lws, buf + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);
        pws = pws->tail;
    }
    free(buf);
*/
}

int ws_service_callback(struct lws *w, enum lws_callback_reasons reason, void *user, void *buffer, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            for (int h = 0 ; h < 80 ; h++) {
                const char *s = lws_token_to_string(h);
                if (strncmp(s, "get", 3) == 0) {
                    int length = lws_hdr_total_length(w, h) + 1;
                    char *url = (char*)malloc(length);
                    lws_hdr_copy(w, url, length, h);
                    printf("dist:%s\n", url);
                    free (url);
                    break;
                }
            }
            break; // 有新连接，可用
        case LWS_CALLBACK_RECEIVE:
            break; // 接收到新消息
        case LWS_CALLBACK_CLOSED:
            break; // 有连接丢失
    }
    return 0;
}

void websocketd_init() {
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