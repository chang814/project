/*==============================================================================
 * 文件名称：net.c
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：联网模块
 * 功能描述：实现基于 Winsock2 的非阻塞 TCP 通信，供联网对战使用。
 * 编译说明：本文件为标准 C 源文件，链接时需加 -lws2_32。
 *============================================================================*/
#include "net.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>

#define NET_RX_BUF 4096

static SOCKET s_listen = INVALID_SOCKET;
static SOCKET s_peer   = INVALID_SOCKET;
static int    s_state  = NET_CLOSED;
static int    s_wsInited = 0;
static char   s_rxbuf[NET_RX_BUF];
static int    s_rxlen  = 0;
static char   s_localIp[32] = "127.0.0.1";

static int ensure_winsock(void)
{
    WSADATA wsa;
    if (s_wsInited) return 0;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
    s_wsInited = 1;
    return 0;
}

static void set_nonblock(SOCKET s)
{
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
}

static void refresh_local_ip(void)
{
    char hostname[128];
    struct hostent *he;
    int i;

    if (gethostname(hostname, sizeof(hostname)) != 0) return;
    he = gethostbyname(hostname);
    if (he == NULL) return;
    for (i = 0; he->h_addr_list[i] != NULL; i++)
    {
        struct in_addr addr;
        memcpy(&addr, he->h_addr_list[i], sizeof(addr));
        const char *ip = inet_ntoa(addr);
        if (strncmp(ip, "127.", 4) != 0)
        {
            strncpy(s_localIp, ip, sizeof(s_localIp) - 1);
            s_localIp[sizeof(s_localIp) - 1] = '\0';
            return;
        }
    }
}

int net_host_start(int port)
{
    struct sockaddr_in addr;

    net_close();

    if (ensure_winsock() != 0) return -1;

    s_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen == INVALID_SOCKET) return -1;
    set_nonblock(s_listen);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((u_short)port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s_listen, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(s_listen);
        s_listen = INVALID_SOCKET;
        return -1;
    }
    if (listen(s_listen, 5) == SOCKET_ERROR)
    {
        closesocket(s_listen);
        s_listen = INVALID_SOCKET;
        return -1;
    }

    refresh_local_ip();
    s_state = NET_LISTENING;
    return 0;
}

int net_join_start(const char *ip, int port)
{
    struct sockaddr_in addr;
    int ret;

    net_close();

    if (ensure_winsock() != 0) return -1;
    if (ip == NULL || ip[0] == '\0') return -1;

    s_peer = socket(AF_INET, SOCK_STREAM, 0);
    if (s_peer == INVALID_SOCKET) return -1;
    set_nonblock(s_peer);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((u_short)port);
    addr.sin_addr.s_addr = inet_addr(ip);

    ret = connect(s_peer, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == 0)
    {
        s_state = NET_CONNECTED;
        return 0;
    }
    if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINPROGRESS)
    {
        s_state = NET_CONNECTING;
        return 0;
    }

    closesocket(s_peer);
    s_peer  = INVALID_SOCKET;
    s_state = NET_CLOSED;
    return -1;
}

void net_close(void)
{
    if (s_listen != INVALID_SOCKET)
    {
        closesocket(s_listen);
        s_listen = INVALID_SOCKET;
    }
    if (s_peer != INVALID_SOCKET)
    {
        closesocket(s_peer);
        s_peer = INVALID_SOCKET;
    }
    s_state = NET_CLOSED;
    s_rxlen = 0;
}

int net_state(void)
{
    return s_state;
}

const char *net_local_ip(void)
{
    return s_localIp;
}

int net_send(const char *line)
{
    int n;
    int total;
    int sent;
    char buf[256];

    if (s_peer == INVALID_SOCKET || line == NULL) return -1;

    n = (int)strlen(line);
    if (n <= 0 || n > 200) return -1;

    snprintf(buf, sizeof(buf), "%s\n", line);
    total = (int)strlen(buf);
    sent  = send(s_peer, buf, total, 0);

    return (sent == total) ? 0 : -1;
}

