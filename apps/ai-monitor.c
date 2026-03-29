/**
 * @file apps/ai-monitor.c
 * @brief linAIx AI Neural Monitor - Real-time AI Kernel Visualization
 *
 * A futuristic interface showing the linAIx AI Core's real-time
 * decisions across memory management, process scheduling, and
 * system guardianship. Features animated graphs, neural-style
 * visualizations, and live telemetry from /proc/aicore.
 *
 * @copyright
 * This file is part of linAIxOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>

#include <sys/fswait.h>
#include <sys/time.h>

#include <linAIx/yutani.h>
#include <linAIx/graphics.h>
#include <linAIx/decorations.h>
#include <linAIx/menu.h>
#include <linAIx/text.h>

/* ── Color Palette ── Dark futuristic theme ─────────────── */
#define BG_DARK       rgb(12,  14,  20)
#define BG_PANEL      rgb(18,  22,  32)
#define BG_CARD       rgb(24,  30,  44)
#define BG_CARD_HEAD  rgb(30,  38,  56)
#define BORDER_DIM    rgb(40,  50,  72)
#define BORDER_GLOW   rgb(0,   180, 255)

#define CYAN_BRIGHT   rgb(0,   220, 255)
#define CYAN_MED      rgb(0,   160, 200)
#define CYAN_DIM      rgb(0,   80,  120)
#define GREEN_BRIGHT  rgb(0,   255, 140)
#define GREEN_MED     rgb(0,   200, 100)
#define GREEN_DIM     rgb(0,   100, 60)
#define MAGENTA       rgb(200, 0,   255)
#define MAGENTA_DIM   rgb(100, 0,   140)
#define ORANGE_BRIGHT rgb(255, 160, 0)
#define ORANGE_DIM    rgb(160, 90,  0)
#define RED_BRIGHT    rgb(255, 60,  80)
#define RED_DIM       rgb(140, 30,  40)
#define WHITE         rgb(230, 235, 245)
#define GRAY_TEXT     rgb(140, 150, 170)
#define GRAY_DIM      rgb(80,  90,  110)
#define YELLOW_BRIGHT rgb(255, 230, 0)

/* ── Window layout ──────────────────────────────────────── */
#define WIN_W  900
#define WIN_H  700

static yutani_t * yctx;
static yutani_window_t * window = NULL;
static gfx_context_t * ctx = NULL;

static struct TT_Font * font_thin  = NULL;
static struct TT_Font * font_bold  = NULL;
static struct TT_Font * font_mono  = NULL;

static struct menu_bar menu_bar = {0};
static struct menu_bar_entries menu_entries[] = {
	{"Neural Core", "file"},
	{"Help", "help"},
	{NULL, NULL},
};

static int should_exit = 0;
static int tick = 0;  /* animation frame counter */

/* ── AI Core Data ───────────────────────────────────────── */
struct ai_data {
	/* Header */
	char version[32];
	char status[32];
	unsigned long uptime;
	unsigned long decisions;
	int  interventions;
	unsigned long eval_cycle;

	/* Memory */
	long mem_total;
	long mem_used;
	long mem_free;
	long kheap;
	int  mem_pressure;
	char mem_strategy[32];
	long pages_managed;
	int  cache_hit_rate;

	/* Scheduler */
	int  proc_total;
	int  proc_running;
	int  proc_sleeping;
	int  proc_stopped;
	int  proc_suspended;
	char sched_policy[32];
	int  ctx_switches;

	/* Guardian */
	char threat_level[32];
	int  opt_score;
	int  io_score;

	/* Top processes */
	struct {
		int pid;
		char name[64];
		char state[4];
		long mem;
		int  ai_priority;
	} procs[20];
	int proc_count;

	/* Action log */
	struct {
		unsigned long timestamp;
		char type[16];
		int  pid;
		char detail[96];
	} actions[32];
	int action_count;
};

static struct ai_data ai = {0};

/* Graph history */
#define GRAPH_SAMPLES 80
static int mem_history[GRAPH_SAMPLES];
static int cpu_history[GRAPH_SAMPLES];
static int opt_history[GRAPH_SAMPLES];
static int graph_initialized = 0;

/* ── Helpers ────────────────────────────────────────────── */

