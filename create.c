#include "extern.h"

#include <stdlib.h>
#include <err.h>

static void update_size_hints(struct pane *, XWindowAttributes *, Display *);
static void update_hints(struct pane *, Display *);

/*
 * Creates a managed window (a pane in our terminology) by reparenting
 * an unmanaged window (w) with a frame that allows control of the
 * original window.
 *
 * For associating a window with a pane, the original window and all the
 * windows of the controls are added to a context in a layout (l).
 *
 * Returns NULL if the window didn't want to be managed.
 *
 * NOTE: This window may be unmapped (Withdrawn) when we initially create
 * pane for it, e.g. when starting the window manager and the window has
 * been previously iconified.
 */
struct pane *
create_pane(Window w, struct layout *l)
{
	static unsigned long  n_panes;
	struct pane          *p;
	XWindowAttributes     a;
	int                   rheight;
	XGCValues             v;
	XSetWindowAttributes  sa;
	char s[256];

	assert(l != NULL);

	/* TODO: Take these values from CreateNotify */
	if (XGetWindowAttributes(l->display, w, &a) == 0)
		assert(0);

	if ((p = calloc(1, sizeof(struct pane))) == NULL)
		err(1, "managing new window");

	/*
	 * When starting the window manager, we check for
	 * override_redirect and IsUnmapped but create_pane() is
	 * also called from MapRequest manager, as we intercept
	 * all attempts to map windows, therefore we need to also
	 * verify here if all is good and we can manage this window.
	 *
	 * TODO: Remove this logic from the startup code and just
	 *       have it here.
	 */
	if (a.override_redirect == True) {
		TRACE_ERR("create: create_pane override redirect");

		/* We don't wish to map window which is Iconified */
		if (a.map_state != IsUnmapped)
			XMapWindow(l->display, w);
		return NULL;
	}

	/* TODO: Get this from createnotify or something */
	XFetchName(l->display, w, &p->name);

#ifdef WANT_ZERO_BORDERS
	if (a.border_width > 0)
		XSetWindowBorderWidth(l->display, w, 0);
#else
		XSetWindowBorderWidth(l->display, w, 10);
#endif

	p->number = ++n_panes;
	p->window = w;

	TRACE("create: Original window size: %d,%d", a.width, a.height);

	update_hints(p, l->display);

	update_size_hints(p, &a, l->display);

	read_pane_protocols(p, l->display);

	if (p->flags & PF_MINIMIZED)
		transition_pane_state(p, IconicState, l->display);
	else
		transition_pane_state(p, NormalState, l->display);

	rheight = region_height(l->display, a.x);
	if (a.height > rheight - (l->titlebar_height_px)) {
		TRACE("create: height was over rheight");
		a.height = rheight - (l->titlebar_height_px);
	}

	p->height = a.height + (l->titlebar_height_px);

	TRACE("create: p->height is: %d", p->height);

	/* TODO: Is this necessary for moving pane to a nice place? */
#if 0
	a.x = x;
#endif

	XGetGCValues(l->display, l->normal_gc, GCBackground, &v);

	sa.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask;
	sa.background_pixel = v.background;
	p->frame = XCreateWindow(l->display, DefaultRootWindow(l->display),
	                         a.x, a.y, l->column->width, p->height,
	                         0, CopyFromParent, InputOutput,
	                         CopyFromParent, CWEventMask | CWBackPixel,
	                         &sa);

	snprintf(s, sizeof(s), "cocowm frame for 0x%lx (%s)", w, p->name);
	XStoreName(l->display, p->frame, s);

	TRACE("create: create pane frame is %lx", p->frame);
	XSync(l->display, False);
	sa.win_gravity = NorthEastGravity;
	p->close_button = XCreateWindow(l->display, p->frame,
	                                l->column->width - l->font_height_px, 0,
					l->font_height_px, (l->titlebar_height_px),
	                                0, CopyFromParent, InputOutput,
	                                CopyFromParent,
	                                CWEventMask | CWBackPixel | CWWinGravity, &sa);
	TRACE("create: create pane close is %lx", p->close_button);

	snprintf(s, sizeof(s), "cocowm close_button for 0x%lx (%s)", w, p->name);
	XStoreName(l->display, p->close_button, s);

	XSync(l->display, False);
	p->maximize_button = XCreateWindow(l->display, p->frame,
	                                   l->column->width - (l->font_height_px * 2),
                                           0, l->font_height_px, (l->titlebar_height_px),
	                                   0, CopyFromParent, InputOutput,
	                                   CopyFromParent,
	                                   CWEventMask | CWBackPixel | CWWinGravity, &sa);

	snprintf(s, sizeof(s), "cocowm maximize_button for 0x%lx (%s)", w, p->name);
	XStoreName(l->display, p->maximize_button, s);

	TRACE("create: create pane maximize is %lx", p->maximize_button);
	XSync(l->display, False);

	XGetGCValues(l->display, l->column_gc, GCForeground, &v);
#if 0
	sa.background_pixel = v.foreground;
	p->column_button = XCreateWindow(l->display, p->frame,
	                                   0, 0, 20, 20,
	                                   0, CopyFromParent, InputOutput,
	                                   CopyFromParent,
	                                   CWBackPixel, &sa);
#endif

	XGetIconName(l->display, w, &p->icon_name);

	XSaveContext(l->display, w, l->context, (XPointer) p);
	XSaveContext(l->display, p->frame, l->context, (XPointer) p);
	XSaveContext(l->display, p->maximize_button, l->context, (XPointer) p);
	XSaveContext(l->display, p->close_button, l->context, (XPointer) p);

#if 0
	XSelectInput(l->display, w, PropertyChangeMask);
#endif

	XAddToSaveSet(l->display, w);

	XMapWindow(l->display, p->frame);
	XMapSubwindows(l->display, p->frame);

#if 0
	XReparentWindow(l->display, w, p->frame, 0, (l->titlebar_height_px));

	/* TODO: Or move these to Reparent window event? */
	if (!(p->flags & PF_MINIMIZED))
		XMapWindow(l->display, w);

	XMapSubwindows(l->display, p->frame);
#endif

	/*
	 * Required for click-to-focus.
	 */
	XGrabButton(l->display, 1, 0, p->window, True,
	            ButtonPressMask | ButtonReleaseMask, GrabModeSync,
		    GrabModeAsync, None, None);

	/*
	 * This I guess is required so that we don't get an infinite
	 * loop of events passing back and forth.
	 */
	sa.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
	sa.event_mask = StructureNotifyMask;
	XChangeWindowAttributes(l->display, w, CWEventMask | CWDontPropagate,
	                        &sa);

	return p;
}

