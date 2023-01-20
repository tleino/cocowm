#include "extern.h"

#include <string.h>

static void draw_border(Window, int, int, bool, struct layout *);

/*
 * Draws button in a pane (p) to layout (l).
 */
static void
draw_button(struct pane *p, Window w, char ch, bool reverse, struct layout *l)
{
	XDrawString(l->display, w, l->normal_gc,
	    1 + l->fs->min_bounds.lbearing +
	        ((l->font_height_px - l->font_width_px) / 2),
	    1 + l->fs->ascent, &ch, 1);

	draw_border(w, l->titlebar_height_px, l->titlebar_height_px, reverse, l);

#if 0
	XDrawRectangle(l->display, w, l->normal_gc, 0, 0, 20, 20);
#endif
}

void
draw_close_button(struct pane *p, struct layout *l)
{
	draw_button(p, p->close_button, 'X', false, l);
}

void
draw_maximize_button(struct pane *p, struct layout *l)
{
	bool reverse;

	reverse = false;
	if (p->flags & PF_MAXIMIZED)
		reverse = true;

	draw_button(p, p->maximize_button, '^', reverse, l);
}

void
draw_outline(struct pane *p, int x, int y, struct layout *l)
{
	XSegment s[7] = {
	/* 1. left vertical line */
		{ x, y,
		  x, y + p->adjusted_height },
	/* 2. right vertical line */
		{ x + l->column->width, y,
		  x + l->column->width, y + p->adjusted_height },
	/* 3. top horizontal line */
		{ x + 1, y,
		  x + l->column->width - 1, y },
	/* 4. horizontal titlebar separator */
		{ x + 1, y + l->titlebar_height_px,
		  x + l->column->width - 1, y + l->titlebar_height_px },
	/* 5. bottom horizontal line */
		{ x + 1, y + p->adjusted_height,
		  x + l->column->width - 1, y + p->adjusted_height },
	/* 6. topleft -> bottomright diagonal */
		{ x + 1, y + 21,
		  x + l->column->width - 1, y + p->adjusted_height - 1 },
	/* 7. bottomleft -> topright diagonal */
		{ x + 1, y + p->adjusted_height - 1,
		  x + l->column->width - 1, y + l->titlebar_height_px }
	};

	/*
	 * We use segments here because we want to have a bit more
	 * than a plain rectangle outline. We're assuming using one
	 * larger drawing operation instead of several smaller
	 * drawing operations is better. In any case there will
	 * be less overlapping draws.
	 */
	XDrawSegments(l->display, DefaultRootWindow(l->display),
	              l->outline_gc, s, sizeof(s) / sizeof(s[0]));
}

static void
draw_border(Window w, int width, int height, bool reverse, struct layout *l)
{
	XSegment bright[] = {
	/* 1. left vertical line */
		{ 0, 0,
		  0, height -1 },
	/* 3. top horizontal line */
		{ 0 + 1, 0,
		  0 + width - 1, 0 }
	};

	XSegment shadow[] = {
	/* 2. right vertical line */
		{ 0 + width - 1, 0,
		  0 + width - 1, height },
	/* 4. bottom horizontal line */
		{ 0, height - 1,
		  0 + width - 1, height - 1 }
	};

	XDrawSegments(l->display, w,
	              reverse ? l->shadow_gc : l->bright_gc,
	              bright, sizeof(bright) / sizeof(bright[0]));
	XDrawSegments(l->display, w,
	              reverse ? l->bright_gc : l->shadow_gc,
	              shadow, sizeof(shadow) / sizeof(shadow[0]));
}

/*
 * Draws frame in a pane (p) to layout (l).
 */
void
draw_frame(struct pane *p, struct layout *l)
{
	char number_str[20];

	XClearWindow(l->display, p->frame);

	snprintf(number_str, sizeof(number_str), "%ld: ", p->number);

	XDrawString(l->display, p->frame, l->normal_gc,
	    1 + l->fs->min_bounds.lbearing, 1 + l->fs->ascent, number_str,
	                 strlen(number_str));

	if (p->name != NULL && (p->flags & PF_MINIMIZED) == 0)
		XDrawString(l->display, p->frame, l->normal_gc,
		            strlen(number_str) * l->font_width_px +
		            l->fs->min_bounds.lbearing + 1, 1 + l->fs->ascent,
			    p->name,
		            strlen(p->name));
	else if (p->icon_name != NULL && (p->flags & PF_MINIMIZED))
		XDrawString(l->display, p->frame, l->normal_gc,
		            strlen(number_str) * 8 + 16, 15, p->icon_name,
		            strlen(p->icon_name));

	if (p->flags & PF_FULLSCREEN)
		draw_border(p->frame, 1920, l->titlebar_height_px, false, l);
	else 
		draw_border(p->frame, l->column->width, l->titlebar_height_px, false, l);
}
