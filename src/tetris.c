/*
 * Tetris for Kindle (FBInk). Touch + page-turn buttons. E-ink friendly:
 * delta redraws, fast DU waveform, periodic flashing full refresh.
 *
 * Build: see build.sh. Test render without a device: test/render_test.c.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tetris_core.h"

#ifndef CONF_DIR
#define CONF_DIR "/mnt/us/extensions/tetris"
#endif

/* ------------------------------------------------------------------ gfx */
/* Backend: gfx_rect draws without refreshing; gfx_flush pushes a region.  */
static int  gfx_rect(int x, int y, int w, int h, uint8_t gray);
static void gfx_flush(int x, int y, int w, int h, bool full);

static int SW, SH; /* logical screen size */

/* ---------------------------------------------------------------- font */
/* 5x7 glyphs, 7 rows of 5 chars each ('#' = on). */
static const struct {
	char        c;
	const char* r;
} GLYPHS[] = {
	{'A',
		".###."
		"#...#"
		"#...#"
		"#####"
		"#...#"
		"#...#"
		"#...#"},
	{'B',
		"####."
		"#...#"
		"#...#"
		"####."
		"#...#"
		"#...#"
		"####."},
	{'C',
		".###."
		"#...#"
		"#...."
		"#...."
		"#...."
		"#...#"
		".###."},
	{'D',
		"####."
		"#...#"
		"#...#"
		"#...#"
		"#...#"
		"#...#"
		"####."},
	{'E',
		"#####"
		"#...."
		"#...."
		"####."
		"#...."
		"#...."
		"#####"},
	{'F',
		"#####"
		"#...."
		"#...."
		"####."
		"#...."
		"#...."
		"#...."},
	{'G',
		".###."
		"#...#"
		"#...."
		"#.###"
		"#...#"
		"#...#"
		".####"},
	{'H',
		"#...#"
		"#...#"
		"#...#"
		"#####"
		"#...#"
		"#...#"
		"#...#"},
	{'I',
		".###."
		"..#.."
		"..#.."
		"..#.."
		"..#.."
		"..#.."
		".###."},
	{'J',
		"..###"
		"...#."
		"...#."
		"...#."
		"...#."
		"#..#."
		".##.."},
	{'K',
		"#...#"
		"#..#."
		"#.#.."
		"##..."
		"#.#.."
		"#..#."
		"#...#"},
	{'L',
		"#...."
		"#...."
		"#...."
		"#...."
		"#...."
		"#...."
		"#####"},
	{'M',
		"#...#"
		"##.##"
		"#.#.#"
		"#.#.#"
		"#...#"
		"#...#"
		"#...#"},
	{'N',
		"#...#"
		"##..#"
		"#.#.#"
		"#..##"
		"#...#"
		"#...#"
		"#...#"},
	{'O',
		".###."
		"#...#"
		"#...#"
		"#...#"
		"#...#"
		"#...#"
		".###."},
	{'P',
		"####."
		"#...#"
		"#...#"
		"####."
		"#...."
		"#...."
		"#...."},
	{'Q',
		".###."
		"#...#"
		"#...#"
		"#...#"
		"#.#.#"
		"#..#."
		".##.#"},
	{'R',
		"####."
		"#...#"
		"#...#"
		"####."
		"#.#.."
		"#..#."
		"#...#"},
	{'S',
		".####"
		"#...."
		"#...."
		".###."
		"....#"
		"....#"
		"####."},
	{'T',
		"#####"
		"..#.."
		"..#.."
		"..#.."
		"..#.."
		"..#.."
		"..#.."},
	{'U',
		"#...#"
		"#...#"
		"#...#"
		"#...#"
		"#...#"
		"#...#"
		".###."},
	{'V',
		"#...#"
		"#...#"
		"#...#"
		"#...#"
		"#...#"
		".#.#."
		"..#.."},
	{'W',
		"#...#"
		"#...#"
		"#...#"
		"#.#.#"
		"#.#.#"
		"#.#.#"
		".#.#."},
	{'X',
		"#...#"
		"#...#"
		".#.#."
		"..#.."
		".#.#."
		"#...#"
		"#...#"},
	{'Y',
		"#...#"
		"#...#"
		".#.#."
		"..#.."
		"..#.."
		"..#.."
		"..#.."},
	{'Z',
		"#####"
		"....#"
		"...#."
		"..#.."
		".#..."
		"#...."
		"#####"},
	{'0',
		".###."
		"#...#"
		"#..##"
		"#.#.#"
		"##..#"
		"#...#"
		".###."},
	{'1',
		"..#.."
		".##.."
		"..#.."
		"..#.."
		"..#.."
		"..#.."
		".###."},
	{'2',
		".###."
		"#...#"
		"....#"
		"...#."
		"..#.."
		".#..."
		"#####"},
	{'3',
		"####."
		"....#"
		"....#"
		".###."
		"....#"
		"....#"
		"####."},
	{'4',
		"...#."
		"..##."
		".#.#."
		"#..#."
		"#####"
		"...#."
		"...#."},
	{'5',
		"#####"
		"#...."
		"####."
		"....#"
		"....#"
		"#...#"
		".###."},
	{'6',
		"..##."
		".#..."
		"#...."
		"####."
		"#...#"
		"#...#"
		".###."},
	{'7',
		"#####"
		"....#"
		"...#."
		"..#.."
		".#..."
		".#..."
		".#..."},
	{'8',
		".###."
		"#...#"
		"#...#"
		".###."
		"#...#"
		"#...#"
		".###."},
	{'9',
		".###."
		"#...#"
		"#...#"
		".####"
		"....#"
		"...#."
		".##.."},
	{'<',
		"...#."
		"..#.."
		".#..."
		"#...."
		".#..."
		"..#.."
		"...#."},
	{'>',
		".#..."
		"..#.."
		"...#."
		"....#"
		"...#."
		"..#.."
		".#..."},
	{'.',
		"....."
		"....."
		"....."
		"....."
		"....."
		".##.."
		".##.."},
	{'!',
		"..#.."
		"..#.."
		"..#.."
		"..#.."
		"..#.."
		"....."
		"..#.."},
	{'-',
		"....."
		"....."
		"....."
		"#####"
		"....."
		"....."
		"....."},
	{'?',
		".###."
		"#...#"
		"....#"
		"...#."
		"..#.."
		"....."
		"..#.."},
	{' ',
		"....."
		"....."
		"....."
		"....."
		"....."
		"....."
		"....."},
};

