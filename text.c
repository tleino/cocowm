/*
 * cocowm - Column Commander Window Manager for X11 Window System
 * Copyright (c) 2023, Tommi Leino <namhas@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "extern.h"

#include <err.h>

static XftFont *ftfont;
static int	 space_width;

#define TABWIDTH 8

void
font_extents(Display *dpy, const char *text, size_t len, XGlyphInfo *extents)
{
	XftTextExtentsUtf8(dpy, ftfont,
	    (const FcChar8 *) text, len, extents);
}

#define NUM_COLOR 6

static XColor color[NUM_COLOR];
static int has_color[NUM_COLOR];

static const char *colorname[] = {
	"black",
	"cornflower blue",
	"gray",
	"red"
};

XColor
query_color(Display *dpy, int i)
{
	XColor def, exact;

	assert(i < NUM_COLOR);

	if (has_color[i])
		return color[i];

	if (XAllocNamedColor(dpy, XDefaultColormap(dpy,
	    DefaultScreen(dpy)), colorname[i], &def, &exact) == 0)
		errx(1, "couldn't allocate '%s'", colorname[i]);

	color[i] = def;
	has_color[i] = 1;

	return color[i];
}

void
set_ftcolor(Display *dpy, XftColor *dst, int color)
{
	XColor xcolor;

	xcolor = query_color(dpy, color);

	dst->pixel = xcolor.pixel;
	dst->color.red = xcolor.red;
	dst->color.green = xcolor.green;
	dst->color.blue = xcolor.blue;
	dst->color.alpha = USHRT_MAX;
}

#ifndef DEFAULT_FONT
#define DEFAULT_FONT "DejaVu Sans Mono:size=12.0"
#endif

#ifndef FALLBACK_FONT
#define FALLBACK_FONT "-misc-fixed-medium-r-normal--20-200-75-75-c-100-iso10646-1"
#endif

XftFont *
font_load(Display *dpy, char *fontname)
{
	XGlyphInfo extents;

	if (fontname == NULL || fontname[0] == '\0')
		fontname = DEFAULT_FONT;

	/*
	 * First try Xlfd form, then Xft font name form.
	 */
	ftfont = XftFontOpenXlfd(dpy, DefaultScreen(dpy), fontname);
	if (ftfont == NULL)
		ftfont = XftFontOpenName(dpy, DefaultScreen(dpy),
		    fontname);

	/*
	 * No success? Try the fallback font.
	 */
	if (ftfont == NULL && strcmp(fontname, FALLBACK_FONT) != 0) {
		warnx("couldn't load font: %s", fontname);
		return font_load(dpy, FALLBACK_FONT);
	} else if (ftfont == NULL)
		errx(1, "couldn't load fallback font: %s", fontname);

	warnx("Font: %s", fontname);

	font_extents(dpy, " ", 1, &extents);
	space_width = extents.xOff;

	warnx("Spacewidth: %d", extents.xOff);

	return ftfont;
}

void
font_clear(XftDraw *ftdraw, Display *dpy, Window window, XftColor bg, int x, int y, int width)
{
	XftDrawRect(ftdraw, &bg, x, y, width, ftfont->height);
}

static int
_font_draw(XftDraw *ftdraw, Display *dpy, Window window, XftColor fg, XftColor bg, int x, int y,
    const char *text, size_t len)
{
	XGlyphInfo extents;

	font_extents(dpy, text, len, &extents);

	XftDrawRect(ftdraw, &bg, x, y, extents.xOff,
	    ftfont->height);

	XftDrawStringUtf8(ftdraw, &fg, ftfont, x,
	    y + ftfont->ascent, (const FcChar8 *) text, len);

	return extents.xOff;
}

#include <ctype.h>

int
font_draw(XftDraw *ftdraw, Display *dpy, Window window, XftColor fg, XftColor bg, int x, int sx, int y, const char *text, size_t len)
{
	size_t i, j;
	int x_out, tabstop, tabwidth, remaining;

	x_out = 0;
	j = 0;
	for (i = 0; i < len; i++) {
		if (text[i] == '\t') {
			if (j!=i)
				x_out += _font_draw(ftdraw, dpy, window, fg, bg, sx+x_out, y,
				    &text[j], i-j);

			j=i+1;

			tabwidth = space_width * TABWIDTH;
			tabstop = ((x+x_out) / tabwidth);
			remaining = tabwidth - ((x+x_out) -
			    (tabstop * tabwidth));
			font_clear(ftdraw, dpy, window, bg, sx+x_out, y, remaining);
			x_out += remaining;
		}
	}
	if (j < len)
		x_out += _font_draw(ftdraw, dpy, window, fg, bg, sx+x_out, y, &text[j], i-j);
	return x_out;
}
