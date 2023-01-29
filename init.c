#include "extern.h"

#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <X11/extensions/Xinerama.h>

static void           init_colors  (Display *, struct layout *);
static void           set_color    (Display *, const char *, const char *, GC *);
static char          *get_option   (Display *, const char *);
static struct column *init_columns (Display *, int n, struct layout *);
static GC create_outline_gc(struct layout *l);

static char*
get_option(Display *display, const char *keyword)
{
	char *option;

	option = XGetDefault(display, "cocowm", keyword);
#ifndef NDEBUG
	if (option == NULL)
		TRACE("NULL option, trying some defaults");
#endif

	/* TODO: Replace this default handling with something better */
	if (option == NULL && strcmp(keyword, "font") == 0)
		option = "fixed";

	TRACE("option: %s", option);

	return option;
}

void init(Display *display, struct layout *layout, int columns)
{
	XGCValues xgc;

	if (ScreenCount(display) > 1)
		warnx("no support for %d legacy screen(s), "
		      "using screen %d, "
		      "please use RandR for multi-monitor support",
		      1 - ScreenCount(display),
		      DefaultScreen(display));

	/* TODO: Use RandR because we can get events and more info */
	if (XineramaIsActive(display) == True) {
		int i, n;
		XineramaScreenInfo *xsi;

		xsi = XineramaQueryScreens(display, &n);
		for (i = 0; i < n; i++) {
			TRACE("screen %d: %d,%d %d,%d", i,
			       xsi[i].x_org, xsi[i].y_org,
			       xsi[i].width, xsi[i].height);
		}
	}

	xgc.line_width = 1;

	layout->ftfont = font_load(display, get_option(display, "font"));

	set_ftcolor(display, &layout->text_fg, 0);
	set_ftcolor(display, &layout->text_active_bg, 1);
	set_ftcolor(display, &layout->text_inactive_bg, 2);
	set_ftcolor(display, &layout->text_cursor, 3);


	layout->font_height_px = layout->ftfont->height;
	layout->font_width_px = layout->ftfont->max_advance_width;
	layout->titlebar_height_px = layout->font_height_px + 2;

	init_colors(display, layout);

	XChangeGC(display, layout->focus_gc,
	          GCLineWidth, &xgc);
	XChangeGC(display, layout->normal_gc,
	          GCLineWidth, &xgc);

	layout->display = display;
	layout->outline_gc = create_outline_gc(layout);

	layout->head = init_columns(display, columns, layout);
	layout->column = layout->head;
}

static GC create_outline_gc(struct layout *l)
{
	XGCValues v;

	v.function = GXxor;
	v.subwindow_mode = IncludeInferiors;
	v.foreground = WhitePixel(l->display, DefaultScreen(l->display));

	return XCreateGC(l->display, DefaultRootWindow(l->display),
	                 GCFunction | GCSubwindowMode | GCForeground, &v);
}

static struct column *
init_columns(Display *display, int n, struct layout *l)
{
	struct column *head, *prev, *column;
	int hspacing, rwidth, equal;
	int x;

	hspacing = l->font_width_px;

	head = prev = NULL;
	x = hspacing;

	rwidth = region_width(display, x);
	rwidth -= hspacing * (n+1);
	equal = rwidth / n;

	TRACE("init columns");

	while (n--) {
		if ((column = calloc(1, sizeof(struct column))) == NULL)
			err(1, "initializing columns");

		TRACE("init column");
		column->x = x;
		column->width = equal;
		column->max_height = region_height(display, column->x);
		column->layout = l;

		/*
		 * Add to tail.
		 */
		if (head == NULL)
			head = column;
		else {
			column->prev = l->tail;
			assert(column->prev != NULL);
			column->prev->next = column;
		}
		l->tail = column;

		x += column->width + hspacing;
	}

	return head;
}

static void init_colors(Display *display, struct layout *layout)
{
#define DARKMODE 0
#if DARKMODE
	set_color(display, "white", "gray20", &(layout->normal_gc));
	set_color(display, "white", "darkgreen", &(layout->focus_gc));
#else
	set_color(display, "black", "gray", &(layout->normal_gc));
	set_color(display, "black", "cornflower blue", &(layout->focus_gc));
#endif

	set_color(display, "white", "gray70", &(layout->bright_gc));
	set_color(display, "black", "gray70", &(layout->shadow_gc));

	set_color(display, "gray20", "gray", &(layout->column_gc));
}

static void
set_color(Display *display, const char *fg, const char *bg, GC *gc)
{
	XColor   fg_color, bg_color, exact;
	Colormap colormap;
	Window   root;

	root = DefaultRootWindow(display);
	colormap = DefaultColormap(display, DefaultScreen(display));

	XAllocNamedColor(display, colormap, fg, &fg_color, &exact);
	XAllocNamedColor(display, colormap, bg, &bg_color, &exact);

	*gc = XCreateGC(display, root, 0, NULL);

	XSetForeground(display, *gc, fg_color.pixel);
	XSetBackground(display, *gc, bg_color.pixel);
}
