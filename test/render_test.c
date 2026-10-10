/* Renders game screens into PGM files using a mock framebuffer (no device). */
#define TETRIS_MOCK 1
#include "../src/tetris.c"

static uint8_t* FB;
static int      flushes;

static int gfx_rect(int x, int y, int w, int h, uint8_t gray)
{
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > SW) w = SW - x;
	if (y + h > SH) h = SH - y;
	for (int j = 0; j < h; j++)
		memset(FB + (size_t) (y + j) * SW + x, gray, (size_t) (w > 0 ? w : 0));
	return 0;
}
static void gfx_flush(int x, int y, int w, int h, bool full) { (void) x; (void) y; (void) w; (void) h; (void) full; flushes++; }

static void dump(const char* path)
{
	FILE* f = fopen(path, "wb");
	fprintf(f, "P5\n%d %d\n255\n", SW, SH);
	fwrite(FB, 1, (size_t) SW * SH, f);
	fclose(f);
}

int main(int argc, char** argv)
{
	SW = argc > 1 ? atoi(argv[1]) : 1264;
	SH = argc > 2 ? atoi(argv[2]) : 1680;
	for (size_t i = 0; i < sizeof GLYPHS / sizeof GLYPHS[0]; i++)
		if (strlen(GLYPHS[i].r) != 35) { printf("bad glyph %c\n", GLYPHS[i].c); return 1; }
	FB = malloc((size_t) SW * SH);
	layout_init();
	printf("cs=%d board=%d,%d %dx%d panel x=%d w=%d ctl y=%d h=%d\n", L.cs, L.bx, L.by, L.bw, L.bh, L.px, L.pw, L.ctl_y, L.ctl_h);
	if (L.ctl_y + L.ctl_h > SH || L.px + L.pw > SW || L.by + L.bh > SH || L.ctl_h < 100) { puts("LAYOUT BAD"); return 1; }

	Game g;
	t_new(&g, 42);
	for (int i = 0; i < 5; i++) { t_move(&g, (i * 3 % 7) - 3); t_rotate(&g, 1); t_hard_drop(&g); }
	g.score = 12400; g.lines = 14; g.level = 2;
	t_step(&g, false); t_step(&g, false); t_step(&g, false);
	draw_everything(&g, 15800, false);
	dump(argc > 3 ? argv[3] : "/tmp/claude-0/game.pgm");

	/* delta update then game over banner */
	t_move(&g, 1);
	Dirty d = {0};
	draw_board(&g, &d);
	draw_panel(&g, 15800, true, &d);
	draw_banner("GAME OVER", "TAP TO RESTART", &d);
	dump("/tmp/claude-0/over.pgm");
	return 0;
}
