#include <stdio.h>
#include <libwebsockets.h>
#include "httpd.h"
#include "main.h"

struct topiclist *topiclisthead = NULL;

void addnewclient(struct lws *lws, char *topic) {
    struct topiclist *topiclist;
    if (topiclisthead == NULL) {
        topiclisthead = (struct topiclist*)malloc(sizeof(struct topiclist));
        topiclisthead->topic = topic;
        topiclisthead->clientlist = NULL;
        topiclisthead->tail = NULL;
        topiclist = topiclisthead;
    } else {
        topiclist = topiclisthead;
        while (1) {
            if (strcmp(topiclist->topic, topic) == 0) {
                break;
            }
            if (topiclist->tail == NULL) { // 到达最后一个也没有找到相同topic的list
                topiclist->tail = (struct topiclist*)malloc(sizeof(struct topiclist));
                topiclist = topiclist->tail;
                topiclist->topic = topic;
                topiclist->clientlist = NULL;
                topiclist->tail = NULL;
                break;
            }
            topiclist = topiclist->tail;
        }
    }
    struct clientlist *clientlist = (struct clientlist*)malloc(sizeof(struct clientlist));
    clientlist->lws = lws;
    clientlist->tail = NULL;
    if (topiclist->clientlist != NULL) {
        struct clientlist *tmp = topiclist->clientlist;
        while (tmp->tail != NULL) {
            tmp = tmp->tail;
        }
        tmp->tail = clientlist;
    } else {
        topiclist->clientlist = clientlist;
    }
}

int removesubclient (struct topiclist *topiclist, struct lws *lws) {
    struct clientlist *lastclientlist = topiclist->clientlist;
    if (lastclientlist->lws == lws) { // 删除的是第一个client
        topiclist->clientlist = lastclientlist->tail;
        free (lastclientlist);
        return 1;
    }
    struct clientlist *clientlist = lastclientlist->tail;
    while (1) {
        if (clientlist == NULL) {
            return 0;
        }
        if (clientlist->lws == lws) {
            lastclientlist->tail = clientlist->tail;
            free(clientlist);
            return 1;
        }
        lastclientlist = clientlist;
        clientlist = clientlist->tail;
    }
}

void removeclient (struct lws *lws) {
    struct topiclist *lasttopiclist = topiclisthead;
    if (removesubclient (lasttopiclist, lws)) { // client是topiclist中的第一个topic
        if (lasttopiclist->clientlist == NULL) {
            topiclisthead = lasttopiclist->tail;
            free (lasttopiclist->topic);
            free (lasttopiclist);
        }
        return;
    }
    struct topiclist *topiclist = lasttopiclist->tail;
    while (1) {
        if (removesubclient (topiclist, lws)) {
            if (topiclist->clientlist == NULL) {
                lasttopiclist->tail = topiclist->tail;
                free (topiclist->topic);
                free (topiclist);
            }
            return;
        }
        lasttopiclist = topiclist;
        topiclist = topiclist->tail;
    }
}

struct clientlist *findclientlist (char *topic) {
    struct topiclist *topiclist = topiclisthead;
    while (1) {
        if (topiclist == NULL) {
            return NULL;
        }
        if (strcmp(topiclist->topic, topic) == 0) {
            return topiclist->clientlist;
        }
        topiclist = topiclist->tail;
    }
}

void showtopology (char *filename, int line) {
    printf("==================in:%s,at:%d=========================\n", filename, line);
    struct topiclist *topiclist = topiclisthead;
    while (topiclist != NULL) {
        printf("topic:%s || ", topiclist->topic);
        struct clientlist *clientlist = topiclist->clientlist;
        for (int i = 0 ; clientlist != NULL ; i++) {
            printf("%d,", i);
            clientlist = clientlist->tail;
        }
        printf("\n");
        topiclist = topiclist->tail;
    }
}

int main(int argc, char *argv[]) {
    int res = starthttpd();
    if (res != 0) {
        printf ("http server launch fail!!!\n");
        return res;
    }
    websocketd_init ();
    return res;
}