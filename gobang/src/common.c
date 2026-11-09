/*==============================================================================
 * 文件名称：common.c
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：公共定义（全体成员共用）
 * 功能描述：实现 common.h 中声明的统一文本函数，供各模块调用，
 *           避免在 main.cpp / record.c / ai.c 中重复定义。
 * 编码说明：源文件以 UTF-8 保存，编译时用 -fexec-charset=GBK 转为 GBK。
 * 创建日期：2026-08-26
 *============================================================================*/
#include "common.h"

const char *common_mode_text(int mode)
{
    if (mode == MODE_PVE) return "人机对战";
    if (mode == MODE_NET) return "联网对战";
    return "双人对战";
}

const char *common_level_text(int level)
{
    switch (level)
    {
    case LEVEL_BEGINNER:    return "入门";
    case LEVEL_EASY:        return "简单";
    case LEVEL_HARD:        return "困难";
    case LEVEL_MASTER:      return "大师";
    case LEVEL_GRANDMASTER: return "棋圣";
    case LEVEL_NORMAL:      return "普通";
    default:                return "未知";
    }
}

const char *common_status_text(int status)
{
    switch (status)
    {
    case GAME_PLAYING:   return "对局进行中";
    case GAME_BLACK_WIN: return "黑方获胜";
    case GAME_WHITE_WIN: return "白方获胜";
    case GAME_DRAW:      return "和棋";
    default:             return "状态未知";
    }
}

const char *common_style_text(int style)
{
    switch (style)
    {
    case STYLE_NORMAL:     return "普通型";
    case STYLE_AGGRESSIVE: return "激进型";
    case STYLE_ATTACK:     return "攻杀型";
    case STYLE_DOUBLE:     return "双杀型";
    case STYLE_WALL:       return "铁壁型";
    case STYLE_COUNTER:    return "反击型";
    case STYLE_TRICK:      return "技巧型";
    case STYLE_CONTROL:    return "控场型";
    case STYLE_BAIT:       return "诱导型";
    default:               return "未知";
    }
}

const char *common_color_text(int color)
{
    if (color == CELL_BLACK) return "黑棋";
    if (color == CELL_WHITE) return "白棋";
    return "空";
}
