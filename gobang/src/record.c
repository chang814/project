/*==============================================================================
 * 文件名称：record.c
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：存档模块              负责人：chang814
 * 功能描述：实现对局记录的文本文件读写、存档浏览与导出中文棋谱。
 *============================================================================*/
#include "record.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <direct.h>
#include <io.h>

#define LINE_MAX_LEN 256

static int read_key_int(FILE *fp, const char *key, int *out)
{
    char line[LINE_MAX_LEN];
    char name[32];
    int  value;

    if (fgets(line, (int)sizeof(line), fp) == NULL)
    {
        return RECORD_ERR_FORMAT;
    }
    if (sscanf(line, "%31s %d", name, &value) != 2)
    {
        return RECORD_ERR_FORMAT;
    }
    if (strcmp(name, key) != 0)
    {
        return RECORD_ERR_FORMAT;
    }

    *out = value;
    return RECORD_OK;
}

int record_ensure_dir(const char *dir)
{
    if (dir == NULL || dir[0] == '\0')
    {
        return RECORD_ERR_NULL;
    }

    if (_mkdir(dir) == 0)
    {
        return RECORD_OK;
    }
    if (errno == EEXIST)
    {
        return RECORD_OK;
    }
    return RECORD_ERR_OPEN;
}

int record_default_path(char *buf, int size, const char *dir, const char *suffix)
{
    time_t     now;
    struct tm *lt;
    char       stamp[32];

    if (buf == NULL || size <= 0)
    {
        return RECORD_ERR_NULL;
    }
    if (dir == NULL)
    {
        dir = RECORD_DIR;
    }

    record_ensure_dir(dir);

    now = time(NULL);
    lt  = localtime(&now);
    if (lt == NULL)
    {
        strcpy(stamp, "00000000_000000");
    }
    else
    {
        strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", lt);
    }

    if (suffix == NULL || suffix[0] == '\0')
    {
        snprintf(buf, (size_t)size, "%s/game_%s.txt", dir, stamp);
    }
    else
    {
        snprintf(buf, (size_t)size, "%s/game_%s_%s.txt", dir, stamp, suffix);
    }

    return RECORD_OK;
}

int record_coord_to_text(int row, int col, char *buf, int size)
{
    if (buf == NULL || size < 4)
    {
        return RECORD_ERR_NULL;
    }
    if (!board_in_range(row, col))
    {
        buf[0] = '\0';
        return RECORD_ERR_DATA;
    }

    snprintf(buf, (size_t)size, "%c%d", (char)('A' + col), row + 1);
    return RECORD_OK;
}

int record_find_latest(const char *dir, char *buf, int size)
{
    struct _finddata_t entry;
    intptr_t           handle;
    char               pattern[RECORD_PATH_MAX];
    char               best[RECORD_PATH_MAX];
    int                found = 0;

    if (buf == NULL || size <= 0)
    {
        return RECORD_ERR_NULL;
    }
    if (dir == NULL)
    {
        dir = RECORD_DIR;
    }

    buf[0]  = '\0';
    best[0] = '\0';

    snprintf(pattern, sizeof(pattern), "%s/game_*.txt", dir);

    handle = _findfirst(pattern, &entry);
    if (handle == -1)
    {
        return RECORD_ERR_OPEN;
    }

    do
    {
        if (strstr(entry.name, "qipu") != NULL)
        {
            continue;
        }
        if (!found || strcmp(entry.name, best) > 0)
        {
            strncpy(best, entry.name, sizeof(best) - 1);
            best[sizeof(best) - 1] = '\0';
            found = 1;
        }
    } while (_findnext(handle, &entry) == 0);

    _findclose(handle);

    if (!found)
    {
        return RECORD_ERR_OPEN;
    }

    snprintf(buf, (size_t)size, "%s/%s", dir, best);
    return RECORD_OK;
}