static const char* glyph(char c)
{
	if (c >= 'a' && c <= 'z')
		c = (char) (c - 'a' + 'A');
	for (size_t i = 0; i < sizeof GLYPHS / sizeof GLYPHS[0]; i++)
		if (GLYPHS[i].c == c)
			return GLYPHS[i].r;
	return GLYPHS[sizeof GLYPHS / sizeof GLYPHS[0] - 1].r;
}

static int text_w(const char* s, int sc)
{
	int n = (int) strlen(s);
	return n ? (n * 6 - 1) * sc : 0;
}

static void draw_text(int x, int y, int sc, const char* s, uint8_t gray)
{
	for (; *s; s++, x += 6 * sc) {
		const char* g = glyph(*s);
		for (int row = 0; row < 7; row++) {
			int col = 0;
			while (col < 5) {
				if (g[row * 5 + col] != '#') {
					col++;
					continue;
				}
				int run = col;
				while (run < 5 && g[row * 5 + run] == '#')
					run++;
				gfx_rect(x + col * sc, y + row * sc, (run - col) * sc, sc, gray);
				col = run;
			}
		}
	}
}

/* ------------------------------------------------------------- layout */
typedef struct {
	int cs;                  /* board cell size */
	int bx, by, bw, bh;      /* board rect */
	int px, pw;              /* side panel x, width */
	int title_y, score_y, lines_y, level_y, best_y, next_y, next_box;
	int pause_x, pause_y, pause_w, pause_h; /* panel buttons */
	int quit_x, quit_y, quit_w, quit_h;
	int ctl_y, ctl_h, ctl_x, ctl_w, ctl_gap; /* control row */
} Layout;

static Layout L;

static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }

static void layout_init(void)
{
	int m     = SW / 32;
	int ctl_h = SH * 19 / 100;
	int cs    = imin((int) ((SW - 3 * m) * 2 / 31), (SH - ctl_h - 3 * m) / T_H);
	L.cs      = cs;
	L.bx = m;
	L.by = m;
	L.bw = cs * T_W;
	L.bh = cs * T_H;
	L.px = L.bx + L.bw + m;
	L.pw = SW - L.px - m;

	int s_lbl = imax(2, L.pw / 90);
	int y     = L.by;
	L.title_y = y;
	y += 7 * imax(4, L.pw / 44) + m;
	L.score_y = y;
	y += 7 * s_lbl + 7 * imax(3, L.pw / 55) + m;
	L.lines_y = y;
	y += 7 * s_lbl + 7 * imax(3, L.pw / 55) + m;
	L.level_y = y;
	y += 7 * s_lbl + 7 * imax(3, L.pw / 55) + m;
	L.best_y = y;
	y += 7 * s_lbl + 7 * imax(3, L.pw / 55) + m;
	L.next_y   = y;
	L.next_box = imin(L.pw, cs * 5 / 2 + m);

	L.pause_w = L.quit_w = L.pw;
	L.pause_x = L.quit_x = L.px;
	L.quit_h = L.pause_h = imax(cs * 2, 96);
	L.quit_y             = L.by + L.bh - L.quit_h;
	L.pause_y            = L.quit_y - L.quit_h - m / 2;

	L.ctl_gap = m / 2;
	L.ctl_x   = m;
	L.ctl_w   = SW - 2 * m;
	L.ctl_y   = L.by + L.bh + m;
	L.ctl_h   = SH - L.ctl_y - m;
}

