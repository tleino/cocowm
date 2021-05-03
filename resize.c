#include "extern.h"

#include <err.h>

static int  sum_below_target (struct column *, int *, int);
static void adjust_height    (struct column *, int, int);

#if 1
static int
wm_resize_error(Display *display, XErrorEvent *event)
{
	warnx("error in resizing");
	return 1;
}
#endif

/*
 * Called for all windows in a column after even one window has been
 * modified, such as if a window has been removed. However, since this
 * processing is started immediately from an event handler for one
 * window, yet may affect multiple windows, we need to be careful not
 * to modify windows that may already have been deleted. That is, we
 * may have DestroyNotify or UnmapNotify in the queue, but not yet
 * processed. Worse, we may not have them yet in the queue, but the
 * server has already destroyed the window. So we need to have one
 * synchronous request per each window.... 
 */
void
resize(struct column *ws)
{
	struct pane *np;
	int slice, y, equal, summed, dheight, n_over;
	XWindowAttributes a;
	Status status;

	assert(ws != NULL);

	if (ws->n == 0)
		return;

#if 0
	XSync(ws->layout->display, True);
	XSetErrorHandler(wm_resize_error);
#endif

	TRACE_BEGIN("resizing column at x=%d with n=%d", ws->x, ws->n);

	dheight = ws->max_height - (VSPACING);
	dheight -= (ws->n * VSPACING);
	equal = dheight / ws->n;

	summed = sum_below_target(ws, &n_over, equal);
	dheight -= summed;

	if (n_over && dheight / n_over > equal)	
		equal = dheight / n_over;

	adjust_height(ws, equal, dheight);

	y = VSPACING;
	for (np = ws->first; np != NULL; np = np->next) {
		slice = np->adjusted_height;

		if (slice < 20)
			slice = 20;
		else
			TRACE_ERR("slice was below 20");

		TRACE("resize frame (np->frame %x) to %d,%d", (unsigned int) np->frame, WSWIDTH, slice);
		XResizeWindow(ws->layout->display, np->frame, WSWIDTH, slice);

		if (slice < 21)
			slice = 21;
		else
			TRACE_ERR("slice was below 21");

		TRACE("resize window %x (frame=%x) to %d,%d",
		    (unsigned int) np->window,
		    (unsigned int) np->frame, WSWIDTH, slice - 20);

		if ((np->flags & PF_MAPPED) && !(np->flags & PF_MINIMIZED)) {
			/*
			 * This is unfortunate that we have to use
			 * exception handling for a logic thing, as
			 * well as having to use a synchronous back and
			 * forth check for checking if the window we're
			 * about to play with still lives. This is
			 * because we're having buffered I/O and we may
			 * not have received all state at this point when
			 * we modify windows.
			 */
			XSetErrorHandler(wm_resize_error);
			status = XGetWindowAttributes(ws->layout->display,
			    np->window, &a);
			XSetErrorHandler(None);
			if (status == True && a.map_state != IsUnmapped)
				XResizeWindow(ws->layout->display,
				    np->window, WSWIDTH, slice - 20);
		}

		TRACE("moving frame to %d,%d", ws->x, y);

		XMoveWindow(ws->layout->display, np->frame, ws->x, y);

		np->y = y;

		y += (slice + VSPACING);

	}

	for (np = ws->first; np != NULL; np = np->next) {
		if (np->flags & PF_FULLSCREEN) {
			XMoveWindow(ws->layout->display, np->frame, 0, 0);
			XResizeWindow(ws->layout->display, np->frame, 1280, 1200);
			XResizeWindow(ws->layout->display, np->window, 1280, 1200 - 20);
		}
	}

#if 0
	XClearWindow(ws->layout->display, DefaultRootWindow(ws->layout->display));
	XFillRectangle(ws->layout->display,
	               DefaultRootWindow(ws->layout->display),
	               ws->layout->column_gc, ws->x - SPACING, 0, 20 + SPACING, y + VSPACING);
#endif

#if 0
	XSync(ws->layout->display, False);
	XSetErrorHandler(None);
#endif
}

static int
sum_below_target(struct column *ws, int *n_over, int target)
{
	struct pane *pane;
	int height;

	height = 0;
	*n_over = 0;
	for (pane = ws->first; pane != NULL; pane = pane->next) {
		if (pane->flags & PF_MINIMIZED) {
			height += 20 /* Was 21 */; 
		} else if (!(pane->flags & PF_MAXIMIZED) &&
		           pane->height <= target) {
			height += pane->height;
		} else {
			(*n_over)++;
		}
	}

	return height;
}

static void
adjust_height(struct column *ws, int equal, int max_height)
{
	struct pane *pane;

	for (pane = ws->first; pane != NULL; pane = pane->next) {
		if (pane->flags & PF_MINIMIZED)
			pane->adjusted_height = 20; /* Was 21 */
		else if (pane->flags & PF_MAXIMIZED)
			pane->adjusted_height = max_height;
		else
			pane->adjusted_height = pane->height;

		if (pane->adjusted_height > equal) {
			pane->adjusted_height = equal;
		} else {
		}
	}
}
