/*==============================================================================
 * 文件名称：net.h
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：联网模块
 * 功能描述：声明联网对战所需的网络通信接口。基于 Winsock2 非阻塞 TCP。
 * 协议格式（每行以换行符结尾）：MOVE r c / UNDO / RESIGN / RESTART
 *============================================================================*/
#ifndef GOBANG_NET_H
#define GOBANG_NET_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NET_CLOSED        0
#define NET_LISTENING     1
#define NET_CONNECTING    2
#define NET_CONNECTED     3
#define NET_DISCONNECTED  4

#define NET_MSG_NONE      0
#define NET_MSG_START     1
#define NET_MSG_MOVE      2
#define NET_MSG_UNDO      3
#define NET_MSG_RESIGN    4
#define NET_MSG_RESTART   5
#define NET_MSG_CLOSE     6

#define NET_DEFAULT_PORT  8888

int net_host_start(int port);
int net_join_start(const char *ip, int port);
void net_close(void);
int net_state(void);
const char *net_local_ip(void);
int net_poll(int *type, int *row, int *col);
int net_send(const char *line);
int net_send_move(int row, int col);
const char *net_state_text(void);

#ifdef __cplusplus
}
#endif

#endif /* GOBANG_NET_H */