enum { B_LEFT, B_RIGHT, B_ROT, B_DOWN, B_DROP, B_COUNT };
static const char* B_LABEL[B_COUNT] = {"<", ">", "ROT", "DOWN", "DROP"};

static int ctl_bw(void) { return (L.ctl_w - (B_COUNT - 1) * L.ctl_gap) / B_COUNT; }

static void frame(int x, int y, int w, int h, int t)
{
	gfx_rect(x, y, w, t, 0);
	gfx_rect(x, y + h - t, w, t, 0);
	gfx_rect(x, y, t, h, 0);
	gfx_rect(x + w - t, y, t, h, 0);
}

static void draw_button(int x, int y, int w, int h, const char* label)
{
	gfx_rect(x, y, w, h, 255);
	frame(x, y, w, h, 4);
	int len = (int) strlen(label);
	int sc  = imin(h / 10, (w * 8 / 10) / (len * 6 - 1));
	if (len == 1)
		sc = imin(h / 6, w / 6);
	if (sc < 1)
		sc = 1;
	draw_text(x + (w - text_w(label, sc)) / 2, y + (h - 7 * sc) / 2, sc, label, 0);
}

static void draw_controls(void)
{
	int bw = ctl_bw();
	for (int i = 0; i < B_COUNT; i++)
		draw_button(L.ctl_x + i * (bw + L.ctl_gap), L.ctl_y, bw, L.ctl_h, B_LABEL[i]);
}

/* ----------------------------------------------------------- board draw */
enum { S_EMPTY = 0, S_LOCKED, S_ACTIVE, S_GHOST };

static uint8_t shown[T_H][T_W];
static bool    shown_valid;

static void draw_cell(int cx, int cy, int state)
{
	int x = L.bx + cx * L.cs, y = L.by + cy * L.cs, s = L.cs;
	gfx_rect(x, y, s, s, 255);
	switch (state) {
		case S_LOCKED:
			gfx_rect(x + 2, y + 2, s - 4, s - 4, 0);
			break;
		case S_ACTIVE: /* solid with a white core: easy to tell from locked */
			gfx_rect(x + 2, y + 2, s - 4, s - 4, 0);
			gfx_rect(x + s / 3, y + s / 3, s - 2 * (s / 3), s - 2 * (s / 3), 255);
			break;
		case S_GHOST:
			frame(x + 3, y + 3, s - 6, s - 6, 2);
			break;
		default:
			break;
	}
}

static void compute_view(const Game* g, uint8_t out[T_H][T_W])
{
	for (int y = 0; y < T_H; y++)
		for (int x = 0; x < T_W; x++)
			out[y][x] = g->board[y][x] ? S_LOCKED : S_EMPTY;
	if (g->over)
		return;
	Cell c[4];
	t_cells(g->type, g->rot, c);
	int gy = t_ghost_y(g);
	for (int i = 0; i < 4; i++) {
		int x = g->x + c[i].x, y = gy + c[i].y;
		if (y >= 0 && out[y][x] == S_EMPTY)
			out[y][x] = S_GHOST;
	}
	for (int i = 0; i < 4; i++) {
		int x = g->x + c[i].x, y = g->y + c[i].y;
		if (y >= 0)
			out[y][x] = S_ACTIVE;
	}
}

typedef struct {
	int  x0, y0, x1, y1;
	bool any;
} Dirty;

static void dirty_add(Dirty* d, int x, int y, int w, int h)
{
	if (!d->any) {
		d->x0 = x;
		d->y0 = y;
		d->x1 = x + w;
		d->y1 = y + h;
		d->any = true;
		return;
	}
	d->x0 = imin(d->x0, x);
	d->y0 = imin(d->y0, y);
	d->x1 = imax(d->x1, x + w);
	d->y1 = imax(d->y1, y + h);
}

static void draw_board(const Game* g, Dirty* d)
{
	uint8_t v[T_H][T_W];
	compute_view(g, v);
	if (!shown_valid) {
		gfx_rect(L.bx - 6, L.by - 6, L.bw + 12, L.bh + 12, 255);
		frame(L.bx - 6, L.by - 6, L.bw + 12, L.bh + 12, 4);
		for (int y = 0; y < T_H; y++)
			for (int x = 0; x < T_W; x++)
				draw_cell(x, y, v[y][x]);
		memcpy(shown, v, sizeof shown);
		shown_valid = true;
		dirty_add(d, L.bx - 6, L.by - 6, L.bw + 12, L.bh + 12);
		return;
	}
	for (int y = 0; y < T_H; y++)
		for (int x = 0; x < T_W; x++)
			if (shown[y][x] != v[y][x]) {
				draw_cell(x, y, v[y][x]);
				shown[y][x] = v[y][x];
				dirty_add(d, L.bx + x * L.cs, L.by + y * L.cs, L.cs, L.cs);
			}
}

