/*==============================================================================
 * 文件名称：main.cpp
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：主控模块
 * 功能描述：程序入口与游戏主循环。
 *============================================================================*/
#include <graphics.h>

#include "board.h"
#include "record.h"
#include "ai.h"
#include "ui.h"
#include "input.h"
#include "net.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#define STATE_MENU  0
#define STATE_GAME  1
#define STATE_LOAD  2
#define STATE_NET   3
#define STATE_EXIT  4
#define STATE_NET_JOIN 5
#define STATE_REPLAY 6

static Board    g_board;
static GameInfo g_info;
static char     g_message[160];
static int      g_state    = STATE_MENU;
static int      g_hoverId  = BTN_NONE;
static int      g_hoverRow = -1;
static int      g_hoverCol = -1;
static int      g_prevState = STATE_MENU;
static char     g_joinIp[64] = "127.0.0.1";

/* 难度详情弹窗 */
static int g_showDiffDialog = 0;
static int g_pendingLevel   = LEVEL_NORMAL;
static int g_pendingStyle   = STYLE_NORMAL;

/* 算法百科页（产品介绍 / AI 发展历程 + 代码逐行讲解） */
static int g_showAlgoBook = 0;
static int g_algoPage     = 0;

/* 棋谱回放 */
static Move     g_replayMoves[MAX_MOVES];
static int      g_replayCount = 0;
static int      g_replayIndex = 0;
static GameInfo g_replayInfo;

/* 回放阵型高亮：仅在阵型新成型的那一步高亮棋子 */
static int g_replayHighlightRows[32];
static int g_replayHighlightCols[32];
static int g_replayHighlightCount = 0;

static SaveEntry g_saves[RECORD_MAX_LIST];
static int       g_saveCount = 0;
static int       g_saveScroll = 0;
static char      g_rowText[RECORD_MAX_LIST][80];
static char      g_subText[RECORD_MAX_LIST][80];

static void set_message(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(g_message, sizeof(g_message), fmt, ap);
	va_end(ap);
}

/* 阵型检测播报:每次落子/悔棋/读档后调用。
 * 去重原则:仅当某方阵法「相比上一手是新出现/发生变化」时才推送一条,
 * 避免同一阵法在黑白双方重复播报、也避免成型后每步反复刷屏。 */
static FormationType s_prevBlack = FORMATION_NONE;
static FormationType s_prevWhite = FORMATION_NONE;

/* 对局阵型高亮：阵型新成型时高亮棋子，下一步后自动消失 */
static int g_gameHighlightRows[32];
static int g_gameHighlightCols[32];
static int g_gameHighlightCount = 0;

static void update_formation_report(void) {
	FormationType blackF;
	FormationType whiteF;
	char buf[128];

	if (g_state != STATE_GAME)
		return;   /* 仅在对局中播报 */

	blackF = ai_detect_formation(&g_board, CELL_BLACK);
	whiteF = ai_detect_formation(&g_board, CELL_WHITE);

	/* 每步先清除上一步的高亮和事件（阵型高亮/介绍只在成型那一步显示） */
	g_gameHighlightCount = 0;
	ui_set_formation_event(NULL);

	if (blackF != s_prevBlack && blackF != FORMATION_NONE) {
		ai_formation_event_text(&g_board, CELL_BLACK, buf, (int)sizeof(buf));
		if (buf[0] != '\0') {
			ui_set_formation_report(buf);
			ui_set_formation_event(buf);
		}
		g_gameHighlightCount = ai_formation_cells(&g_board, CELL_BLACK,
			g_gameHighlightRows, g_gameHighlightCols, 32);
	}
	if (whiteF != s_prevWhite && whiteF != FORMATION_NONE) {
		ai_formation_event_text(&g_board, CELL_WHITE, buf, (int)sizeof(buf));
		if (buf[0] != '\0') {
			ui_set_formation_report(buf);
			ui_set_formation_event(buf);
		}
		if (g_gameHighlightCount == 0)
			g_gameHighlightCount = ai_formation_cells(&g_board, CELL_WHITE,
				g_gameHighlightRows, g_gameHighlightCols, 32);
	}

	s_prevBlack = blackF;
	s_prevWhite = whiteF;
}

/* 阵法播报历史重置(新开局/读档前调用) */
static void clear_formation_report(void) {
	ui_set_formation_report(NULL);
	ui_set_formation_event(NULL);
	g_gameHighlightCount = 0;
	s_prevBlack = FORMATION_NONE;
	s_prevWhite = FORMATION_NONE;
}

static int ai_color(void) {
	return OPPONENT(g_info.humanColor);
}