static void parse_aicore(void) {
	FILE * f = fopen("/proc/aicore", "r");
	if (!f) {
		strcpy(ai.version, "N/A");
		strcpy(ai.status, "OFFLINE");
		return;
	}

	char line[256];
	char section[32] = {0};
	ai.proc_count = 0;
	ai.action_count = 0;

	while (fgets(line, sizeof(line), f)) {
		/* Remove newline */
		char * nl = strchr(line, '\n');
		if (nl) *nl = '\0';
		char * cr = strchr(line, '\r');
		if (cr) *cr = '\0';

		if (line[0] == '[') {
			/* Section header */
			char * end = strchr(line, ']');
			if (end) {
				*end = '\0';
				strncpy(section, line + 1, sizeof(section) - 1);
			}
			continue;
		}

		if (line[0] == '\0') continue;

		if (!strcmp(section, "PROCESSES")) {
			if (ai.proc_count < 20) {
				int n = ai.proc_count;
				if (sscanf(line, "%d %63s %3s %ld %d",
					&ai.procs[n].pid, ai.procs[n].name,
					ai.procs[n].state, &ai.procs[n].mem,
					&ai.procs[n].ai_priority) >= 4) {
					ai.proc_count++;
				}
			}
			continue;
		}

		if (!strcmp(section, "ACTIONS")) {
			/* Parse: timestamp|type|pid|detail */
			if (ai.action_count < 32) {
				int n = ai.action_count;
				char * p1 = strchr(line, '|');
				if (!p1) continue;
				*p1 = '\0'; p1++;
				char * p2 = strchr(p1, '|');
				if (!p2) continue;
				*p2 = '\0'; p2++;
				char * p3 = strchr(p2, '|');
				if (!p3) continue;
				*p3 = '\0'; p3++;

				ai.actions[n].timestamp = strtoul(line, NULL, 10);
				strncpy(ai.actions[n].type, p1, 15);
				ai.actions[n].type[15] = '\0';
				ai.actions[n].pid = atoi(p2);
				strncpy(ai.actions[n].detail, p3, 95);
				ai.actions[n].detail[95] = '\0';
				ai.action_count++;
			}
			continue;
		}

		/* Key: Value parsing */
		char * colon = strchr(line, ':');
		if (!colon) continue;
		*colon = '\0';
		char * key = line;
		char * val = colon + 1;
		while (*val == ' ') val++;

		if (!strcmp(section, "HEADER")) {
			if (!strcmp(key, "AICore_Version"))       strncpy(ai.version, val, 31);
			else if (!strcmp(key, "AICore_Status"))        strncpy(ai.status, val, 31);
			else if (!strcmp(key, "AICore_Uptime"))        ai.uptime = strtoul(val, NULL, 10);
			else if (!strcmp(key, "AICore_Decisions"))     ai.decisions = strtoul(val, NULL, 10);
			else if (!strcmp(key, "AICore_Interventions")) ai.interventions = atoi(val);
			else if (!strcmp(key, "AICore_Eval_Cycle"))    ai.eval_cycle = strtoul(val, NULL, 10);
		} else if (!strcmp(section, "MEMORY")) {
			if (!strcmp(key, "Mem_Total_kB"))      ai.mem_total = atol(val);
			else if (!strcmp(key, "Mem_Used_kB"))       ai.mem_used = atol(val);
			else if (!strcmp(key, "Mem_Free_kB"))       ai.mem_free = atol(val);
			else if (!strcmp(key, "KHeap_kB"))          ai.kheap = atol(val);
			else if (!strcmp(key, "Mem_Pressure"))      ai.mem_pressure = atoi(val);
			else if (!strcmp(key, "Mem_Strategy"))      strncpy(ai.mem_strategy, val, 31);
			else if (!strcmp(key, "Pages_Managed"))     ai.pages_managed = atol(val);
			else if (!strcmp(key, "Cache_HitRate"))     ai.cache_hit_rate = atoi(val);
		} else if (!strcmp(section, "SCHEDULER")) {
			if (!strcmp(key, "Proc_Total"))        ai.proc_total = atoi(val);
			else if (!strcmp(key, "Proc_Running"))      ai.proc_running = atoi(val);
			else if (!strcmp(key, "Proc_Sleeping"))     ai.proc_sleeping = atoi(val);
			else if (!strcmp(key, "Proc_Stopped"))      ai.proc_stopped = atoi(val);
			else if (!strcmp(key, "Proc_Suspended"))    ai.proc_suspended = atoi(val);
			else if (!strcmp(key, "Sched_Policy"))      strncpy(ai.sched_policy, val, 31);
			else if (!strcmp(key, "Ctx_Switches_Sec"))  ai.ctx_switches = atoi(val);
		} else if (!strcmp(section, "GUARDIAN")) {
			if (!strcmp(key, "Threat_Level"))      strncpy(ai.threat_level, val, 31);
			else if (!strcmp(key, "Optimization_Score")) ai.opt_score = atoi(val);
			else if (!strcmp(key, "IO_Score"))           ai.io_score = atoi(val);
		}
	}

	fclose(f);
}

static void update_history(void) {
	if (!graph_initialized) {
		for (int i = 0; i < GRAPH_SAMPLES; i++) {
			mem_history[i] = -1;
			cpu_history[i] = -1;
			opt_history[i] = -1;
		}
		graph_initialized = 1;
	}

	memmove(&mem_history[0], &mem_history[1], (GRAPH_SAMPLES - 1) * sizeof(int));
	memmove(&cpu_history[0], &cpu_history[1], (GRAPH_SAMPLES - 1) * sizeof(int));
	memmove(&opt_history[0], &opt_history[1], (GRAPH_SAMPLES - 1) * sizeof(int));

	mem_history[GRAPH_SAMPLES - 1] = ai.mem_pressure;
	opt_history[GRAPH_SAMPLES - 1] = ai.opt_score;

	/* CPU from /proc/idle */
	FILE * cf = fopen("/proc/idle", "r");
	if (cf) {
		char buf[256];
		if (fgets(buf, sizeof(buf), cf)) {
			char * a = strchr(buf, ':');
			if (a) {
				a++;
				int idle = 0;
				idle += strtol(a, &a, 10);
				idle += strtol(a, &a, 10);
				idle += strtol(a, &a, 10);
				idle += strtol(a, &a, 10);
				idle /= 4;
				if (idle > 1000) idle = 1000;
				if (idle < 0) idle = 0;
				cpu_history[GRAPH_SAMPLES - 1] = 100 - (idle / 10);
			}
		}
		fclose(cf);
	}
}

