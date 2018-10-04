#ifndef __WEBSOCKETD_H__
#define __WEBSOCKETD_H__

void ws_publish(char *str, size_t len, char *topic);
void websocketd_init();

#endif