static int is_ai_turn(void) {
	return (g_info.mode == MODE_PVE &&
	        g_board.status == GAME_PLAYING &&
	        g_board.current != g_info.humanColor);
}

static int is_local_turn(void) {
	if (g_board.status != GAME_PLAYING)
		return 0;
	if (g_info.mode == MODE_NET)
		return (g_board.current == g_info.humanColor);
	return 1;
}

static int announce_result(void) {
	switch (g_board.status) {
		case GAME_BLACK_WIN:
			set_message("五子连珠，黑方获胜！");
			return 1;
		case GAME_WHITE_WIN:
			set_message("五子连珠，白方获胜！");
			return 1;
		case GAME_DRAW:
			set_message("棋盘已下满，本局为和棋。");
			return 1;
		default:
			return 0;
	}
}

static void start_new_game(int mode) {
	g_info.mode = mode;
	board_init(&g_board);

	g_hoverId  = BTN_NONE;
	g_hoverRow = -1;
	g_hoverCol = -1;
	g_state    = STATE_GAME;

	/* 新开局:清空上一局的阵法播报 */
	clear_formation_report();

	if (mode == MODE_PVE) {
		set_message("人机对战开始，难度%s，玩家执%s。",
		            ai_level_text(g_info.level),
		            (g_info.humanColor == CELL_BLACK) ? "黑先行" : "白后行");
	} else if (mode == MODE_NET) {
		set_message("联网对战开始，本机执%s。",
		            (g_info.humanColor == CELL_BLACK) ? "黑先行" : "白后行");
	} else {
		set_message("双人对战开始，黑方先行。");
	}

	input_flush();
}

static void do_place(int row, int col) {
	char coord[8];
	int  ret;

	ret = board_place(&g_board, row, col);
	if (ret != PLACE_OK) {
		set_message("%s", board_place_error(ret));
		return;
	}

	record_coord_to_text(row, col, coord, (int)sizeof(coord));
	if (!announce_result()) {
		set_message("落子于 %s。", coord);
	}

	if (g_info.mode == MODE_NET) {
		net_send_move(row, col);
	}

	update_formation_report();
}

static void do_undo(void) {
	int steps;

	if (g_board.moveCount == 0) {
		set_message("当前还没有落子，无法悔棋。");
		return;
	}

	if (g_info.mode == MODE_NET) {
		if (g_board.moveCount < 2) {
			set_message("至少落满两手才能悔棋。");
			return;
		}
		steps = board_undo_steps(&g_board, 2);
		net_send("UNDO");
		set_message("已悔棋 %d 步，当前共 %d 手。", steps, g_board.moveCount);
		update_formation_report();
		return;
	}

	steps = (g_info.mode == MODE_PVE) ? 2 : 1;
	steps = board_undo_steps(&g_board, steps);

	if (g_info.mode == MODE_PVE &&
	        g_board.moveCount > 0 &&
	        g_board.current != g_info.humanColor) {
		steps += board_undo(&g_board);
	}

	set_message("已悔棋 %d 步，当前共 %d 手。", steps, g_board.moveCount);
	update_formation_report();
}

static void do_resign(void) {
	int loser;

	if (g_board.status != GAME_PLAYING) {
		set_message("本局已经结束了。");
		return;
	}

	loser = (g_info.mode == MODE_PVE) ? g_info.humanColor : g_board.current;
	if (g_info.mode == MODE_NET) {
		loser = g_info.humanColor;
	}
	board_resign(&g_board, loser);

	set_message("%s认输，%s获胜。",
	            (loser == CELL_BLACK) ? "黑方" : "白方",
	            (loser == CELL_BLACK) ? "白方" : "黑方");

	if (g_info.mode == MODE_NET) {
		net_send("RESIGN");
	}
}

static void do_save(void) {
	char path[RECORD_PATH_MAX];
	char readable[RECORD_PATH_MAX];
	int  ret;

	if (g_board.moveCount == 0) {
		set_message("当前还没有落子，无需保存。");
		return;
	}

	record_default_path(path, (int)sizeof(path), RECORD_DIR, NULL);
	ret = record_save(&g_board, &g_info, path);
	if (ret != RECORD_OK) {
		set_message("保存失败：%s", record_error(ret));
		return;
	}

	record_default_path(readable, (int)sizeof(readable), RECORD_DIR, "qipu");
	ret = record_export(&g_board, &g_info, readable);
	if (ret != RECORD_OK) {
		set_message("存档已保存到 %s；棋谱导出失败：%s", path, record_error(ret));
		return;
	}

	set_message("已保存：%s（另附可读棋谱 %s）", path, readable);
}