static void extract_stamp(const char *filename, char *stamp, int size)
{
    const char *p;

    stamp[0] = '\0';
    if (filename == NULL)
    {
        return;
    }
    p = strstr(filename, "game_");
    if (p == NULL)
    {
        return;
    }
    p += 5;
    strncpy(stamp, p, (size_t)(size - 1));
    stamp[size - 1] = '\0';
    if (strlen(stamp) > 15)
    {
        stamp[15] = '\0';
    }
}

int record_peek(const char *path, SaveEntry *entry)
{
    FILE *fp;
    int   version;
    char  magic[32];
    char  line[LINE_MAX_LEN];
    int   ret;

    if (path == NULL || entry == NULL)
    {
        return RECORD_ERR_NULL;
    }

    memset(entry, 0, sizeof(*entry));

    fp = fopen(path, "r");
    if (fp == NULL)
    {
        return RECORD_ERR_OPEN;
    }

    if (fgets(line, (int)sizeof(line), fp) == NULL ||
        sscanf(line, "%31s %d", magic, &version) != 2 ||
        strcmp(magic, RECORD_MAGIC) != 0 ||
        version != RECORD_VERSION)
    {
        fclose(fp);
        return RECORD_ERR_FORMAT;
    }

    ret = read_key_int(fp, "MODE", &entry->mode);
    if (ret != RECORD_OK) { fclose(fp); return ret; }
    ret = read_key_int(fp, "LEVEL", &entry->level);
    if (ret != RECORD_OK) { fclose(fp); return ret; }
    ret = read_key_int(fp, "HUMAN", &entry->humanColor);
    if (ret != RECORD_OK) { fclose(fp); return ret; }
    /* 跳过 STYLE 字段（保存时写入，列表显示不需要） */
    {
        int style;
        ret = read_key_int(fp, "STYLE", &style);
        if (ret != RECORD_OK) { fclose(fp); return ret; }
    }
    ret = read_key_int(fp, "STATUS", &entry->status);
    if (ret != RECORD_OK) { fclose(fp); return ret; }
    ret = read_key_int(fp, "COUNT", &entry->moveCount);
    if (ret != RECORD_OK) { fclose(fp); return ret; }

    fclose(fp);

    snprintf(entry->path, sizeof(entry->path), "%s", path);
    return RECORD_OK;
}

