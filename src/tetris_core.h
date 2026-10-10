/* Pure Tetris logic: no I/O, no platform deps. */
#ifndef TETRIS_CORE_H
#define TETRIS_CORE_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define T_W 10
#define T_H 20

typedef struct {
	int8_t x, y;
} Cell;

/* Base orientation of each piece inside an n*n box. Order: I O T S Z J L. */
static const struct {
	int  n;
	Cell c[4];
} T_SHAPES[7] = {
	{4, {{0, 1}, {1, 1}, {2, 1}, {3, 1}}},
	{2, {{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
	{3, {{1, 0}, {0, 1}, {1, 1}, {2, 1}}},
	{3, {{1, 0}, {2, 0}, {0, 1}, {1, 1}}},
	{3, {{0, 0}, {1, 0}, {1, 1}, {2, 1}}},
	{3, {{0, 0}, {0, 1}, {1, 1}, {2, 1}}},
	{3, {{2, 0}, {0, 1}, {1, 1}, {2, 1}}},
};

typedef struct {
	uint8_t  board[T_H][T_W]; /* 0 = empty, else piece type + 1 */
	int      type, rot, x, y; /* active piece; (x,y) = top-left of its box */
	int      next;
	uint8_t  bag[7];
	int      bag_i;
	uint32_t rng;
	int      score, lines, level;
	int      last_cleared; /* rows cleared by the most recent lock */
	bool     over;
} Game;

static inline void t_cells(int type, int rot, Cell out[4])
{
	int n = T_SHAPES[type].n;
	for (int i = 0; i < 4; i++) {
		int x = T_SHAPES[type].c[i].x, y = T_SHAPES[type].c[i].y;
		for (int r = 0; r < (rot & 3); r++) {
			int nx = n - 1 - y;
			int ny = x;
			x      = nx;
			y      = ny;
		}
		out[i].x = (int8_t) x;
		out[i].y = (int8_t) y;
	}
}

static inline uint32_t t_rand(Game* g)
{
	/* xorshift32 */
	uint32_t x = g->rng ? g->rng : 0x9E3779B9u;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	g->rng = x;
	return x;
}

/* 7-bag randomizer */
static inline int t_bag_next(Game* g)
{
	if (g->bag_i >= 7) {
		for (int i = 0; i < 7; i++)
			g->bag[i] = (uint8_t) i;
		for (int i = 6; i > 0; i--) {
			int     j = (int) (t_rand(g) % (uint32_t) (i + 1));
			uint8_t t = g->bag[i];
			g->bag[i] = g->bag[j];
			g->bag[j] = t;
		}
		g->bag_i = 0;
	}
	return g->bag[g->bag_i++];
}

static inline bool t_fits(const Game* g, int type, int rot, int x, int y)
{
	Cell c[4];
	t_cells(type, rot, c);
	for (int i = 0; i < 4; i++) {
		int cx = x + c[i].x, cy = y + c[i].y;
		if (cx < 0 || cx >= T_W || cy >= T_H)
			return false;
		if (cy >= 0 && g->board[cy][cx])
			return false;
	}
	return true;
}

static inline void t_spawn(Game* g)
{
	g->type = g->next;
	g->next = t_bag_next(g);
	g->rot  = 0;
	g->x    = (T_W - T_SHAPES[g->type].n) / 2;
	g->y    = 0;
	if (!t_fits(g, g->type, 0, g->x, g->y))
		g->over = true;
}

static inline void t_new(Game* g, uint32_t seed)
{
	memset(g, 0, sizeof(*g));
	g->rng   = seed ? seed : 1;
	g->bag_i = 7;
	g->level = 1;
	g->next  = t_bag_next(g);
	t_spawn(g);
}

static inline bool t_move(Game* g, int dx)
{
	if (g->over || !t_fits(g, g->type, g->rot, g->x + dx, g->y))
		return false;
	g->x += dx;
	return true;
}

/* dir: +1 clockwise, -1 counter-clockwise. Tries simple wall/floor kicks. */
static inline bool t_rotate(Game* g, int dir)
{
	static const int kx[5] = {0, -1, 1, -2, 2};
	if (g->over)
		return false;
	int nr = (g->rot + dir) & 3;
	for (int dy = 0; dy >= -1; dy--) {
		for (int i = 0; i < 5; i++) {
			if (t_fits(g, g->type, nr, g->x + kx[i], g->y + dy)) {
				g->rot = nr;
				g->x += kx[i];
				g->y += dy;
				return true;
			}
		}
	}
	return false;
}

static inline int t_ghost_y(const Game* g)
{
	int y = g->y;
	while (t_fits(g, g->type, g->rot, g->x, y + 1))
		y++;
	return y;
}

static inline int t_gravity_ms(int level)
{
	int ms = 1000 - (level - 1) * 85;
	return ms < 300 ? 300 : ms; /* e-ink can't keep up with less */
}

static inline int t_lock(Game* g)
{
	Cell c[4];
	t_cells(g->type, g->rot, c);
	for (int i = 0; i < 4; i++) {
		int cx = g->x + c[i].x, cy = g->y + c[i].y;
		if (cy >= 0)
			g->board[cy][cx] = (uint8_t) (g->type + 1);
	}
	int cleared = 0;
	for (int y = T_H - 1; y >= 0;) {
		bool full = true;
		for (int x = 0; x < T_W; x++)
			if (!g->board[y][x]) {
				full = false;
				break;
			}
		if (full) {
			memmove(&g->board[1], &g->board[0], (size_t) y * T_W);
			memset(&g->board[0], 0, T_W);
			cleared++;
		} else {
			y--;
		}
	}
	static const int pts[5] = {0, 100, 300, 500, 800};
	g->score += pts[cleared] * g->level;
	g->lines += cleared;
	g->level        = g->lines / 10 + 1;
	g->last_cleared = cleared;
	t_spawn(g);
	return cleared;
}

/* One gravity step (or soft-drop step when soft=true). Returns true if the
 * piece moved down, false if it locked. */
static inline bool t_step(Game* g, bool soft)
{
	if (g->over)
		return false;
	if (t_fits(g, g->type, g->rot, g->x, g->y + 1)) {
		g->y++;
		if (soft)
			g->score += 1;
		return true;
	}
	t_lock(g);
	return false;
}

static inline int t_hard_drop(Game* g)
{
	if (g->over)
		return 0;
	int rows = 0;
	while (t_fits(g, g->type, g->rot, g->x, g->y + 1)) {
		g->y++;
		rows++;
	}
	g->score += rows * 2;
	t_lock(g);
	return rows;
}

#endif