/* ── Drawing Primitives ─────────────────────────────────── */

static void draw_filled_rounded_rect(gfx_context_t * ctx, int x, int y, int w, int h, uint32_t color) {
	/* Safe filled rectangle using library call */
	draw_rectangle(ctx, x, y, w, h, color);
}

static void draw_rect_outline(gfx_context_t * ctx, int x, int y, int w, int h, uint32_t color) {
	draw_line(ctx, x, x + w - 1, y, y, color);
	draw_line(ctx, x, x + w - 1, y + h - 1, y + h - 1, color);
	draw_line(ctx, x, x, y, y + h - 1, color);
	draw_line(ctx, x + w - 1, x + w - 1, y, y + h - 1, color);
}

static void draw_progress_bar(gfx_context_t * ctx, int x, int y, int w, int h, int pct, uint32_t fg, uint32_t bg) {
	/* Background */
	draw_rectangle(ctx, x, y, w, h, bg);
	/* Filled portion */
	int fill_w = (w * pct) / 100;
	if (fill_w > w) fill_w = w;
	if (fill_w > 0) {
		draw_rectangle(ctx, x, y, fill_w, h, fg);
	}
}

static void draw_graph_line(gfx_context_t * ctx, int x, int y, int w, int h,
                            int * data, int max_val, uint32_t line_color, uint32_t fill_color) {
	if (max_val == 0) max_val = 1;
	float step = (float)w / (float)(GRAPH_SAMPLES - 1);

	for (int i = 1; i < GRAPH_SAMPLES; i++) {
		if (data[i] < 0 || data[i - 1] < 0) continue;

		int x0 = x + (int)((i - 1) * step);
		int x1 = x + (int)(i * step);
		int y0 = y + h - (data[i - 1] * h / max_val);
		int y1 = y + h - (data[i] * h / max_val);

		if (y0 < y) y0 = y;
		if (y1 < y) y1 = y;
		if (y0 > y + h) y0 = y + h;
		if (y1 > y + h) y1 = y + h;

		/* Fill below the line using safe vertical strips */
		for (int px = x0; px <= x1 && px < x + w; px++) {
			float t = (x1 == x0) ? 1.0f : (float)(px - x0) / (float)(x1 - x0);
			int py = (int)(y0 + t * (y1 - y0));
			if (py < y) py = y;
			int col_h = y + h - py;
			if (col_h > 0) {
				draw_rectangle(ctx, px, py, 1, col_h, fill_color);
			}
		}

		/* Draw the line itself */
		draw_line(ctx, x0, x1, y0, y1, line_color);
		if (y0 + 1 <= y + h && y1 + 1 <= y + h) {
			draw_line(ctx, x0, x1, y0 + 1, y1 + 1, line_color);
		}
	}
}

static void draw_mini_bar(gfx_context_t * ctx, int x, int y, int w, int h,
                          int val, int max_val, uint32_t color) {
	if (max_val <= 0) max_val = 1;
	int fill = (val * w) / max_val;
	if (fill > w) fill = w;
	draw_rectangle(ctx, x, y, w, h, rgb(30, 36, 52));
	if (fill > 0) {
		draw_rectangle(ctx, x, y, fill, h, color);
	}
}

/* ── Draw Sections ──────────────────────────────────────── */

static void draw_header(int bx, int by) {
	/* Top banner with pulsing glow effect */
	draw_filled_rounded_rect(ctx, bx, by, WIN_W - 16, 52, BG_CARD);
	draw_rect_outline(ctx, bx, by, WIN_W - 16, 52, BORDER_DIM);

	/* Pulsing status indicator (triangle wave, no math.h needed) */
	int pulse = (tick * 6) % 512;
	if (pulse > 255) pulse = 511 - pulse;
	uint32_t dot_color = rgb(0, pulse > 200 ? 255 : pulse, 255);

	/* Status dot - safe rectangle */
	draw_rectangle(ctx, bx + 11, by + 23, 7, 7, dot_color);

	tt_set_size(font_bold, 16);
	tt_draw_string(ctx, font_bold, bx + 24, by + 21, "linAIx NEURAL CORE", CYAN_BRIGHT);

	tt_set_size(font_thin, 11);
	char status_buf[160];
	snprintf(status_buf, sizeof(status_buf), "v%s | %s | UP %luh%lum | ACTIONS: %lu | ACTIVE: %d",
		ai.version, ai.status, ai.uptime / 3600, (ai.uptime % 3600) / 60, ai.decisions, ai.interventions);
	tt_draw_string(ctx, font_thin, bx + 24, by + 40, status_buf, GRAY_TEXT);

	/* Optimization score badge */
	uint32_t score_color = ai.opt_score > 80 ? GREEN_BRIGHT :
	                       ai.opt_score > 60 ? CYAN_BRIGHT :
	                       ai.opt_score > 40 ? ORANGE_BRIGHT : RED_BRIGHT;
	tt_set_size(font_bold, 14);
	char score_buf[32];
	snprintf(score_buf, sizeof(score_buf), "%d%%", ai.opt_score);
	int sw = tt_string_width(font_bold, score_buf);
	tt_draw_string(ctx, font_bold, bx + WIN_W - 36 - sw, by + 22, score_buf, score_color);
	tt_set_size(font_thin, 9);
	int lw = tt_string_width(font_thin, "OPT SCORE");
	tt_draw_string(ctx, font_thin, bx + WIN_W - 36 - lw, by + 40, "OPT SCORE", GRAY_DIM);
}