/* ------------------------------------------------------------- panel */
static int  last_score = -1, last_lines = -1, last_level = -1, last_best = -1, last_next = -1;
static bool panel_valid;

static void draw_stat(int y, const char* label, int value)
{
	int s_lbl = imax(2, L.pw / 90), s_val = imax(3, L.pw / 55);
	char buf[16];
	snprintf(buf, sizeof buf, "%d", value);
	gfx_rect(L.px, y, L.pw, 7 * s_lbl + 7 * s_val + 4, 255);
	draw_text(L.px, y, s_lbl, label, 0);
	draw_text(L.px, y + 7 * s_lbl + 4, s_val, buf, 0);
}

static void draw_next(int type)
{
	int box = L.next_box, pcs = box / 5;
	gfx_rect(L.px, L.next_y, L.pw, 7 * imax(2, L.pw / 90) + 4 + box, 255);
	int s_lbl = imax(2, L.pw / 90);
	draw_text(L.px, L.next_y, s_lbl, "NEXT", 0);
	int oy = L.next_y + 7 * s_lbl + 4;
	frame(L.px, oy, box, box, 3);
	Cell c[4];
	t_cells(type, 0, c);
	int n  = T_SHAPES[type].n;
	int ox = L.px + (box - n * pcs) / 2, py = oy + (box - n * pcs) / 2;
	for (int i = 0; i < 4; i++)
		gfx_rect(ox + c[i].x * pcs + 1, py + c[i].y * pcs + 1, pcs - 2, pcs - 2, 0);
}

static void draw_panel(const Game* g, int best, bool paused, Dirty* d)
{
	if (!panel_valid) {
		gfx_rect(L.px, 0, SW - L.px, L.ctl_y - 1, 255);
		draw_text(L.px, L.title_y, imax(4, L.pw / 44), "TETRIS", 0);
		draw_button(L.pause_x, L.pause_y, L.pause_w, L.pause_h, "PAUSE");
		draw_button(L.quit_x, L.quit_y, L.quit_w, L.quit_h, "QUIT");
		last_score = last_lines = last_level = last_best = last_next = -1;
		dirty_add(d, L.px, 0, SW - L.px, L.ctl_y - 1);
		panel_valid = true;
	}
	int s_lbl = imax(2, L.pw / 90), s_val = imax(3, L.pw / 55);
	int sh_ = 7 * s_lbl + 7 * s_val + 4;
	if (g->score != last_score) {
		draw_stat(L.score_y, "SCORE", g->score);
		last_score = g->score;
		dirty_add(d, L.px, L.score_y, L.pw, sh_);
	}
	if (g->lines != last_lines) {
		draw_stat(L.lines_y, "LINES", g->lines);
		last_lines = g->lines;
		dirty_add(d, L.px, L.lines_y, L.pw, sh_);
	}
	if (g->level != last_level) {
		draw_stat(L.level_y, "LEVEL", g->level);
		last_level = g->level;
		dirty_add(d, L.px, L.level_y, L.pw, sh_);
	}
	if (best != last_best) {
		draw_stat(L.best_y, "BEST", best);
		last_best = best;
		dirty_add(d, L.px, L.best_y, L.pw, sh_);
	}
	if (g->next != last_next) {
		draw_next(g->next);
		last_next = g->next;
		dirty_add(d, L.px, L.next_y, L.pw, 7 * s_lbl + 4 + L.next_box);
	}
	static bool was_paused;
	if (paused != was_paused) {
		draw_button(L.pause_x, L.pause_y, L.pause_w, L.pause_h, paused ? "RESUME" : "PAUSE");
		was_paused = paused;
		dirty_add(d, L.pause_x, L.pause_y, L.pause_w, L.pause_h);
	}
}

static void draw_banner(const char* l1, const char* l2, Dirty* d)
{
	int sc1 = imin(L.bw / 40, 9), sc2 = imax(2, sc1 / 2);
	int w   = imax(text_w(l1, sc1), text_w(l2, sc2)) + 60;
	int h   = 7 * sc1 + 7 * sc2 + 90;
	int x   = L.bx + (L.bw - w) / 2, y = L.by + (L.bh - h) / 2;
	gfx_rect(x, y, w, h, 255);
	frame(x, y, w, h, 6);
	draw_text(x + (w - text_w(l1, sc1)) / 2, y + 30, sc1, l1, 0);
	draw_text(x + (w - text_w(l2, sc2)) / 2, y + 60 + 7 * sc1, sc2, l2, 0);
	dirty_add(d, x, y, w, h);
	shown_valid = false; /* banner overwrote cells: force a full board redraw later */
}