static void
update_size_hints(struct pane *p, XWindowAttributes *a, Display *d)
{
	XSizeHints normal;
	long supplied;
	int min_height_px, j;

	if (XGetWMNormalHints(d, p->window, &normal, &supplied) == 0) {
		TRACE_ERR("hints: none for %lx", p->window);
	} else {
		/* Minimum size */
		if ((normal.flags & (PBaseSize | PResizeInc | PMinSize)) ==
		    (PBaseSize | PResizeInc | PMinSize)) {
			/*
			 * This algorithm is from Xlib Programming Manual
			 * X11R5 section 12.3.1.3.
			 */
			for (j = 0; j < normal.min_height; j++) {
				min_height_px = normal.base_height +
				                (j * normal.height_inc);
			}
			if (a->height < min_height_px) {
				TRACE("hints: minheight is set to %d", min_height_px);
				a->height = min_height_px;
			}

			p->min_height = min_height_px;

			TRACE("hints: baseheight with resize increments");
		} else if (normal.flags & PBaseSize) {
			if (a->height < normal.base_height) {
				TRACE("hints: minsize set because %d!!!", a->height);
				a->height = normal.base_height;
			}

			p->min_height = normal.base_height;

			TRACE("hints: baseheight only, minheight %d, height now %d", p->min_height, a->height);
		} else if (normal.flags & PMinSize) {
			if (a->height < normal.min_height) {
				TRACE("hints: minsize set because %d", a->height);
				a->height = normal.min_height;
			}

			p->min_height = normal.min_height;

			TRACE("hints: minheight only");
		}

		/* Maximum size */
		if (normal.flags & PMaxSize) {
			if (a->height > normal.max_height) {
				TRACE("hints: max height is set!!");
				a->height = normal.max_height;
			}

			p->max_height = normal.max_height;
		}

		TRACE("hints: minmax height set for pane %ld "
		    "to be min=%dpx max=%dpx and current height is %d",
		    PANE_NUMBER(p),
		    p->min_height, p->max_height, a->height);
	}
}

static void
update_hints(struct pane *p, Display *d)
{
	XWMHints *hints;

	if ((hints = XGetWMHints(d, p->window)) == NULL)
		return;

	if (hints->flags & StateHint)
		if (hints->initial_state == IconicState) {
			TRACE("hints: WOW, initial state is Iconic?? Cool");
			p->flags |= PF_MINIMIZED;
		}

	if (read_pane_state(p, d) == IconicState)
		p->flags |= PF_MINIMIZED;

	XFree(hints);
}
