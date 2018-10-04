#include <microhttpd.h>
#include <string.h>
#include <stdio.h>

int connectionHandler(void *cls,
            struct MHD_Connection *connection,
            const char *url,
            const char *method,
            const char *version,
            const char *upload_data,
            size_t *upload_data_size,
            void ** ptr) {
    static int dummy;
    if (&dummy != *ptr) {
        /* The first time only the headers are valid, do not respond in the first round... */
        *ptr = &dummy;
        return MHD_YES;
    }
    if (*upload_data_size != 0) {
        // send_packet();
        *upload_data_size = 0;
        return MHD_YES;
    }

    char *html = "This is jsmpegserver, power by https://www.worldflying.cn";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(html), (void*) html, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Content-Type", "text/plain");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    // MHD_add_response_header(response, "expires", "Fri, 31 Dec 9999 23:59:59 GMT");
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return MHD_YES;
}

int starthttpd() {
    struct MHD_Daemon *d = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY, 8001, NULL, NULL, &connectionHandler, NULL, MHD_OPTION_END);
    if (d == NULL) {
        return -1;
    }
    return 0;
}