static void draw_memory_card(int bx, int by, int cw, int ch) {
	draw_filled_rounded_rect(ctx, bx, by, cw, ch, BG_CARD);
	draw_rect_outline(ctx, bx, by, cw, ch, BORDER_DIM);

	/* Card header */
	draw_rectangle(ctx, bx + 1, by + 1, cw - 2, 24, BG_CARD_HEAD);

	tt_set_size(font_bold, 11);
	tt_draw_string(ctx, font_bold, bx + 10, by + 17, "\xE2\x96\xA0 MEMORY AI", CYAN_BRIGHT);

	/* Strategy badge */
	tt_set_size(font_thin, 9);
	tt_draw_string(ctx, font_thin, bx + cw - 10 - tt_string_width(font_thin, ai.mem_strategy),
		by + 17, ai.mem_strategy, GREEN_MED);

	/* Memory stats */
	int sy = by + 34;
	char buf[128];

	tt_set_size(font_thin, 10);
	snprintf(buf, sizeof(buf), "Total: %ld MB", ai.mem_total / 1024);
	tt_draw_string(ctx, font_thin, bx + 10, sy, buf, GRAY_TEXT);

	snprintf(buf, sizeof(buf), "Used: %ld MB  Free: %ld MB", ai.mem_used / 1024, ai.mem_free / 1024);
	tt_draw_string(ctx, font_thin, bx + 10, sy + 16, buf, GRAY_TEXT);

	/* Memory pressure bar */
	sy += 34;
	tt_set_size(font_thin, 9);
	tt_draw_string(ctx, font_thin, bx + 10, sy, "PRESSURE", GRAY_DIM);

	uint32_t pbar_color = ai.mem_pressure > 80 ? RED_BRIGHT :
	                      ai.mem_pressure > 50 ? ORANGE_BRIGHT : CYAN_BRIGHT;
	draw_progress_bar(ctx, bx + 10, sy + 5, cw - 60, 8, ai.mem_pressure, pbar_color, rgb(30, 36, 52));

	snprintf(buf, sizeof(buf), "%d%%", ai.mem_pressure);
	tt_draw_string(ctx, font_thin, bx + cw - 40, sy, buf, pbar_color);

	/* Cache hit rate */
	sy += 22;
	tt_draw_string(ctx, font_thin, bx + 10, sy, "CACHE HIT", GRAY_DIM);
	draw_progress_bar(ctx, bx + 10, sy + 5, cw - 60, 8, ai.cache_hit_rate, GREEN_MED, rgb(30, 36, 52));
	snprintf(buf, sizeof(buf), "%d%%", ai.cache_hit_rate);
	tt_draw_string(ctx, font_thin, bx + cw - 40, sy, buf, GREEN_MED);

	/* Kernel heap */
	sy += 22;
	snprintf(buf, sizeof(buf), "Kernel Heap: %ld kB", ai.kheap);
	tt_draw_string(ctx, font_thin, bx + 10, sy, buf, GRAY_DIM);

	/* Pages info */
	sy += 14;
	snprintf(buf, sizeof(buf), "Pages managed: %ld", ai.pages_managed);
	tt_draw_string(ctx, font_thin, bx + 10, sy, buf, GRAY_DIM);

	/* Memory graph */
	sy += 12;
	int gh = ch - (sy - by) - 8;
	if (gh > 10) {
		draw_rectangle(ctx, bx + 8, sy, cw - 16, gh, rgb(16, 20, 30));
		draw_rect_outline(ctx, bx + 8, sy, cw - 16, gh, rgb(30, 38, 54));
		draw_graph_line(ctx, bx + 9, sy + 1, cw - 18, gh - 2, mem_history, 100, CYAN_BRIGHT, CYAN_DIM);
	}
}