/* Redraw everything, refresh with a flashing full-screen waveform. */
static void draw_everything(const Game* g, int best, bool paused)
{
	gfx_rect(0, 0, SW, SH, 255);
	shown_valid = false;
	panel_valid = false;
	Dirty d     = {0};
	draw_board(g, &d);
	draw_panel(g, best, paused, &d);
	draw_controls();
	gfx_flush(0, 0, SW, SH, true);
}

/* =========================================================== platforms */
#ifndef TETRIS_MOCK

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "fbink.h"

static int         fbfd = -1;
static FBInkConfig cfg;
static FBInkState  st;

static int gfx_rect(int x, int y, int w, int h, uint8_t gray)
{
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (x + w > SW)
		w = SW - x;
	if (y + h > SH)
		h = SH - y;
	if (w <= 0 || h <= 0)
		return 0;
	FBInkRect r = {(unsigned short) x, (unsigned short) y, (unsigned short) w, (unsigned short) h};
	/* NOTE: field order is left, top, width, height */
	cfg.no_refresh = true;
	return fbink_fill_rect_gray(fbfd, &cfg, &r, false, gray);
}

static void gfx_flush(int x, int y, int w, int h, bool full)
{
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (x + w > SW)
		w = SW - x;
	if (y + h > SH)
		h = SH - y;
	if (w <= 0 || h <= 0)
		return;
	cfg.no_refresh  = false;
	cfg.is_flashing = full;
	cfg.wfm_mode    = full ? WFM_GC16 : WFM_DU;
	fbink_refresh(fbfd, (uint32_t) y, (uint32_t) x, (uint32_t) w, (uint32_t) h, &cfg);
	if (full)
		fbink_wait_for_complete(fbfd, LAST_MARKER);
	cfg.is_flashing = false;
	cfg.no_refresh  = true;
}

static long long now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* ------------------------------------------------------------- input */
/* Kindle kernels are 32-bit: struct input_event is 16 bytes. Do not use libc's. */
struct kev {
	uint32_t sec, usec;
	uint16_t type, code;
	int32_t  value;
};

#define EV_SYN 0
#define EV_KEY 1
#define EV_ABS 3
#define ABS_X 0
#define ABS_Y 1
#define ABS_MT_POSITION_X 53
#define ABS_MT_POSITION_Y 54
#define ABS_MT_TRACKING_ID 57
#define BTN_TOUCH 330

struct absinfo {
	int32_t value, minimum, maximum, fuzz, flat, resolution;
};
#define EVIOCGABS_(abs) _IOR('E', 0x40 + (abs), struct absinfo)

typedef struct {
	int  fd;
	bool touch;
	int  min[2], max[2]; /* raw ranges, [0]=x [1]=y */
	int  raw[2];
	bool have[2], down;
} Dev;

#define MAX_DEV 8
static Dev  devs[MAX_DEV];
static int  ndev;
static bool tr_swap, tr_mirx, tr_miry, tr_valid;

typedef enum {
	A_NONE,
	A_LEFT,
	A_RIGHT,
	A_ROT,
	A_DOWN,
	A_DROP,
	A_PAUSE,
	A_QUIT,
	A_TAP, /* generic tap, used by game-over restart */
} Act;

typedef struct {
	bool   is_tap;
	double u, v; /* normalised raw touch, 0..1 */
	Act    key;  /* from a hardware key */
} Evt;

static void input_open(void)
{
	size_t              n   = 0;
	FBInkInputDevice* list = fbink_input_scan(INPUT_TOUCHSCREEN | INPUT_KEY | INPUT_PAGINATION_BUTTONS, 0,
						  NO_RECAP, &n);
	for (size_t i = 0; list && i < n && ndev < MAX_DEV; i++) {
		if (!list[i].matched || list[i].fd < 0)
			continue;
		Dev* d = &devs[ndev];
		memset(d, 0, sizeof *d);
		d->fd    = list[i].fd;
		d->touch = (list[i].type & (INPUT_TOUCHSCREEN | INPUT_TABLET)) != 0;
		if (d->touch) {
			struct absinfo ax = {0}, ay = {0};
			int            cx = ABS_MT_POSITION_X, cy = ABS_MT_POSITION_Y;
			if (ioctl(d->fd, EVIOCGABS_(cx), &ax) < 0 || ax.maximum <= ax.minimum) {
				cx = ABS_X;
				cy = ABS_Y;
				ioctl(d->fd, EVIOCGABS_(cx), &ax);
			}
			ioctl(d->fd, EVIOCGABS_(cy), &ay);
			d->min[0] = ax.minimum;
			d->max[0] = ax.maximum;
			d->min[1] = ay.minimum;
			d->max[1] = ay.maximum;
			if (d->max[0] <= d->min[0] || d->max[1] <= d->min[1])
				d->touch = false;
		}
		ndev++;
	}
	/* free the list shell but keep the fds open */
	free(list);
}