static void open_browser(void) {
	g_prevState  = g_state;
	g_saveCount  = record_list_saves(RECORD_DIR, g_saves, RECORD_MAX_LIST);
	g_saveScroll = 0;
	g_hoverId    = BTN_NONE;
	g_state      = STATE_LOAD;
	input_flush();
}

static int g_replayFinalStatus;
static int g_replayHasWinLine;
static int g_replayWinLine[WIN_COUNT][2];

static void replay_build(int index) {
	int i;
	Board prevBoard;

	if (index < 0)
		index = 0;
	if (index > g_replayCount)
		index = g_replayCount;

	/* 先构建上一步局面（用于检测阵型是否在这一步新成型） */
	board_init(&prevBoard);
	for (i = 0; i < index - 1; i++) {
		board_place(&prevBoard, g_replayMoves[i].row, g_replayMoves[i].col);
	}

	/* 构建当前局面 */
	board_init(&g_board);
	for (i = 0; i < index; i++) {
		board_place(&g_board, g_replayMoves[i].row, g_replayMoves[i].col);
	}

	/* 最后一步可能是认输等特殊结束，恢复最终状态 */
	if (index == g_replayCount) {
		board_set_status(&g_board, g_replayFinalStatus);
		if (g_replayHasWinLine) {
			g_board.hasWinLine = 1;
			for (i = 0; i < WIN_COUNT; i++) {
				g_board.winLine[i][0] = g_replayWinLine[i][0];
				g_board.winLine[i][1] = g_replayWinLine[i][1];
			}
		}
	}

	/* 阵型高亮检测：仅当这一步新成型阵法时高亮，下一步后自动消失 */
	g_replayHighlightCount = 0;
	if (index > 0) {
		FormationType curBlack  = ai_detect_formation(&g_board, CELL_BLACK);
		FormationType prevBlack = ai_detect_formation(&prevBoard, CELL_BLACK);
		if (curBlack != FORMATION_NONE && prevBlack == FORMATION_NONE) {
			g_replayHighlightCount = ai_formation_cells(&g_board, CELL_BLACK,
				g_replayHighlightRows, g_replayHighlightCols, 32);
		}
		if (g_replayHighlightCount == 0) {
			FormationType curWhite  = ai_detect_formation(&g_board, CELL_WHITE);
			FormationType prevWhite = ai_detect_formation(&prevBoard, CELL_WHITE);
			if (curWhite != FORMATION_NONE && prevWhite == FORMATION_NONE) {
				g_replayHighlightCount = ai_formation_cells(&g_board, CELL_WHITE,
					g_replayHighlightRows, g_replayHighlightCols, 32);
			}
		}
	}

	g_replayIndex = index;
}

static void load_save_path(const char *path) {
	Board    loadedBoard;
	GameInfo loadedInfo;
	int      ret;
	int      i;

	ret = record_load(&loadedBoard, &loadedInfo, path);
	if (ret != RECORD_OK) {
		set_message("读取失败：%s", record_error(ret));
		return;
	}

	/* 保存走法记录和最终状态用于回放 */
	g_replayCount = loadedBoard.moveCount;
	for (i = 0; i < g_replayCount; i++) {
		g_replayMoves[i] = loadedBoard.moves[i];
	}
	g_replayInfo       = loadedInfo;
	g_replayFinalStatus = loadedBoard.status;
	g_replayHasWinLine  = loadedBoard.hasWinLine;
	for (i = 0; i < WIN_COUNT; i++) {
		g_replayWinLine[i][0] = loadedBoard.winLine[i][0];
		g_replayWinLine[i][1] = loadedBoard.winLine[i][1];
	}

	g_info    = loadedInfo;
	g_state   = STATE_REPLAY;
	g_hoverId = BTN_NONE;

	/* 默认显示最终局面 */
	replay_build(g_replayCount);

	set_message("已载入棋谱，共 %d 手，可逐步回放。", g_replayCount);
	input_flush();
}