static void draw_scheduler_card(int bx, int by, int cw, int ch) {
	draw_filled_rounded_rect(ctx, bx, by, cw, ch, BG_CARD);
	draw_rect_outline(ctx, bx, by, cw, ch, BORDER_DIM);

	draw_rectangle(ctx, bx + 1, by + 1, cw - 2, 24, BG_CARD_HEAD);

	tt_set_size(font_bold, 11);
	tt_draw_string(ctx, font_bold, bx + 10, by + 17, "\xE2\x96\xA0 PROCESS AI", MAGENTA);

	tt_set_size(font_thin, 9);
	tt_draw_string(ctx, font_thin, bx + cw - 10 - tt_string_width(font_thin, ai.sched_policy),
		by + 17, ai.sched_policy, GREEN_MED);

	int sy = by + 34;
	char buf[128];
	tt_set_size(font_thin, 10);

	/* Process stats in a grid */
	snprintf(buf, sizeof(buf), "Total: %d", ai.proc_total);
	tt_draw_string(ctx, font_thin, bx + 10, sy, buf, GRAY_TEXT);
	snprintf(buf, sizeof(buf), "Running: %d", ai.proc_running);
	tt_draw_string(ctx, font_thin, bx + cw / 2, sy, buf, GREEN_BRIGHT);

	sy += 16;
	snprintf(buf, sizeof(buf), "Sleeping: %d", ai.proc_sleeping);
	tt_draw_string(ctx, font_thin, bx + 10, sy, buf, CYAN_MED);
	snprintf(buf, sizeof(buf), "Stopped: %d", ai.proc_stopped);
	tt_draw_string(ctx, font_thin, bx + cw / 2, sy, buf, RED_DIM);

	sy += 16;
	snprintf(buf, sizeof(buf), "AI Suspended: %d", ai.proc_suspended);
	tt_draw_string(ctx, font_thin, bx + 10, sy, buf,
		ai.proc_suspended > 0 ? ORANGE_BRIGHT : GRAY_DIM);

	/* Context switches */
	sy += 22;
	tt_set_size(font_thin, 9);
	tt_draw_string(ctx, font_thin, bx + 10, sy, "CTX SWITCHES/s", GRAY_DIM);
	snprintf(buf, sizeof(buf), "%d", ai.ctx_switches);
	tt_draw_string(ctx, font_thin, bx + cw - 10 - tt_string_width(font_thin, buf), sy, buf, ORANGE_BRIGHT);

	/* CPU usage graph */
	sy += 14;
	tt_draw_string(ctx, font_thin, bx + 10, sy, "CPU UTILIZATION", GRAY_DIM);
	sy += 8;
	int gh = ch - (sy - by) - 8;
	if (gh > 10) {
		draw_rectangle(ctx, bx + 8, sy, cw - 16, gh, rgb(16, 20, 30));
		draw_rect_outline(ctx, bx + 8, sy, cw - 16, gh, rgb(30, 38, 54));
		draw_graph_line(ctx, bx + 9, sy + 1, cw - 18, gh - 2, cpu_history, 100, MAGENTA, MAGENTA_DIM);
	}
}

static void draw_guardian_card(int bx, int by, int cw, int ch) {
	draw_filled_rounded_rect(ctx, bx, by, cw, ch, BG_CARD);
	draw_rect_outline(ctx, bx, by, cw, ch, BORDER_DIM);

	draw_rectangle(ctx, bx + 1, by + 1, cw - 2, 24, BG_CARD_HEAD);

	tt_set_size(font_bold, 11);
	tt_draw_string(ctx, font_bold, bx + 10, by + 17, "\xE2\x96\xA0 SYSTEM GUARDIAN", ORANGE_BRIGHT);

	/* Threat level */
	uint32_t threat_color = GREEN_BRIGHT;
	if (!strcmp(ai.threat_level, "ELEVATED")) threat_color = RED_BRIGHT;
	else if (!strcmp(ai.threat_level, "ADVISORY")) threat_color = ORANGE_BRIGHT;

	tt_set_size(font_thin, 9);
	tt_draw_string(ctx, font_thin, bx + cw - 10 - tt_string_width(font_thin, ai.threat_level),
		by + 17, ai.threat_level, threat_color);

	int sy = by + 34;
	char buf[64];
	tt_set_size(font_thin, 10);

	/* Optimization score circle representation */
	tt_draw_string(ctx, font_thin, bx + 10, sy, "System Health", GRAY_TEXT);
	sy += 6;

	/* Big score display */
	tt_set_size(font_bold, 28);
	snprintf(buf, sizeof(buf), "%d", ai.opt_score);
	uint32_t score_col = ai.opt_score > 80 ? GREEN_BRIGHT :
	                     ai.opt_score > 60 ? CYAN_BRIGHT :
	                     ai.opt_score > 40 ? ORANGE_BRIGHT : RED_BRIGHT;
	int numw = tt_string_width(font_bold, buf);
	tt_draw_string(ctx, font_bold, bx + (cw - numw) / 2, sy + 30, buf, score_col);

	tt_set_size(font_thin, 9);
	int labelw = tt_string_width(font_thin, "HEALTH INDEX");
	tt_draw_string(ctx, font_thin, bx + (cw - labelw) / 2, sy + 42, "HEALTH INDEX", GRAY_DIM);

	/* Sub-scores */
	sy += 56;
	tt_set_size(font_thin, 9);

	tt_draw_string(ctx, font_thin, bx + 10, sy, "I/O PERF", GRAY_DIM);
	draw_progress_bar(ctx, bx + 10, sy + 5, cw - 60, 6, ai.io_score, ORANGE_BRIGHT, rgb(30, 36, 52));
	snprintf(buf, sizeof(buf), "%d%%", ai.io_score);
	tt_draw_string(ctx, font_thin, bx + cw - 40, sy, buf, ORANGE_BRIGHT);

	/* Optimization history graph */
	sy += 22;
	tt_draw_string(ctx, font_thin, bx + 10, sy, "OPTIMIZATION TREND", GRAY_DIM);
	sy += 8;
	int gh = ch - (sy - by) - 8;
	if (gh > 10) {
		draw_rectangle(ctx, bx + 8, sy, cw - 16, gh, rgb(16, 20, 30));
		draw_rect_outline(ctx, bx + 8, sy, cw - 16, gh, rgb(30, 38, 54));
		draw_graph_line(ctx, bx + 9, sy + 1, cw - 18, gh - 2, opt_history, 100, GREEN_BRIGHT, GREEN_DIM);
	}
}

