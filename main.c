#include <stdio.h>
#include "httpd.h"

/*
struct topiclist {
    char *topic;
    struct topiclist *tail;
};
struct topiclist topiclisthead = NULL;
struct topiclist topiclistnow = NULL;
*/

int main(int argc, char *argv[]) {
    int res = starthttpd();
    if (res != 0) {
        printf ("http server launch fail!!!\n");
        return res;
    }
    websocketd_init();
    return res;
}