static void load_frame(void) {
	const int rowTop   = 128;      /* 与 ui_draw_browser 保持一致 */
	const int rowStep  = 58;
	const int maxRows  = 9;
	UiButton  buttons[RECORD_MAX_LIST + 1];
	UiRow     rows[RECORD_MAX_LIST];
	int       buttonCount = 0;
	int       rowCount = 0;
	int       i;
	int       maxScroll;
	UiEvent   ev;

	buttons[buttonCount].x    = 470;
	buttons[buttonCount].y    = 730;
	buttons[buttonCount].w    = 240;
	buttons[buttonCount].h    = 40;
	buttons[buttonCount].id   = BTN_LOAD_BACK;
	buttons[buttonCount].text = "返  回";
	buttonCount++;

	maxScroll = (g_saveCount > maxRows) ? (g_saveCount - maxRows) : 0;
	if (g_saveScroll > maxScroll)
		g_saveScroll = maxScroll;
	if (g_saveScroll < 0)
		g_saveScroll = 0;

	for (i = 0; i < maxRows && g_saveScroll + i < g_saveCount; i++) {
		const SaveEntry *e = &g_saves[g_saveScroll + i];
		int   id = BTN_ROW_BASE + i;
		char  date[16];
		char  clockTime[8];

		snprintf(date, sizeof(date), "%.4s年%.2s月%.2s日", e->stamp, e->stamp + 4, e->stamp + 6);
		snprintf(clockTime, sizeof(clockTime), "%.2s:%.2s", e->stamp + 9, e->stamp + 11);

		snprintf(g_rowText[rowCount], sizeof(g_rowText[rowCount]),
		         "%s %s   %s   共 %d 手", date, clockTime, common_status_text(e->status), e->moveCount);
		rows[rowCount].text = g_rowText[rowCount];

		snprintf(g_subText[rowCount], sizeof(g_subText[rowCount]),
		         "%s · %s难度", common_mode_text(e->mode), common_level_text(e->level));
		rows[rowCount].subtext = g_subText[rowCount];

		rows[rowCount].y  = rowTop + i * rowStep;
		rows[rowCount].id = id;

		buttons[buttonCount].x    = 80;
		buttons[buttonCount].y    = rows[rowCount].y;
		buttons[buttonCount].w    = UI_WIDTH - 160;
		buttons[buttonCount].h    = 52;
		buttons[buttonCount].id   = id;
		buttons[buttonCount].text = "";
		buttonCount++;

		rowCount++;
	}

	if (g_saveCount == 0) {
		set_message("暂无存档，请先在对局中点击保存棋谱按钮。");
	}

	while (input_poll(buttons, buttonCount, 0, &ev)) {
		g_hoverId = ev.buttonId;

		if (ev.type == EVENT_BUTTON) {
			if (ev.buttonId == BTN_LOAD_BACK) {
				g_state   = g_prevState;
				g_hoverId = BTN_NONE;
				set_message("");
				return;
			}
			if (ev.buttonId >= BTN_ROW_BASE && ev.buttonId < BTN_ROW_BASE + maxRows) {
				int idx = g_saveScroll + (ev.buttonId - BTN_ROW_BASE);
				if (idx >= 0 && idx < g_saveCount) {
					load_save_path(g_saves[idx].path);
				}
				if (g_state == STATE_GAME)
					return;
			}
		}

		if (g_state != STATE_LOAD)
			return;
	}

	ui_draw_browser(rows, rowCount, g_hoverId, g_message);
	ui_flush();
}

static void run_ai_turn(void) {
	char coord[8];
	int  row = -1;
	int  col = -1;
	int  ret;

	ui_draw_game(&g_board, &g_info, g_message, BTN_NONE, -1, -1, 1,
	             g_gameHighlightRows, g_gameHighlightCols, g_gameHighlightCount);
	ui_flush();

	/* 应用当前风格(普通及以下难度固定普通型) */
	ai_set_style((g_info.level >= LEVEL_HARD) ? g_info.style : STYLE_NORMAL);
	ret = ai_choose(&g_board, ai_color(), g_info.level, &row, &col);
	if (ret != AI_OK) {
		set_message("电脑无处可下。");
		return;
	}

	ret = board_place(&g_board, row, col);
	if (ret != PLACE_OK) {
		set_message("电脑落子异常：%s", board_place_error(ret));
		return;
	}

	record_coord_to_text(row, col, coord, (int)sizeof(coord));
	if (!announce_result()) {
		set_message("电脑落子于 %s，用时 %d 毫秒，搜索深度 %d。",
		            coord, ai_last_time_ms(), ai_last_depth());
	}

	update_formation_report();
	input_flush();
}

static void start_net_host(void);
static void start_net_join(void);