static void draw_process_table(int bx, int by, int tw, int th) {
	draw_filled_rounded_rect(ctx, bx, by, tw, th, BG_CARD);
	draw_rect_outline(ctx, bx, by, tw, th, BORDER_DIM);

	draw_rectangle(ctx, bx + 1, by + 1, tw - 2, 24, BG_CARD_HEAD);

	tt_set_size(font_bold, 11);
	tt_draw_string(ctx, font_bold, bx + 10, by + 17, "\xE2\x96\xA0 AI PROCESS PRIORITY MAP", CYAN_BRIGHT);

	/* Table header */
	int sy = by + 30;
	tt_set_size(font_thin, 9);
	tt_draw_string(ctx, font_thin, bx + 10, sy, "PID", GRAY_DIM);
	tt_draw_string(ctx, font_thin, bx + 52, sy, "PROCESS", GRAY_DIM);
	tt_draw_string(ctx, font_thin, bx + 200, sy, "ST", GRAY_DIM);
	tt_draw_string(ctx, font_thin, bx + 230, sy, "MEM", GRAY_DIM);
	tt_draw_string(ctx, font_thin, bx + 310, sy, "AI PRIORITY", GRAY_DIM);

	/* Separator */
	draw_line(ctx, bx + 8, bx + tw - 8, sy + 5, sy + 5, BORDER_DIM);

	/* Process rows */
	sy += 14;
	int max_rows = (th - 50) / 14;
	if (max_rows > ai.proc_count) max_rows = ai.proc_count;
	if (max_rows > 14) max_rows = 14;

	for (int i = 0; i < max_rows && sy < by + th - 6; i++) {
		char buf[64];

		/* Alternating row background */
		if (i % 2 == 0) {
			draw_rectangle(ctx, bx + 2, sy - 9, tw - 4, 14, rgb(20, 26, 38));
		}

		uint32_t text_col = GRAY_TEXT;
		if (!strcmp(ai.procs[i].state, "R")) text_col = GREEN_BRIGHT;

		snprintf(buf, sizeof(buf), "%d", ai.procs[i].pid);
		tt_draw_string(ctx, font_thin, bx + 10, sy, buf, text_col);

		/* Truncate process name */
		char name_trunc[20];
		strncpy(name_trunc, ai.procs[i].name, 18);
		name_trunc[18] = '\0';
		tt_draw_string(ctx, font_thin, bx + 52, sy, name_trunc, text_col);

		tt_draw_string(ctx, font_thin, bx + 200, sy, ai.procs[i].state, text_col);

		snprintf(buf, sizeof(buf), "%ld kB", ai.procs[i].mem);
		tt_draw_string(ctx, font_thin, bx + 230, sy, buf, GRAY_DIM);

		/* Priority bar */
		uint32_t pri_color = ai.procs[i].ai_priority > 70 ? GREEN_BRIGHT :
		                     ai.procs[i].ai_priority > 40 ? CYAN_MED : GRAY_DIM;
		draw_mini_bar(ctx, bx + 310, sy - 6, 80, 6,
		              ai.procs[i].ai_priority, 100, pri_color);

		snprintf(buf, sizeof(buf), "%d", ai.procs[i].ai_priority);
		tt_draw_string(ctx, font_thin, bx + 396, sy, buf, pri_color);

		sy += 14;
	}
}

/* ── Action Log Panel ───────────────────────────────────── */