static bool have_touch(void)
{
	for (int i = 0; i < ndev; i++)
		if (devs[i].touch)
			return true;
	return false;
}

static Act key_to_act(int code)
{
	switch (code) {
		case 109: /* KEY_PAGEDOWN */
		case 194: /* right/forward page button on Oasis */
			return A_ROT;
		case 104: /* KEY_PAGEUP */
		case 193:
			return A_DROP;
		default:
			return A_NONE;
	}
}

/* Reads pending events from one device, appending to out. */
static int dev_read(Dev* d, Evt* out, int max)
{
	struct kev buf[64];
	int        cnt = 0;
	ssize_t    n   = read(d->fd, buf, sizeof buf);
	if (n <= 0)
		return 0;
	size_t ne = (size_t) n / sizeof(struct kev);
	for (size_t i = 0; i < ne && cnt < max; i++) {
		struct kev* e = &buf[i];
		if (e->type == EV_KEY && d->touch && e->code == BTN_TOUCH) {
			if (e->value == 1)
				d->down = true;
			else if (e->value == 0 && d->down) {
				d->down = false;
				goto release;
			}
		} else if (e->type == EV_KEY && e->value == 1) {
			Act a = key_to_act(e->code);
			if (a != A_NONE) {
				out[cnt].is_tap = false;
				out[cnt].key    = a;
				cnt++;
			}
		} else if (e->type == EV_ABS && d->touch) {
			if (e->code == ABS_MT_POSITION_X || e->code == ABS_X) {
				d->raw[0]  = e->value;
				d->have[0] = true;
			} else if (e->code == ABS_MT_POSITION_Y || e->code == ABS_Y) {
				d->raw[1]  = e->value;
				d->have[1] = true;
			} else if (e->code == ABS_MT_TRACKING_ID) {
				if (e->value >= 0)
					d->down = true;
				else if (d->down) {
					d->down = false;
					goto release;
				}
			}
		}
		continue;
	release:
		if (d->have[0] && d->have[1]) {
			out[cnt].is_tap = true;
			out[cnt].key    = A_NONE;
			out[cnt].u      = (double) (d->raw[0] - d->min[0]) / (double) (d->max[0] - d->min[0]);
			out[cnt].v      = (double) (d->raw[1] - d->min[1]) / (double) (d->max[1] - d->min[1]);
			cnt++;
		}
		d->have[0] = d->have[1] = false;
	}
	return cnt;
}

/* Wait up to timeout_ms (-1 = forever) for events. Returns count. */
static int input_wait(Evt* out, int max, int timeout_ms)
{
	struct pollfd p[MAX_DEV];
	for (int i = 0; i < ndev; i++) {
		p[i].fd     = devs[i].fd;
		p[i].events = POLLIN;
	}
	int r = poll(p, (nfds_t) ndev, timeout_ms);
	if (r <= 0)
		return 0;
	int cnt = 0;
	for (int i = 0; i < ndev && cnt < max; i++)
		if (p[i].revents & POLLIN)
			cnt += dev_read(&devs[i], out + cnt, max - cnt);
	return cnt;
}

static void to_screen(double u, double v, int* x, int* y)
{
	if (tr_swap) {
		double t = u;
		u        = v;
		v        = t;
	}
	if (tr_mirx)
		u = 1.0 - u;
	if (tr_miry)
		v = 1.0 - v;
	*x = (int) (u * SW);
	*y = (int) (v * SH);
}

/* Map a tap to an action. */
static Act hit_test(int x, int y)
{
	if (y >= L.ctl_y - L.ctl_gap) {
		int bw = ctl_bw();
		int i  = (x - L.ctl_x) / (bw + L.ctl_gap);
		if (i < 0)
			i = 0;
		if (i >= B_COUNT)
			i = B_COUNT - 1;
		static const Act m[B_COUNT] = {A_LEFT, A_RIGHT, A_ROT, A_DOWN, A_DROP};
		return m[i];
	}
	if (x >= L.px) {
		if (y >= L.quit_y && y < L.quit_y + L.quit_h)
			return A_QUIT;
		if (y >= L.pause_y && y < L.pause_y + L.pause_h)
			return A_PAUSE;
	}
	return A_TAP;
}

/* ------------------------------------------------------- calibration */
static void conf_path(char* out, size_t n, const char* name) { snprintf(out, n, "%s/%s", CONF_DIR, name); }

static bool calib_load(void)
{
	char p[256];
	conf_path(p, sizeof p, "calib.conf");
	FILE* f = fopen(p, "r");
	if (!f)
		return false;
	int s = 0, mx = 0, my = 0;
	int ok = fscanf(f, "swap=%d mirx=%d miry=%d", &s, &mx, &my);
	fclose(f);
	if (ok != 3)
		return false;
	tr_swap  = s;
	tr_mirx  = mx;
	tr_miry  = my;
	tr_valid = true;
	return true;
}