static void handle_menu_button(int buttonId) {
	switch (buttonId) {
		case BTN_PVP:
			start_new_game(MODE_PVP);
			break;
		case BTN_PVE:
			start_new_game(MODE_PVE);
			break;
		case BTN_NET_HOST:
			start_net_host();
			break;
		case BTN_NET_JOIN:
			start_net_join();
			break;
		case BTN_LEVEL_BEGINNER:
			g_info.level = LEVEL_BEGINNER;
			break;
		case BTN_LEVEL_EASY:
			g_info.level = LEVEL_EASY;
			break;
		case BTN_LEVEL_NORMAL:
			g_info.level = LEVEL_NORMAL;
			break;
		case BTN_LEVEL_HARD:
			g_info.level = LEVEL_HARD;
			break;
		case BTN_LEVEL_MASTER:
			g_info.level = LEVEL_MASTER;
			break;
		case BTN_LEVEL_GRANDMASTER:
			g_info.level = LEVEL_GRANDMASTER;
			break;
		case BTN_COLOR_BLACK:
			g_info.humanColor = CELL_BLACK;
			break;
		case BTN_COLOR_WHITE:
			g_info.humanColor = CELL_WHITE;
			break;
		case BTN_MENU_LOAD:
			open_browser();
			break;
		case BTN_MENU_EXIT:
			g_state = STATE_EXIT;
			break;
		case BTN_THEME:
			ui_set_theme((ui_get_theme() + 1) % THEME_COUNT);
			break;
		case BTN_ALGO_BOOK:
			g_showAlgoBook = 1;
			g_algoPage     = 0;
			g_hoverId      = BTN_NONE;
			set_message("");
			break;
		default:
			break;
	}
}

static void handle_game_button(int buttonId) {
	switch (buttonId) {
		case BTN_UNDO:
			do_undo();
			break;
		case BTN_RESIGN:
			do_resign();
			break;
		case BTN_SAVE:
			do_save();
			break;
		case BTN_LOAD:
			open_browser();
			break;
		case BTN_RESTART:
			start_new_game(g_info.mode);
			if (g_info.mode == MODE_NET)
				net_send("RESTART");
			break;
		case BTN_BACK:
			if (g_info.mode == MODE_NET)
				net_close();
			g_state   = STATE_MENU;
			g_hoverId = BTN_NONE;
			set_message("");
			break;
		default:
			break;
	}
}

static void start_net_host(void) {
	if (net_host_start(NET_DEFAULT_PORT) != 0) {
		set_message("创建房间失败：端口可能被占用。");
		return;
	}
	g_info.mode       = MODE_NET;
	g_info.humanColor = CELL_BLACK;
	g_prevState       = STATE_MENU;
	g_hoverId         = BTN_NONE;
	g_state           = STATE_NET;
	set_message("等待对方连接（本机 IP：%s，端口 %d）……", net_local_ip(), NET_DEFAULT_PORT);
	input_flush();
}

static void start_net_join(void) {
	g_prevState = STATE_MENU;
	g_hoverId   = BTN_NONE;
	g_state     = STATE_NET_JOIN;
	set_message("");
	input_flush();
}

static int try_connect(void) {
	if (g_joinIp[0] == '\0') {
		set_message("请先输入 IP 地址。");
		return 0;
	}
	if (net_join_start(g_joinIp, NET_DEFAULT_PORT) != 0) {
		set_message("无法连接服务器 %s，请检查 IP 与网络。", g_joinIp);
		return 0;
	}
	g_info.mode       = MODE_NET;
	g_info.humanColor = CELL_WHITE;
	g_prevState       = STATE_MENU;
	g_hoverId         = BTN_NONE;
	g_state           = STATE_NET;
	set_message("正在连接服务器 %s ……", g_joinIp);
	input_flush();
	return 1;
}

static void net_join_frame(void) {
	static const UiButton buttons[] = {
		{ 390, 355, 180, 44, BTN_NET_CONNECT, "连  接" },
		{ 610, 355, 180, 44, BTN_JOIN_BACK,    "返  回" }
	};
	const int btnCount = 2;
	static int cursorBlink = 1;
	static int blinkCounter = 0;
	UiEvent ev;
	int len;

	/* 光标闪烁计时（约 60fps，30 帧切换一次） */
	blinkCounter++;
	if (blinkCounter >= 30) {
		blinkCounter = 0;
		cursorBlink = !cursorBlink;
	}

	/* 处理键盘输入 */
	while (kbmsg()) {
		key_msg k = getkey();

		/* 可打印字符：key_msg_char 的 key 已是 ASCII 码（含 Shift 转换） */
		if (k.msg == key_msg_char) {
			len = (int)strlen(g_joinIp);
			if ((k.key >= '0' && k.key <= '9') || k.key == '.') {
				if (len < (int)sizeof(g_joinIp) - 1) {
					g_joinIp[len] = (char)k.key;
					g_joinIp[len + 1] = '\0';
					set_message("");
				}
			}
		}
		/* 控制键：key_msg_down 的 key 是虚拟键码 */
		else if (k.msg == key_msg_down) {
			if (k.key == 8) { /* VK_BACK 退格 */
				len = (int)strlen(g_joinIp);
				if (len > 0) {
					g_joinIp[len - 1] = '\0';
					set_message("");
				}
			} else if (k.key == 13) { /* VK_RETURN 回车 */
				if (try_connect())
					return;
			} else if (k.key == 27) { /* VK_ESCAPE */
				g_state   = STATE_MENU;
				g_hoverId = BTN_NONE;
				set_message("");
				return;
			}
		}
	}

	/* 处理鼠标 */
	while (input_poll(buttons, btnCount, 0, &ev)) {
		g_hoverId = ev.buttonId;
		if (ev.type == EVENT_BUTTON) {
			if (ev.buttonId == BTN_NET_CONNECT) {
				if (try_connect())
					return;
			} else if (ev.buttonId == BTN_JOIN_BACK) {
				g_state   = STATE_MENU;
				g_hoverId = BTN_NONE;
				set_message("");
				return;
			}
		}
	}

	ui_draw_net_join(g_joinIp, cursorBlink, g_hoverId, g_message);
	ui_flush();
}