static void draw_action_log(int bx, int by, int tw, int th) {
	draw_filled_rounded_rect(ctx, bx, by, tw, th, BG_CARD);
	draw_rect_outline(ctx, bx, by, tw, th, BORDER_DIM);

	draw_rectangle(ctx, bx + 1, by + 1, tw - 2, 24, BG_CARD_HEAD);

	tt_set_size(font_bold, 11);
	tt_draw_string(ctx, font_bold, bx + 10, by + 17, "\xE2\x96\xA0 AI ACTION LOG", YELLOW_BRIGHT);

	char count_buf[32];
	snprintf(count_buf, sizeof(count_buf), "%lu total", ai.decisions);
	tt_set_size(font_thin, 9);
	tt_draw_string(ctx, font_thin, bx + tw - 10 - tt_string_width(font_thin, count_buf),
		by + 17, count_buf, GRAY_DIM);

	int sy = by + 32;
	int max_rows = (th - 38) / 13;
	if (max_rows > ai.action_count) max_rows = ai.action_count;
	if (max_rows > 20) max_rows = 20;

	for (int i = 0; i < max_rows && sy < by + th - 6; i++) {
		/* Color based on action type */
		uint32_t type_color = GRAY_TEXT;
		if (!strncmp(ai.actions[i].type, "MEM_SUSPEND", 11))  type_color = RED_BRIGHT;
		else if (!strncmp(ai.actions[i].type, "MEM_RESUME", 10))   type_color = GREEN_BRIGHT;
		else if (!strncmp(ai.actions[i].type, "CPU_THROTTLE", 12)) type_color = ORANGE_BRIGHT;
		else if (!strncmp(ai.actions[i].type, "CPU_RELEASE", 11))  type_color = GREEN_MED;
		else if (!strncmp(ai.actions[i].type, "ANOMALY", 7))       type_color = MAGENTA;
		else if (!strncmp(ai.actions[i].type, "POLICY", 6))        type_color = CYAN_MED;
		else if (!strncmp(ai.actions[i].type, "STARTUP", 7))       type_color = CYAN_BRIGHT;
		else if (!strncmp(ai.actions[i].type, "DETECT", 6))        type_color = ORANGE_DIM;

		/* Alternating row background */
		if (i % 2 == 0) {
			draw_rectangle(ctx, bx + 2, sy - 9, tw - 4, 13, rgb(20, 26, 38));
		}

		/* Timestamp */
		char ts_buf[16];
		unsigned long t = ai.actions[i].timestamp;
		snprintf(ts_buf, sizeof(ts_buf), "%lu:%02lu", t / 60, t % 60);
		tt_set_size(font_mono, 8);
		tt_draw_string(ctx, font_mono, bx + 6, sy, ts_buf, GRAY_DIM);

		/* Type badge */
		tt_set_size(font_bold, 8);
		tt_draw_string(ctx, font_bold, bx + 52, sy, ai.actions[i].type, type_color);

		/* Detail text - truncate to fit */
		tt_set_size(font_thin, 8);
		char detail_trunc[60];
		strncpy(detail_trunc, ai.actions[i].detail, 58);
		detail_trunc[58] = '\0';
		tt_draw_string(ctx, font_thin, bx + 140, sy, detail_trunc, GRAY_TEXT);

		sy += 13;
	}

	if (ai.action_count == 0) {
		tt_set_size(font_thin, 10);
		tt_draw_string(ctx, font_thin, bx + 10, by + 50, "Waiting for AI actions...", GRAY_DIM);
	}
}

/* ── Decorative Elements ────────────────────────────────── */

static void draw_scan_line(int bx, int by, int full_w, int full_h) {
	/* Subtle horizontal scan line that moves down */
	int scan_y = by + (tick * 2) % full_h;
	if (scan_y >= by && scan_y < by + full_h) {
		/* Safe line draw instead of direct pixel manipulation */
		draw_line(ctx, bx, bx + full_w - 1, scan_y, scan_y, rgb(32, 40, 56));
	}
}

static void draw_corner_accents(int bx, int by, int full_w, int full_h) {
	int len = 12;
	uint32_t ac = CYAN_DIM;

	/* Top-left */
	draw_line(ctx, bx, bx + len, by, by, ac);
	draw_line(ctx, bx, bx, by, by + len, ac);
	/* Top-right */
	draw_line(ctx, bx + full_w - len, bx + full_w, by, by, ac);
	draw_line(ctx, bx + full_w, bx + full_w, by, by + len, ac);
	/* Bottom-left */
	draw_line(ctx, bx, bx + len, by + full_h, by + full_h, ac);
	draw_line(ctx, bx, bx, by + full_h - len, by + full_h, ac);
	/* Bottom-right */
	draw_line(ctx, bx + full_w - len, bx + full_w, by + full_h, by + full_h, ac);
	draw_line(ctx, bx + full_w, bx + full_w, by + full_h - len, by + full_h, ac);
}

/* ── Main redraw ────────────────────────────────────────── */

static struct decor_bounds bounds;

static void redraw(struct menu_bar * _mbar) {
	(void)_mbar;
	decor_get_bounds(window, &bounds);

	int bx = bounds.left_width;
	int by = bounds.top_height + MENU_BAR_HEIGHT;
	int inner_w = WIN_W;
	int inner_h = WIN_H - MENU_BAR_HEIGHT;

	/* Dark background */
	draw_fill(ctx, BG_DARK);

	/* Decorations and menu bar */
	render_decorations(window, ctx, "linAIx AI Neural Monitor");
	menu_bar.x = bounds.left_width;
	menu_bar.y = bounds.top_height;
	menu_bar.width = WIN_W;
	menu_bar.window = window;
	menu_bar_render(&menu_bar, ctx);

	/* Content background */
	draw_rectangle(ctx, bx, by, inner_w, inner_h, BG_DARK);

	int pad = 8;
	int card_w = (inner_w - pad * 4) / 3;
	int card_h = (inner_h - 62 - pad * 3) * 45 / 100;
	int bottom_h = inner_h - 62 - card_h - pad * 3;

	/* Header banner */
	draw_header(bx + pad, by + pad);

	/* Three main cards */
	int cards_y = by + pad + 58;

	draw_memory_card(bx + pad, cards_y, card_w, card_h);
	draw_scheduler_card(bx + pad * 2 + card_w, cards_y, card_w, card_h);
	draw_guardian_card(bx + pad * 3 + card_w * 2, cards_y, card_w, card_h);

	/* Bottom row: Action Log (left 55%) | Process Table (right 45%) */
	int bottom_y = cards_y + card_h + pad;
	int log_w = (inner_w - pad * 3) * 55 / 100;
	int table_w = inner_w - pad * 3 - log_w;

	draw_action_log(bx + pad, bottom_y, log_w, bottom_h);
	draw_process_table(bx + pad * 2 + log_w, bottom_y, table_w, bottom_h);

	/* Decorative scan line */
	draw_scan_line(bx, by, inner_w, inner_h);

	/* Corner accents on the content area */
	draw_corner_accents(bx + 4, by + 4, inner_w - 8, inner_h - 8);

	/* Status bar at very bottom */
	tt_set_size(font_thin, 8);
	char footer[128];
	snprintf(footer, sizeof(footer), "linAIx Neural Core v%s  |  Refresh: 1s  |  Tick: %d",
		ai.version, tick);
	tt_draw_string(ctx, font_thin, bx + pad, by + inner_h - 4, footer, GRAY_DIM);

	flip(ctx);
	yutani_flip(yctx, window);
}

