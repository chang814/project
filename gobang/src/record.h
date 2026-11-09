/*==============================================================================
 * 文件名称：record.h
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：存档模块              负责人：chang814
 * 功能描述：声明对局记录的文件读写接口。
 *============================================================================*/
#ifndef GOBANG_RECORD_H
#define GOBANG_RECORD_H

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_OK           0
#define RECORD_ERR_NULL   (-1)
#define RECORD_ERR_OPEN   (-2)
#define RECORD_ERR_FORMAT (-3)
#define RECORD_ERR_DATA   (-4)
#define RECORD_ERR_WRITE  (-5)

#define RECORD_MAGIC   "GOBANG-RECORD"
#define RECORD_VERSION 1

#define RECORD_DIR      "saves"
#define RECORD_PATH_MAX 260
#define RECORD_MAX_LIST 64

typedef struct
{
    char     path[RECORD_PATH_MAX];
    int      mode;
    int      level;
    int      humanColor;
    int      status;
    int      moveCount;
    char     stamp[16];
} SaveEntry;

int record_save(const Board *board, const GameInfo *info, const char *path);
int record_load(Board *board, GameInfo *info, const char *path);
int record_export(const Board *board, const GameInfo *info, const char *path);
int record_ensure_dir(const char *dir);
int record_default_path(char *buf, int size, const char *dir, const char *suffix);
int record_coord_to_text(int row, int col, char *buf, int size);
int record_find_latest(const char *dir, char *buf, int size);
int record_list_saves(const char *dir, SaveEntry *entries, int maxEntries);
int record_peek(const char *path, SaveEntry *entry);
const char *record_error(int code);

#ifdef __cplusplus
}
#endif

#endif /* GOBANG_RECORD_H */