static void net_wait_frame(void) {
	static const UiButton cancelButton = { 490, 470, 200, 44, BTN_NET_CANCEL, "返  回" };
	UiButton buttons[1];
	UiEvent  ev;
	int      type;
	int      r;
	int      c;

	buttons[0] = cancelButton;

	if (net_poll(&type, &r, &c)) {
		if (type == NET_MSG_START) {
			start_new_game(MODE_NET);
			return;
		}
		if (type == NET_MSG_CLOSE) {
			set_message("连接已断开。");
			net_close();
			g_state = STATE_MENU;
			return;
		}
	}

	while (input_poll(buttons, 1, 0, &ev)) {
		g_hoverId = ev.buttonId;
		if (ev.type == EVENT_BUTTON && ev.buttonId == BTN_NET_CANCEL) {
			net_close();
			g_state   = STATE_MENU;
			g_hoverId = BTN_NONE;
			set_message("");
			return;
		}
	}

	ui_draw_net_wait(g_info.humanColor == CELL_BLACK,
	                 net_state_text(), net_local_ip(),
	                 (g_info.humanColor == CELL_WHITE) ? g_joinIp : NULL,
	                 g_hoverId);
	ui_flush();
}

static void menu_frame(void) {
	const UiButton *buttons = NULL;
	int             count   = ui_menu_buttons(&buttons);
	UiEvent         ev;

	/* 算法百科页模式（翻页浏览） */
	if (g_showAlgoBook) {
		UiButton bookButtons[3];
		int      bookCount = ui_get_algo_buttons(g_algoPage, bookButtons, 3);

		while (input_poll(bookButtons, bookCount, 0, &ev)) {
			g_hoverId = ev.buttonId;
			if (ev.type == EVENT_BUTTON) {
				if (ev.buttonId == BTN_ALGO_PREV && g_algoPage > 0)
					g_algoPage--;
				else if (ev.buttonId == BTN_ALGO_NEXT && g_algoPage < ui_algo_page_count() - 1)
					g_algoPage++;
				else if (ev.buttonId == BTN_ALGO_BACK) {
					g_showAlgoBook = 0;
					g_hoverId     = BTN_NONE;
					set_message("");
				}
			}
		}
		ui_draw_algo_book(g_algoPage, g_hoverId);
		ui_flush();
		return;
	}

	/* 难度详情弹窗模式 */
	if (g_showDiffDialog) {
		/* 风格按钮布局统一由 ui_get_style_buttons 提供（与绘制共用，避免两处维护坐标） */
		UiButton styleButtons[9];
		int      styleCount = ui_get_style_buttons(g_pendingLevel, styleButtons, 9);
		UiButton dialogButtons[11];
		int      dialogCount = 2;
		int      si;

		dialogButtons[0] = (UiButton) {
			330, 668, 180, 40, BTN_DIFF_CONFIRM, "确认选择"
		};
		dialogButtons[1] = (UiButton) {
			650, 668, 180, 40, BTN_DIFF_CANCEL,  "取消"
		};

		for (si = 0; si < styleCount; si++) {
			dialogButtons[dialogCount++] = styleButtons[si];
		}

		while (input_poll(dialogButtons, dialogCount, 0, &ev)) {
			g_hoverId = ev.buttonId;
			if (ev.type == EVENT_BUTTON) {
				if (ev.buttonId >= BTN_STYLE_NORMAL && ev.buttonId <= BTN_STYLE_ATTACK) {
					/* 行1 困难+;行2 大师+;行3 棋圣+(按钮仅按难度放置,此处兜底) */
					int reqLevel = LEVEL_HARD;
					if (ev.buttonId >= BTN_STYLE_COUNTER && ev.buttonId <= BTN_STYLE_DOUBLE)
						reqLevel = LEVEL_MASTER;
					else if (ev.buttonId >= BTN_STYLE_TRICK)
						reqLevel = LEVEL_GRANDMASTER;
					if (g_pendingLevel >= reqLevel) {
						switch (ev.buttonId) {
							case BTN_STYLE_NORMAL:
								g_pendingStyle = STYLE_NORMAL;
								break;
							case BTN_STYLE_AGGRESSIVE:
								g_pendingStyle = STYLE_AGGRESSIVE;
								break;
							case BTN_STYLE_WALL:
								g_pendingStyle = STYLE_WALL;
								break;
							case BTN_STYLE_COUNTER:
								g_pendingStyle = STYLE_COUNTER;
								break;
							case BTN_STYLE_CONTROL:
								g_pendingStyle = STYLE_CONTROL;
								break;
							case BTN_STYLE_DOUBLE:
								g_pendingStyle = STYLE_DOUBLE;
								break;
							case BTN_STYLE_TRICK:
								g_pendingStyle = STYLE_TRICK;
								break;
							case BTN_STYLE_BAIT:
								g_pendingStyle = STYLE_BAIT;
								break;
							default:
								g_pendingStyle = STYLE_ATTACK;
								break;
						}
					}
				} else if (ev.buttonId == BTN_DIFF_CONFIRM) {
					g_info.level = g_pendingLevel;
					g_info.style = g_pendingStyle;
					g_showDiffDialog = 0;
					g_hoverId = BTN_NONE;
					set_message("");
				} else if (ev.buttonId == BTN_DIFF_CANCEL) {
					g_showDiffDialog = 0;
					g_hoverId = BTN_NONE;
					set_message("");
				}
			}
		}
		ui_draw_menu(&g_info, BTN_NONE);
		ui_draw_diff_dialog(g_pendingLevel, g_pendingStyle, g_hoverId);
		ui_flush();
		return;
	}

	while (input_poll(buttons, count, 0, &ev)) {
		g_hoverId = ev.buttonId;

		if (ev.type == EVENT_BUTTON) {
			/* 难度按钮：打开详情弹窗 */
			if (ev.buttonId == BTN_LEVEL_BEGINNER || ev.buttonId == BTN_LEVEL_EASY ||
			        ev.buttonId == BTN_LEVEL_NORMAL || ev.buttonId == BTN_LEVEL_HARD ||
			        ev.buttonId == BTN_LEVEL_MASTER || ev.buttonId == BTN_LEVEL_GRANDMASTER) {
				switch (ev.buttonId) {
					case BTN_LEVEL_BEGINNER:
						g_pendingLevel = LEVEL_BEGINNER;
						break;
					case BTN_LEVEL_EASY:
						g_pendingLevel = LEVEL_EASY;
						break;
					case BTN_LEVEL_NORMAL:
						g_pendingLevel = LEVEL_NORMAL;
						break;
					case BTN_LEVEL_HARD:
						g_pendingLevel = LEVEL_HARD;
						break;
					case BTN_LEVEL_MASTER:
						g_pendingLevel = LEVEL_MASTER;
						break;
					case BTN_LEVEL_GRANDMASTER:
						g_pendingLevel = LEVEL_GRANDMASTER;
						break;
				}
				g_pendingStyle = g_info.style;
				g_showDiffDialog = 1;
				g_hoverId = BTN_NONE;
				set_message("");
				return;
			}
			handle_menu_button(ev.buttonId);
		}
		if (g_state != STATE_MENU)
			return;
	}

	ui_draw_menu(&g_info, g_hoverId);
	ui_flush();
}