int record_list_saves(const char *dir, SaveEntry *entries, int maxEntries)
{
    struct _finddata_t fd;
    intptr_t           handle;
    char               pattern[RECORD_PATH_MAX];
    int                count = 0;
    int                i;
    int                j;

    if (entries == NULL || maxEntries <= 0)
    {
        return 0;
    }
    if (dir == NULL)
    {
        dir = RECORD_DIR;
    }

    snprintf(pattern, sizeof(pattern), "%s/game_*.txt", dir);

    handle = _findfirst(pattern, &fd);
    if (handle == -1)
    {
        return 0;
    }

    do
    {
        SaveEntry e;
        char      path[RECORD_PATH_MAX];

        if (strstr(fd.name, "qipu") != NULL)
        {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", dir, fd.name);
        if (record_peek(path, &e) != RECORD_OK)
        {
            continue;
        }
        extract_stamp(fd.name, e.stamp, (int)sizeof(e.stamp));

        if (count < maxEntries)
        {
            entries[count++] = e;
        }
    } while (_findnext(handle, &fd) == 0);

    _findclose(handle);

    for (i = 1; i < count; i++)
    {
        SaveEntry key = entries[i];
        j = i - 1;
        while (j >= 0 && strcmp(entries[j].stamp, key.stamp) < 0)
        {
            entries[j + 1] = entries[j];
            j--;
        }
        entries[j + 1] = key;
    }

    return count;
}

int record_save(const Board *board, const GameInfo *info, const char *path)
{
    FILE *fp;
    int   i;
    int   failed;

    if (board == NULL || info == NULL || path == NULL || path[0] == '\0')
    {
        return RECORD_ERR_NULL;
    }

    fp = fopen(path, "w");
    if (fp == NULL)
    {
        return RECORD_ERR_OPEN;
    }

    fprintf(fp, "%s %d\n", RECORD_MAGIC, RECORD_VERSION);
    fprintf(fp, "MODE %d\n",   info->mode);
    fprintf(fp, "LEVEL %d\n",  info->level);
    fprintf(fp, "HUMAN %d\n",  info->humanColor);
    /* style 防御性钳制:未初始化/非法值一律按普通型写入 */
    fprintf(fp, "STYLE %d\n",  (info->style >= STYLE_NORMAL && info->style < STYLE_COUNT)
                               ? info->style : STYLE_NORMAL);
    fprintf(fp, "STATUS %d\n", board->status);
    fprintf(fp, "COUNT %d\n",  board->moveCount);

    for (i = 0; i < board->moveCount; i++)
    {
        fprintf(fp, "%d %d %d\n",
                board->moves[i].row,
                board->moves[i].col,
                board->moves[i].player);
    }

    fprintf(fp, "END\n");

    failed = ferror(fp);
    if (fclose(fp) != 0)
    {
        failed = 1;
    }

    return failed ? RECORD_ERR_WRITE : RECORD_OK;
}

int record_load(Board *board, GameInfo *info, const char *path)
{
    FILE *fp;
    char  line[LINE_MAX_LEN];
    char  magic[32];
    int   version;
    int   mode;
    int   level;
    int   human;
    int   style;
    int   status;
    int   count;
    int   i;
    int   row;
    int   col;
    int   player;
    int   ret;

    if (board == NULL || info == NULL || path == NULL || path[0] == '\0')
    {
        return RECORD_ERR_NULL;
    }

    fp = fopen(path, "r");
    if (fp == NULL)
    {
        return RECORD_ERR_OPEN;
    }

    if (fgets(line, (int)sizeof(line), fp) == NULL ||
        sscanf(line, "%31s %d", magic, &version) != 2 ||
        strcmp(magic, RECORD_MAGIC) != 0 ||
        version != RECORD_VERSION)
    {
        fclose(fp);
        return RECORD_ERR_FORMAT;
    }

    ret = read_key_int(fp, "MODE", &mode);
    if (ret != RECORD_OK) { fclose(fp); return ret; }
    ret = read_key_int(fp, "LEVEL", &level);
    if (ret != RECORD_OK) { fclose(fp); return ret; }
    ret = read_key_int(fp, "HUMAN", &human);
    if (ret != RECORD_OK) { fclose(fp); return ret; }
    /* STYLE 为可选键:旧存档无此行,默认普通型(向后兼容) */
    style = STYLE_NORMAL;
    {
        long mark = ftell(fp);
        if (fgets(line, (int)sizeof(line), fp) != NULL)
        {
            char key[32];
            int  value;
            if (sscanf(line, "%31s %d", key, &value) == 2 && strcmp(key, "STYLE") == 0)
            {
                style = value;
            }
            else
            {
                fseek(fp, mark, SEEK_SET);   /* 非 STYLE 行:回退继续读 STATUS */
            }
        }
    }
    ret = read_key_int(fp, "STATUS", &status);
    if (ret != RECORD_OK) { fclose(fp); return ret; }
    ret = read_key_int(fp, "COUNT", &count);
    if (ret != RECORD_OK) { fclose(fp); return ret; }

    if (mode != MODE_PVP && mode != MODE_PVE && mode != MODE_NET)
    {
        fclose(fp);
        return RECORD_ERR_DATA;
    }
    if (level < LEVEL_BEGINNER || level >= LEVEL_COUNT)
    {
        fclose(fp);
        return RECORD_ERR_DATA;
    }
    if (style < STYLE_NORMAL || style >= STYLE_COUNT)
    {
        fclose(fp);
        return RECORD_ERR_DATA;
    }
    if (human != CELL_BLACK && human != CELL_WHITE)
    {
        fclose(fp);
        return RECORD_ERR_DATA;
    }
    if (status < GAME_PLAYING || status > GAME_DRAW)
    {
        fclose(fp);
        return RECORD_ERR_DATA;
    }
    if (count < 0 || count > MAX_MOVES)
    {
        fclose(fp);
        return RECORD_ERR_DATA;
    }

    board_init(board);

    for (i = 0; i < count; i++)
    {
        if (fgets(line, (int)sizeof(line), fp) == NULL)
        {
            fclose(fp);
            return RECORD_ERR_FORMAT;
        }
        if (sscanf(line, "%d %d %d", &row, &col, &player) != 3)
        {
            fclose(fp);
            return RECORD_ERR_FORMAT;
        }
        if (player != board->current)
        {
            fclose(fp);
            return RECORD_ERR_DATA;
        }
        if (board_place(board, row, col) != PLACE_OK)
        {
            fclose(fp);
            return RECORD_ERR_DATA;
        }
    }

    if (fgets(line, (int)sizeof(line), fp) == NULL ||
        strncmp(line, "END", 3) != 0)
    {
        fclose(fp);
        return RECORD_ERR_FORMAT;
    }

    fclose(fp);

    if (board->status != GAME_PLAYING && board->status != status)
    {
        return RECORD_ERR_DATA;
    }
    board_set_status(board, status);

    info->mode       = mode;
    info->level      = level;
    info->humanColor = human;
    info->style      = style;

    return RECORD_OK;
}

int record_export(const Board *board, const GameInfo *info, const char *path)
{
    FILE *fp;
    char  coord[8];
    int   i;
    int   failed;

    if (board == NULL || info == NULL || path == NULL || path[0] == '\0')
    {
        return RECORD_ERR_NULL;
    }

    fp = fopen(path, "w");
    if (fp == NULL)
    {
        return RECORD_ERR_OPEN;
    }

    fprintf(fp, "================================================\n");
    fprintf(fp, "            五子棋对局记录\n");
    fprintf(fp, "================================================\n");
    fprintf(fp, "对局模式：%s\n", common_mode_text(info->mode));
    if (info->mode == MODE_PVE)
    {
        fprintf(fp, "难度等级：%s\n", common_level_text(info->level));
        fprintf(fp, "AI 风格：%s\n", common_style_text(info->style));
        fprintf(fp, "玩家执子：%s\n", common_color_text(info->humanColor));
    }
    fprintf(fp, "对局结果：%s\n", common_status_text(board->status));
    fprintf(fp, "总 手 数：%d\n", board->moveCount);
    fprintf(fp, "坐标说明：列 A~O 自左向右，行 1~15 自上而下\n");
    fprintf(fp, "------------------------------------------------\n");

    for (i = 0; i < board->moveCount; i++)
    {
        record_coord_to_text(board->moves[i].row, board->moves[i].col,
                             coord, (int)sizeof(coord));
        fprintf(fp, "%3d. %s %-4s", i + 1,
                common_color_text(board->moves[i].player), coord);
        if ((i + 1) % 4 == 0)
        {
            fprintf(fp, "\n");
        }
    }
    if (board->moveCount % 4 != 0)
    {
        fprintf(fp, "\n");
    }

    fprintf(fp, "================================================\n");

    failed = ferror(fp);
    if (fclose(fp) != 0)
    {
        failed = 1;
    }

    return failed ? RECORD_ERR_WRITE : RECORD_OK;
}

const char *record_error(int code)
{
    switch (code)
    {
    case RECORD_OK:
        return "存档操作成功";
    case RECORD_ERR_NULL:
        return "参数无效，操作已取消";
    case RECORD_ERR_OPEN:
        return "文件打不开，请检查路径是否正确";
    case RECORD_ERR_FORMAT:
        return "文件格式不正确，不是本程序的存档";
    case RECORD_ERR_DATA:
        return "存档内容非法，棋谱中存在违规走法";
    case RECORD_ERR_WRITE:
        return "写入文件失败，请检查磁盘空间";
    default:
        return "未知错误";
    }
}
