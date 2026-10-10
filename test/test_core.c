#include <assert.h>
#include <stdio.h>

#include "../src/tetris_core.h"

static void test_shapes(void)
{
	/* every piece in every rotation has 4 distinct cells inside its box */
	for (int t = 0; t < 7; t++)
		for (int r = 0; r < 4; r++) {
			Cell c[4];
			t_cells(t, r, c);
			int n = T_SHAPES[t].n;
			for (int i = 0; i < 4; i++) {
				assert(c[i].x >= 0 && c[i].x < n && c[i].y >= 0 && c[i].y < n);
				for (int j = i + 1; j < 4; j++)
					assert(c[i].x != c[j].x || c[i].y != c[j].y);
			}
		}
	/* 4 rotations = identity */
	Cell a[4], b[4];
	t_cells(2, 0, a);
	t_cells(2, 4, b);
	assert(memcmp(a, b, sizeof a) == 0);
	/* T pointing up rotates clockwise to pointing right: cell (1,0) -> (2,1) */
	t_cells(2, 1, b);
	int found = 0;
	for (int i = 0; i < 4; i++)
		if (b[i].x == 2 && b[i].y == 1)
			found = 1;
	assert(found);
}

static void test_bag(void)
{
	Game g;
	t_new(&g, 12345);
	int seen[7] = {0};
	/* first piece (g.type) + next came from bag 1; draw rest of bag 1 and check 7-in-7 */
	int cnt[7] = {0};
	cnt[g.type]++;
	cnt[g.next]++;
	for (int i = 0; i < 5; i++)
		cnt[t_bag_next(&g)]++;
	for (int i = 0; i < 7; i++) {
		assert(cnt[i] == 1);
		seen[i] = 1;
	}
	(void) seen;
}

static void test_move_wall(void)
{
	Game g;
	t_new(&g, 7);
	for (int i = 0; i < 20; i++)
		t_move(&g, -1);
	Cell c[4];
	t_cells(g.type, g.rot, c);
	int minx = 99;
	for (int i = 0; i < 4; i++)
		if (g.x + c[i].x < minx)
			minx = g.x + c[i].x;
	assert(minx == 0);
	assert(!t_move(&g, -1));
}

static void test_line_clear(void)
{
	Game g;
	t_new(&g, 3);
	memset(g.board, 0, sizeof g.board);
	/* bottom row full except columns 0..3, then drop an I flat there */
	for (int x = 4; x < T_W; x++)
		g.board[T_H - 1][x] = 1;
	g.board[T_H - 2][9] = 1; /* debris above that must fall one row */
	g.type = 0;              /* I */
	g.rot  = 0;
	g.x    = 0;
	g.y    = 0;
	int rows = t_hard_drop(&g);
	assert(rows > 0);
	assert(g.last_cleared == 1);
	assert(g.lines == 1);
	assert(g.score == rows * 2 + 100);
	assert(g.board[T_H - 1][9] == 1); /* debris fell */
	assert(g.board[T_H - 1][0] == 0);
	assert(!g.over);
}

static void test_tetris_and_level(void)
{
	Game g;
	t_new(&g, 3);
	memset(g.board, 0, sizeof g.board);
	for (int y = T_H - 4; y < T_H; y++)
		for (int x = 0; x < T_W - 1; x++)
			g.board[y][x] = 1;
	g.type = 0;
	g.rot  = 1; /* vertical I */
	Cell c[4];
	t_cells(0, 1, c);
	g.x = T_W - 1 - c[0].x;
	g.y = 0;
	t_hard_drop(&g);
	assert(g.last_cleared == 4);
	assert(g.lines == 4);
	g.lines = 9;
	g.level = 1;
	assert(t_gravity_ms(1) == 1000);
	assert(t_gravity_ms(50) == 300);
}

static void test_game_over(void)
{
	Game g;
	t_new(&g, 99);
	int n = 0;
	while (!g.over && n < 100) {
		t_hard_drop(&g);
		n++;
	}
	assert(g.over);
	assert(n < 100);
	assert(!t_move(&g, 1));
	assert(!t_rotate(&g, 1));
}

static void test_rotate_kick(void)
{
	Game g;
	t_new(&g, 5);
	memset(g.board, 0, sizeof g.board);
	g.type = 0; /* I, vertical against the right wall must still rotate via kick */
	g.rot  = 1;
	Cell c[4];
	t_cells(0, 1, c);
	g.x = T_W - 1 - c[0].x;
	g.y = 5;
	assert(t_fits(&g, g.type, g.rot, g.x, g.y));
	assert(t_rotate(&g, 1));
	assert(t_fits(&g, g.type, g.rot, g.x, g.y));
}

static void test_ghost(void)
{
	Game g;
	t_new(&g, 11);
	memset(g.board, 0, sizeof g.board);
	int gy = t_ghost_y(&g);
	assert(gy > g.y);
	assert(!t_fits(&g, g.type, g.rot, g.x, gy + 1));
}

int main(void)
{
	test_shapes();
	test_bag();
	test_move_wall();
	test_line_clear();
	test_tetris_and_level();
	test_game_over();
	test_rotate_kick();
	test_ghost();
	puts("core: all tests passed");
	return 0;
}