static void handle_net_message(int type, int row, int col) {
	switch (type) {
		case NET_MSG_MOVE:
			if (board_place(&g_board, row, col) == PLACE_OK) {
				if (!announce_result()) {
					set_message("对方落子于 %c%d。", (char)('A' + col), row + 1);
				}
			}
			break;
		case NET_MSG_UNDO:
			board_undo_steps(&g_board, 2);
			set_message("对方请求悔棋，已同步回退。");
			break;
		case NET_MSG_RESIGN:
			board_resign(&g_board, OPPONENT(g_info.humanColor));
			set_message("对方认输，本机获胜。");
			break;
		case NET_MSG_RESTART:
			board_init(&g_board);
			set_message("对方请求重新开始，本局已重置。");
			break;
		case NET_MSG_CLOSE:
			set_message("对方已断开连接。");
			net_close();
			g_state = STATE_MENU;
			break;
		default:
			break;
	}
}

static void game_frame(void) {
	const UiButton *buttons = NULL;
	int             count   = ui_game_buttons(&buttons);
	int             allowBoard;
	int             netType;
	int             netRow;
	int             netCol;
	UiEvent         ev;

	if (g_info.mode == MODE_NET) {
		while (net_poll(&netType, &netRow, &netCol)) {
			handle_net_message(netType, netRow, netCol);
			if (g_state != STATE_GAME)
				return;
		}
	}

	allowBoard = (g_board.status == GAME_PLAYING && is_local_turn());

	while (input_poll(buttons, count, allowBoard, &ev)) {
		g_hoverId  = ev.buttonId;
		g_hoverRow = ev.hoverRow;
		g_hoverCol = ev.hoverCol;

		switch (ev.type) {
			case EVENT_PLACE:
				do_place(ev.row, ev.col);
				break;
			case EVENT_BUTTON:
				handle_game_button(ev.buttonId);
				break;
			case EVENT_OUTSIDE:
				if (g_board.status != GAME_PLAYING) {
					announce_result();
				} else if (is_ai_turn()) {
					set_message("现在是电脑的回合，请稍候。");
				} else if (g_info.mode == MODE_NET && !is_local_turn()) {
					set_message("现在是对方的回合，请稍候。");
				} else {
					set_message("%s", board_place_error(PLACE_ERR_RANGE));
				}
				break;
			default:
				break;
		}

		if (g_state != STATE_GAME)
			return;
		if (is_ai_turn())
			break;
	}

	ui_draw_game(&g_board, &g_info, g_message,
	             g_hoverId, g_hoverRow, g_hoverCol, 0,
	             g_gameHighlightRows, g_gameHighlightCols, g_gameHighlightCount);
	ui_flush();

	if (is_ai_turn()) {
		run_ai_turn();
	}
}