/* ── Menu callbacks ─────────────────────────────────────── */

static void _menu_action_exit(struct MenuEntry * entry) {
	(void)entry;
	should_exit = 1;
}

static void _menu_action_about(struct MenuEntry * entry) {
	(void)entry;
	char about_cmd[512];
	snprintf(about_cmd, 511,
		"about \"About AI Monitor\" /usr/share/icons/48/utilities-system-monitor.png "
		"\"linAIx AI Neural Monitor\" "
		"\"Kernel AI Intelligence Dashboard\n-\n"
		"Part of linAIxOS\n"
		"\xC2\xA9 2024-2026 linAIx Project\"");
	system(about_cmd);
}

static void _menu_action_help(struct MenuEntry * entry) {
	(void)entry;
}

/* ── Main ───────────────────────────────────────────────── */

int main(int argc, char * argv[]) {
	(void)argc; (void)argv;

	yctx = yutani_init();
	if (!yctx) {
		fprintf(stderr, "ai-monitor: failed to connect to compositor\n");
		return 1;
	}

	init_decorations();

	decor_get_bounds(NULL, &bounds);

	window = yutani_window_create(yctx, WIN_W + bounds.width, WIN_H + bounds.height);
	yutani_window_move(yctx, window, (yctx->display_width - window->width) / 2,
	                                  (yctx->display_height - window->height) / 2);
	yutani_window_advertise_icon(yctx, window, "AI Neural Monitor", "utilities-system-monitor");

	ctx = init_graphics_yutani_double_buffer(window);

	font_thin = tt_font_from_shm("sans-serif");
	font_bold = tt_font_from_shm("sans-serif.bold");
	font_mono = tt_font_from_shm("monospace");

	menu_bar.entries = menu_entries;
	menu_bar.redraw_callback = redraw;
	menu_bar.set = menu_set_create();

	struct MenuList * m = menu_create();
	menu_insert(m, menu_create_normal("exit", NULL, "Exit", _menu_action_exit));
	menu_set_insert(menu_bar.set, "file", m);

	m = menu_create();
	menu_insert(m, menu_create_normal("help", NULL, "About AI Monitor", _menu_action_about));
	menu_set_insert(menu_bar.set, "help", m);

	/* Initial data fetch */
	parse_aicore();
	update_history();
	redraw(NULL);

	while (!should_exit) {
		int fds[1] = {fileno(yctx->sock)};
		int index = fswait2(1, fds, 1000); /* 1 second refresh */

		if (index == 0) {
			/* GUI event */
			yutani_msg_t * m = yutani_poll(yctx);
			while (m) {
				if (menu_process_event(yctx, m)) {
					redraw(NULL);
				}
				switch (m->type) {
					case YUTANI_MSG_KEY_EVENT: {
						struct yutani_msg_key_event * ke = (void*)m->data;
						if (ke->event.action == KEY_ACTION_DOWN && ke->event.keycode == 'q') {
							should_exit = 1;
							sched_yield();
						}
						break;
					}
					case YUTANI_MSG_WINDOW_FOCUS_CHANGE: {
						struct yutani_msg_window_focus_change * wf = (void*)m->data;
						yutani_window_t * win = hashmap_get(yctx->windows, (void*)(uintptr_t)wf->wid);
						if (win == window) {
							win->focused = wf->focused;
							redraw(NULL);
						}
						break;
					}
					case YUTANI_MSG_RESIZE_OFFER: {
						struct yutani_msg_window_resize * wr = (void*)m->data;
						yutani_window_resize_accept(yctx, window, wr->width, wr->height);
						reinit_graphics_yutani(ctx, window);
						redraw(NULL);
						yutani_window_resize_done(yctx, window);
						break;
					}
					case YUTANI_MSG_WINDOW_MOUSE_EVENT: {
						struct yutani_msg_window_mouse_event * me = (void*)m->data;
						int result = decor_handle_event(yctx, m);
						switch (result) {
							case DECOR_CLOSE:
								should_exit = 1;
								break;
							case DECOR_RIGHT:
								decor_show_default_menu(window, window->x + me->new_x, window->y + me->new_y);
								break;
						}
						menu_bar_mouse_event(yctx, window, &menu_bar, me, me->new_x, me->new_y);
						break;
					}
					case YUTANI_MSG_WINDOW_CLOSE:
					case YUTANI_MSG_SESSION_END:
						should_exit = 1;
						break;
				}
				free(m);
				m = yutani_poll_async(yctx);
			}
		} else {
			/* Timer expired - refresh data */
			tick++;
			parse_aicore();
			update_history();
			redraw(NULL);
		}
	}

	yutani_close(yctx, window);
	return 0;
}