int net_send_move(int row, int col)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "MOVE %d %d", row, col);
    return net_send(buf);
}

static int try_accept(void)
{
    struct sockaddr_in cliAddr;
    int cliLen = sizeof(cliAddr);
    SOCKET s;

    if (s_listen == INVALID_SOCKET) return 0;

    s = accept(s_listen, (struct sockaddr *)&cliAddr, &cliLen);
    if (s == INVALID_SOCKET) return 0;
    set_nonblock(s);
    s_peer  = s;
    s_state = NET_CONNECTED;
    return 1;
}

static int try_finish_connect(void)
{
    fd_set wfds;
    struct timeval tv;

    if (s_peer == INVALID_SOCKET) return 0;

    FD_ZERO(&wfds);
    FD_SET(s_peer, &wfds);
    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    if (select(0, NULL, &wfds, NULL, &tv) <= 0) return 0;
    s_state = NET_CONNECTED;
    return 1;
}

static int extract_line(char *line, int lineSize)
{
    int i;

    for (i = 0; i < s_rxlen; i++)
    {
        if (s_rxbuf[i] == '\n')
        {
            int len = (i < lineSize - 1) ? i : lineSize - 1;
            memcpy(line, s_rxbuf, (size_t)len);
            line[len] = '\0';
            if (len > 0 && line[len - 1] == '\r') line[len - 1] = '\0';
            memmove(s_rxbuf, s_rxbuf + i + 1, (size_t)(s_rxlen - i - 1));
            s_rxlen -= i + 1;
            return 1;
        }
    }
    return 0;
}

static void parse_message(const char *line, int *type, int *row, int *col)
{
    int r;
    int c;

    if (sscanf(line, "MOVE %d %d", &r, &c) == 2)
    {
        *type = NET_MSG_MOVE;
        *row  = r;
        *col  = c;
    }
    else if (strcmp(line, "UNDO") == 0)      *type = NET_MSG_UNDO;
    else if (strcmp(line, "RESIGN") == 0)    *type = NET_MSG_RESIGN;
    else if (strcmp(line, "RESTART") == 0)   *type = NET_MSG_RESTART;
    else                                      *type = NET_MSG_NONE;
}

int net_poll(int *type, int *row, int *col)
{
    char line[256];
    int  ret;

    if (type != NULL) *type = NET_MSG_NONE;
    if (row  != NULL) *row  = -1;
    if (col  != NULL) *col  = -1;

    if (s_state == NET_LISTENING)
    {
        if (try_accept())
        {
            if (type != NULL) *type = NET_MSG_START;
            return 1;
        }
        return 0;
    }
    if (s_state == NET_CONNECTING)
    {
        if (try_finish_connect())
        {
            if (type != NULL) *type = NET_MSG_START;
            return 1;
        }
        return 0;
    }
    if (s_state != NET_CONNECTED) return 0;

    ret = recv(s_peer, s_rxbuf + s_rxlen, NET_RX_BUF - s_rxlen - 1, 0);
    if (ret > 0)
    {
        s_rxlen += ret;
        s_rxbuf[s_rxlen] = '\0';
    }
    else if (ret == 0)
    {
        if (type != NULL) *type = NET_MSG_CLOSE;
        s_state = NET_DISCONNECTED;
        return 1;
    }
    else
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINTR)
        {
            if (type != NULL) *type = NET_MSG_CLOSE;
            s_state = NET_DISCONNECTED;
            return 1;
        }
    }

    if (extract_line(line, sizeof(line)))
    {
        parse_message(line, type, row, col);
        if (*type != NET_MSG_NONE) return 1;
        return net_poll(type, row, col);
    }

    return 0;
}

const char *net_state_text(void)
{
    switch (s_state)
    {
    case NET_LISTENING:    return "服务器已就绪，等待对方连接";
    case NET_CONNECTING:   return "正在连接服务器";
    case NET_CONNECTED:    return "已连接";
    case NET_DISCONNECTED: return "连接已断开";
    default:               return "未连接";
    }
}
