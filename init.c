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

	layout->fs = XLoadQueryFont(display,
	                     get_option(display, "font"));
	xgc.font = layout->fs->fid;
	xgc.line_width = 1;

	layout->font_width_px =
	    layout->fs->max_bounds.rbearing -
	    layout->fs->min_bounds.lbearing;
	layout->font_height_px =
	    layout->fs->max_bounds.ascent +
	    layout->fs->max_bounds.descent;
	layout->titlebar_height_px = layout->font_height_px + 2;

	layout->head = init_columns(display, columns, layout);

	layout->column = layout->head;

	init_colors(display, layout);

	XChangeGC(display, layout->focus_gc,
	          GCFont | GCLineWidth, &xgc);
	XChangeGC(display, layout->normal_gc,
	          GCFont | GCLineWidth, &xgc);

	layout->display = display;
	layout->outline_gc = create_outline_gc(layout);
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

	while (n--) {
		if ((column = calloc(1, sizeof(struct column))) == NULL)
			err(1, "initializing columns");

		column->x = x;
		column->width = equal;
		x += column->width + hspacing;

		column->max_height = region_height(display, column->x);
		column->layout = l;
		column->next = head;

		if (column->next == NULL)
			l->tail = column;

		head = column;
		if (head->next != NULL)
			head->next->prev = head;
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