static void replay_frame(void) {
	static const UiButton buttons[] = {
		{ 790, 430, 170, 38, BTN_REPLAY_FIRST, "首步" },
		{ 980, 430, 170, 38, BTN_REPLAY_PREV,  "上一步" },
		{ 790, 476, 170, 38, BTN_REPLAY_NEXT,  "下一步" },
		{ 980, 476, 170, 38, BTN_REPLAY_LAST,  "末步" },
		{ 790, 540, 360, 42, BTN_REPLAY_BACK,  "返回棋谱列表" }
	};
	const int btnCount = 5;
	UiEvent ev;

	while (input_poll(buttons, btnCount, 0, &ev)) {
		g_hoverId = ev.buttonId;
		if (ev.type == EVENT_BUTTON) {
			switch (ev.buttonId) {
				case BTN_REPLAY_FIRST:
					if (g_replayIndex > 0)
						replay_build(0);
					break;
				case BTN_REPLAY_PREV:
					if (g_replayIndex > 0)
						replay_build(g_replayIndex - 1);
					break;
				case BTN_REPLAY_NEXT:
					if (g_replayIndex < g_replayCount)
						replay_build(g_replayIndex + 1);
					break;
				case BTN_REPLAY_LAST:
					if (g_replayIndex < g_replayCount)
						replay_build(g_replayCount);
					break;
				case BTN_REPLAY_BACK:
					g_state   = STATE_LOAD;
					g_hoverId = BTN_NONE;
					set_message("");
					return;
				default:
					break;
			}
		}
	}

	ui_draw_replay(&g_board, &g_info, g_replayIndex, g_replayCount, g_hoverId,
	               g_replayHighlightRows, g_replayHighlightCols, g_replayHighlightCount);
	ui_flush();
}

int main(void) {
	ui_init();
	ai_init((unsigned int)time(NULL));
	record_ensure_dir(RECORD_DIR);

	g_info.mode       = MODE_PVE;
	g_info.level      = LEVEL_NORMAL;
	g_info.humanColor = CELL_BLACK;
	g_info.style      = STYLE_NORMAL;

	board_init(&g_board);
	set_message("");

	while (is_run() && g_state != STATE_EXIT) {
		if (g_state == STATE_MENU) {
			menu_frame();
		} else if (g_state == STATE_LOAD) {
			load_frame();
		} else if (g_state == STATE_NET) {
			net_wait_frame();
		} else if (g_state == STATE_NET_JOIN) {
			net_join_frame();
		} else if (g_state == STATE_REPLAY) {
			replay_frame();
		} else {
			game_frame();
		}
	}

	net_close();
	ui_close();
	return 0;
}
