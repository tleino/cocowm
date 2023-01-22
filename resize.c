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

void
force_one_maximized(struct column *ws)
{
	struct pane *np;

	for (np = ws->first; np != NULL; np = np->next) {
		if (np == ws->layout->focus) {
			if (!(np->flags & PF_MAXIMIZED))
				np->flags |= PF_MAXIMIZED;
			if (np->flags & PF_MINIMIZED) {
				np->flags ^= PF_MINIMIZED;
				transition_pane_state(np, NormalState,
				    ws->layout->display);
				XMapWindow(ws->layout->display, np->window);
			}
		} else {
			if (!(np->flags & PF_MINIMIZED)) {
				np->flags ^= PF_MINIMIZED;
				transition_pane_state(np, IconicState,
				    ws->layout->display);
				XUnmapWindow(ws->layout->display, np->window);
			}
		}
	}
}

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
	int vspacing;

	assert(ws != NULL);

	if (ws->n == 0)
		return;

#if 0
	XSync(ws->layout->display, True);
	XSetErrorHandler(wm_resize_error);
#endif

	TRACE_BEGIN("resizing column at x=%d with n=%d", ws->x, ws->n);

	vspacing = 0;

	dheight = ws->max_height - (vspacing);
	dheight -= (ws->n * vspacing);
	equal = dheight / ws->n;

	summed = sum_below_target(ws, &n_over, equal);
	dheight -= summed;

	if (n_over && dheight / n_over > equal)	
		equal = dheight / n_over;

	adjust_height(ws, equal, dheight);

	y = vspacing;
	for (np = ws->first; np != NULL; np = np->next) {
		XWindowChanges changes;

		slice = np->adjusted_height;

		if (slice < ws->layout->titlebar_height_px)
			slice = ws->layout->titlebar_height_px;
		else
			TRACE_ERR("slice was below font_height_px");

		TRACE("resize frame (np->frame %x) to %d,%d", (unsigned int) np->frame, ws->width, slice);

		changes.x = ws->x;
		changes.y = y;
		changes.width = ws->width;
		changes.height = slice;
		changes.stack_mode = Below;

		XConfigureWindow(ws->layout->display, np->frame,
		    CWX | CWY | CWWidth | CWHeight | CWStackMode, &changes);

		if (slice < ws->layout->titlebar_height_px)
			slice = ws->layout->titlebar_height_px;
		else
			TRACE_ERR("slice was below titlebar_height_px");

		TRACE("resize window %x (frame=%x) to %d,%d",
		    (unsigned int) np->window,
		    (unsigned int) np->frame, ws->width, slice - ws->layout->titlebar_height_px);

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
			if (status == True && a.map_state != IsUnmapped) {
				changes.x = 0;
				changes.y = ws->layout->titlebar_height_px;
				changes.height -= ws->layout->titlebar_height_px;
				changes.stack_mode = Above;
				XConfigureWindow(ws->layout->display, np->window,
				    CWX | CWY | CWWidth | CWHeight | CWStackMode,
				    &changes);
			}
		}

		np->y = y;
		y += (slice + vspacing);
	}

	for (np = ws->first; np != NULL; np = np->next) {
		if (np->flags & PF_FULLSCREEN) {
			XMoveWindow(ws->layout->display, np->frame, 0, 0);
			XResizeWindow(ws->layout->display, np->frame, 1280, 1200);
			XResizeWindow(ws->layout->display, np->window, 1280, 1200 - ws->layout->titlebar_height_px);
		}
	}

#if 0
	XClearWindow(ws->layout->display, DefaultRootWindow(ws->layout->display));
	XFillRectangle(ws->layout->display,
	               DefaultRootWindow(ws->layout->display),
	               ws->layout->column_gc, ws->x - hspacing, 0, 20 + hspacing, y + vspacing);
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
			height += ws->layout->titlebar_height_px; 
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
			pane->adjusted_height = ws->layout->titlebar_height_px;
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