static void calib_save(void)
{
	char p[256];
	conf_path(p, sizeof p, "calib.conf");
	FILE* f = fopen(p, "w");
	if (!f)
		return;
	fprintf(f, "swap=%d mirx=%d miry=%d\n", tr_swap, tr_mirx, tr_miry);
	fclose(f);
}

/* Wait for one raw tap. Returns false if a hardware key was pressed (skip). */
static bool wait_raw_tap(double* u, double* v)
{
	Evt ev[16];
	for (;;) {
		int n = input_wait(ev, 16, -1);
		for (int i = 0; i < n; i++) {
			if (ev[i].is_tap) {
				*u = ev[i].u;
				*v = ev[i].v;
				return true;
			}
			if (ev[i].key != A_NONE)
				return false;
		}
	}
}

static void calib_target(int cx, int cy, const char* msg)
{
	gfx_rect(0, 0, SW, SH, 255);
	int sz = SW / 8;
	gfx_rect(cx - sz / 2, cy - sz / 2, sz, sz, 0);
	gfx_rect(cx - sz / 4, cy - sz / 4, sz / 2, sz / 2, 255);
	int sc = imax(3, SW / 130);
	draw_text((SW - text_w(msg, sc)) / 2, SH / 2, sc, msg, 0);
	draw_text((SW - text_w("PAGE BUTTON SKIPS", sc)) / 2, SH / 2 + 12 * sc, sc, "PAGE BUTTON SKIPS", 0);
	gfx_flush(0, 0, SW, SH, true);
}

static bool calibrate(void)
{
	/* tap three known points: top-left, top-right, bottom-left */
	int    px[3] = {SW / 6, SW * 5 / 6, SW / 6};
	int    py[3] = {SH / 6, SH / 6, SH * 5 / 6};
	double u[3], v[3];
	for (int i = 0; i < 3; i++) {
		calib_target(px[i], py[i], "TAP THE SQUARE");
		if (!wait_raw_tap(&u[i], &v[i]))
			return false;
	}
	double dx1 = u[1] - u[0], dy1 = v[1] - v[0]; /* screen +x */
	double dx2 = u[2] - u[0], dy2 = v[2] - v[0]; /* screen +y */
	double a1  = dx1 < 0 ? -dx1 : dx1, b1 = dy1 < 0 ? -dy1 : dy1;
	if (a1 < 0.2 && b1 < 0.2)
		return false; /* taps too close together: bad input, retry */
	if (a1 >= b1) {
		tr_swap = false;
		tr_mirx = dx1 < 0;
		tr_miry = dy2 < 0;
	} else {
		tr_swap = true;
		tr_mirx = dy1 < 0;
		tr_miry = dx2 < 0;
	}
	tr_valid = true;
	calib_save();
	return true;
}

/* ------------------------------------------------------------- score */
static int best_load(void)
{
	char p[256];
	conf_path(p, sizeof p, "highscore");
	FILE* f = fopen(p, "r");
	int   b = 0;
	if (f) {
		if (fscanf(f, "%d", &b) != 1)
			b = 0;
		fclose(f);
	}
	return b;
}

static void best_save(int b)
{
	char p[256];
	conf_path(p, sizeof p, "highscore");
	FILE* f = fopen(p, "w");
	if (f) {
		fprintf(f, "%d\n", b);
		fclose(f);
	}
}

/* -------------------------------------------------------------- main */
static volatile sig_atomic_t stop_flag;
static void                  on_sig(int s)
{
	(void) s;
	stop_flag = 1;
}

