#include "extern.h"

#include <err.h>
#include <X11/Xresource.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <X11/extensions/Xinerama.h>


struct column *
find_column(struct column *head, int x)
{
	struct column *column;
	int hspacing = head->layout->hspacing;

	for (column = head; column != NULL; column = column->next)
		if (x >= column->x && x - column->x < column->width + hspacing)
			return column;

	return head;
}

int
region(Display *display, int x)
{
	int i, n;
	XineramaScreenInfo *xsi;

	/* TODO: Use RandR because we can get events and more info */
	if (XineramaIsActive(display) != True)
		return 0;

	xsi = XineramaQueryScreens(display, &n);
	for (i = 0; i < n; i++) {
		if (x >= xsi[i].x_org && x < xsi[i].x_org + xsi[i].width)
			return i;
	}
	/* TODO: xsi leaks memory, free it? */

	warnx("region out of bounds x=%d", x);

	return 0;
}

int
region_x_org(Display *display, int x)
{
	int i, n;
	XineramaScreenInfo *xsi;

	/* TODO: Use RandR because we can get events and more info */
	if (XineramaIsActive(display) != True)
		return 0;

	xsi = XineramaQueryScreens(display, &n);
	for (i = 0; i < n; i++) {
		if (x >= xsi[i].x_org && x < xsi[i].x_org + xsi[i].width)
			return xsi[i].x_org;
	}
	/* TODO: xsi leaks memory, free it? */

	warnx("region out of bounds x=%d", x);

	return 0;
}

int
region_width(Display *display, int x)
{
	int i, n;
	XineramaScreenInfo *xsi;

	/* TODO: Use RandR because we can get events and more info */
	if (XineramaIsActive(display) != True)
		return DisplayWidth(display, DefaultScreen(display));

	xsi = XineramaQueryScreens(display, &n);
	for (i = 0; i < n; i++) {
		if (x >= xsi[i].x_org && x < xsi[i].x_org + xsi[i].width)
			return xsi[i].width;
	}
	/* TODO: xsi leaks memory, free it? */

	warnx("region out of bounds x=%d", x);

	return DisplayWidth(display, DefaultScreen(display));
}

int
region_height(Display *display, int x)
{
	int i, n;
	XineramaScreenInfo *xsi;

	/* TODO: Use RandR because we can get events and more info */
	if (XineramaIsActive(display) != True)
		return DisplayHeight(display, DefaultScreen(display));

	xsi = XineramaQueryScreens(display, &n);
	for (i = 0; i < n; i++) {
		if (x >= xsi[i].x_org && x < xsi[i].x_org + xsi[i].width)
			return xsi[i].height;
	}
	/* TODO: xsi leaks memory, free it? */

	warnx("region out of bounds x=%d", x);

	return DisplayHeight(display, DefaultScreen(display));
}




struct column *
cycle_column(struct column *head, struct column *current, int direction)
{
	switch (direction) {
	case Forward:
		current = current->next;
		break;
	case Backward:
		current = current->prev;
		break;
	}

	return (current == NULL) ? head : current;
}

#if 0
struct pane *
layout_find(struct layout *dis, Window w)
{
	struct pane *m;
	XPointer     data;

	if (XFindContext(dis->display, w, dis->context, &data) == 0)
		m = (struct pane *) data;
	else
		m = NULL;

	return m;
}
#endif

/*
 * Returns a pane for a window.
 */
#if 0
struct pane *
find_pane(Display *display, XContext context, Window window)
{
	struct pane *m;
	XPointer     data;

	if (XFindContext(display, window, context, &data) == 0)
		m = (struct pane *) data;
	else
		m = NULL;

	if (m != NULL && m->window == window)
		TRACE("find_pane window match for %s", PANE_STR(m));
	else if (m != NULL && m->frame == window)
		TRACE("find_pane frame match for %s", PANE_STR(m));
	else if (m != NULL && m->maximize_button == window)
		TRACE("find_pane maximize buttonf for %s", PANE_STR(m));

	return m;
}
#endif

/*
 * Returns optimal anchor pane.
 * Useful when moving from one column to another.
 */
struct pane *
align_nicely(struct pane *pane, struct column *target)
{
	struct pane *peer;

	peer = target->last;
	while (peer != NULL && pane != NULL && peer->y > pane->y)
		peer = peer->prev;

	if (peer == NULL)
		peer = target->last;

	return peer;
}

#if 0
struct pane*
layout_manage(Display *display, XContext context, Window window, int x)
{
	struct pane       *m;
	Window             frame, close_button, maximize_button;
	XWindowAttributes  attrib;
	int                rheight;

	if (XGetWindowAttributes(display, window, &attrib) == 0)
		assert(0);

	if (attrib.override_redirect == True)
		return NULL;

	if (attrib.height <= 0)
		attrib.height = 1;

	rheight = region_height(display, attrib.x);
	if (attrib.height > rheight - 20)
		attrib.height = rheight - 20;

	attrib.x = x;

	frame = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                    attrib.x, attrib.y, WSWIDTH,
		                    attrib.height + 20,
                                    0, 3123828388, 856489);

	close_button = XCreateSimpleWindow(display, frame,
	                                   WSWIDTH - 20, 0, 20,
	                                   20,
	                                   0, 0, 54584854);

	maximize_button = XCreateSimpleWindow(display, frame,
	                                      WSWIDTH - 43, 0, 20,
	                                      20,
	                                      0, 0, 54584854);

	m = create_pane(window, frame, attrib.height + 20);
	m->close_button = close_button;
	m->maximize_button = maximize_button;

	XSelectInput(display, m->maximize_button,
			             ButtonPressMask | ButtonReleaseMask);

	if (XFetchName(display, m->window, &m->name) == 0)
		m->name = NULL;

	XSaveContext(display, m->window, context, (XPointer) m);
	XSaveContext(display, m->frame, context, (XPointer) m);
	XSaveContext(display, m->maximize_button, context, (XPointer) m);

	XSelectInput(display, m->window, PropertyChangeMask);

	XAddToSaveSet(display, m->window);

	
#ifdef WANT_ZERO_BORDERS
	XSetWindowBorderWidth(display, m->window, 0);
#else
	XSetWindowBorderWidth(display, m->window, 10);
#endif
	XReparentWindow(display, m->window, m->frame, 0, m->layout->titlebar_height_px);


	return m;
}
#endif