int main(int argc, char** argv)
{
	bool force_cal = false;
	for (int i = 1; i < argc; i++)
		if (!strcmp(argv[i], "--calibrate"))
			force_cal = true;

	signal(SIGTERM, on_sig);
	signal(SIGINT, on_sig);

	cfg.is_quiet = true;
	fbfd         = fbink_open();
	if (fbfd < 0) {
		fprintf(stderr, "tetris: cannot open framebuffer\n");
		return 1;
	}
	if (fbink_init(fbfd, &cfg) < 0) {
		fprintf(stderr, "tetris: fbink_init failed\n");
		return 1;
	}
	fbink_get_state(&cfg, &st);
	SW = (int) st.view_width;
	SH = (int) st.view_height;
	fprintf(stderr, "tetris: %s %dx%d rota=%d touch swap=%d mx=%d my=%d\n", st.device_name, SW, SH,
		st.current_rota, st.touch_swap_axes, st.touch_mirror_x, st.touch_mirror_y);
	layout_init();
	input_open();

	/* default transform from FBInk's device knowledge; calibration overrides */
	tr_swap = st.touch_swap_axes;
	tr_mirx = st.touch_mirror_x;
	tr_miry = st.touch_mirror_y;
	if (have_touch() && (force_cal || !calib_load())) {
		bool ok = false;
		for (int tries = 0; tries < 3 && !ok; tries++)
			ok = calibrate();
		(void) ok;
	}

	int  best  = best_load();
	Game g;
	t_new(&g, (uint32_t) now_ms() ^ (uint32_t) getpid() * 2654435761u);
	bool paused = false, need_full = true, quit_armed = false;
	long long quit_deadline = 0, next_tick = now_ms() + t_gravity_ms(g.level);
	int  locks = 0, prev_lines = 0;
	bool was_over = false;

	while (!stop_flag) {
		if (need_full) {
			draw_everything(&g, best, paused);
			need_full = false;
			if (g.over) {
				Dirty d = {0};
				draw_banner("GAME OVER", "TAP TO RESTART", &d);
				gfx_flush(d.x0, d.y0, d.x1 - d.x0, d.y1 - d.y0, false);
			}
		}

		long long now = now_ms();
		int       to  = -1;
		if (!paused && !g.over)
			to = (int) imax(0, (int) (next_tick - now));
		if (quit_armed) {
			int qt = (int) imax(0, (int) (quit_deadline - now));
			to     = to < 0 ? qt : imin(to, qt);
		}

		Evt ev[32];
		int n = input_wait(ev, 32, to);

		bool changed = false;
		bool locked  = false;
		for (int i = 0; i < n; i++) {
			Act a = A_NONE;
			if (ev[i].is_tap) {
				int x, y;
				to_screen(ev[i].u, ev[i].v, &x, &y);
				a = hit_test(x, y);
			} else {
				a = ev[i].key;
			}
			if (a == A_QUIT) {
				if (quit_armed) {
					stop_flag = 1;
					break;
				}
				quit_armed    = true;
				quit_deadline = now_ms() + 3000;
				draw_button(L.quit_x, L.quit_y, L.quit_w, L.quit_h, "SURE?");
				gfx_flush(L.quit_x, L.quit_y, L.quit_w, L.quit_h, false);
				continue;
			}
			if (quit_armed && a != A_NONE) {
				quit_armed = false;
				draw_button(L.quit_x, L.quit_y, L.quit_w, L.quit_h, "QUIT");
				gfx_flush(L.quit_x, L.quit_y, L.quit_w, L.quit_h, false);
			}
			if (g.over) {
				if (a != A_NONE) { /* any tap/key restarts */
					t_new(&g, (uint32_t) now_ms() ^ 0x5bd1e995u);
					need_full  = true;
					prev_lines = 0;
					next_tick  = now_ms() + t_gravity_ms(g.level);
				}
				continue;
			}
			if (a == A_PAUSE) {
				paused = !paused;
				changed = true;
				next_tick = now_ms() + t_gravity_ms(g.level);
				continue;
			}
			if (paused)
				continue;
			switch (a) {
				case A_LEFT:
					changed |= t_move(&g, -1);
					break;
				case A_RIGHT:
					changed |= t_move(&g, +1);
					break;
				case A_ROT:
					changed |= t_rotate(&g, +1);
					break;
				case A_DOWN:
					if (!t_step(&g, true))
						locked = true;
					changed        = true;
					next_tick      = now_ms() + t_gravity_ms(g.level);
					break;
				case A_DROP:
					t_hard_drop(&g);
					locked    = true;
					changed   = true;
					next_tick = now_ms() + t_gravity_ms(g.level);
					break;
				default:
					break;
			}
		}
		if (stop_flag)
			break;

		if (quit_armed && now_ms() >= quit_deadline) {
			quit_armed = false;
			draw_button(L.quit_x, L.quit_y, L.quit_w, L.quit_h, "QUIT");
			gfx_flush(L.quit_x, L.quit_y, L.quit_w, L.quit_h, false);
		}

		if (!paused && !g.over && now_ms() >= next_tick) {
			if (!t_step(&g, false))
				locked = true;
			changed   = true;
			next_tick = now_ms() + t_gravity_ms(g.level);
		}

		if (locked) {
			locks++;
			if (g.lines != prev_lines || locks % 12 == 0)
				need_full = true; /* line clears / periodic ghost cleanup */
			prev_lines = g.lines;
		}
		if (g.over && !was_over) {
			if (g.score > best) {
				best = g.score;
				best_save(best);
			}
			need_full = true; /* redraw once, then show the banner */
		}
		was_over = g.over;

		if (changed && !need_full) {
			Dirty d = {0};
			draw_board(&g, &d);
			draw_panel(&g, best, paused, &d);
			if (d.any)
				gfx_flush(d.x0, d.y0, d.x1 - d.x0, d.y1 - d.y0, false);
		}
	}

	/* leave a clean white screen for the framework to repaint over */
	gfx_rect(0, 0, SW, SH, 255);
	gfx_flush(0, 0, SW, SH, true);
	fbink_close(fbfd);
	return 0;
}

#endif /* !TETRIS_MOCK */
