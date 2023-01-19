#include "extern.h"

#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <err.h>

static void observemap         (Display *, XContext, Window, struct layout *);
static void interceptconfigure (XContext, Window, XConfigureRequestEvent);
static void observe_unmap(Display *, XContext, Window, struct layout *);

static void handle_button_release (XEvent *, struct layout *);
static void handle_button_press   (Display *, XContext, XEvent *, struct layout *);
static void snap_pane             (XEvent *, struct layout *);

static void draw(Window w, struct layout *l);

int
handle_event(Display *display, XEvent *event, XContext context,
             struct layout *layout)
{
	int op, target;

	if (event->type != MotionNotify &&	/* Avoid logging too much */
	    event->type != Expose && event->type != ConfigureNotify)
		TRACE_BEGIN("event: %s (xany.window: %lx)", EVENT_STR(event),
		            event->xany.window);

	if (event->type == KeyRelease) {
		if ((op = find_binding(&(event->xkey), &target)) == -1) {
			TRACE("key without registered action " \
			      "- how this can happen?");
			return NoAction;
		}
		TRACE("handling user action");
		return handle_action(display, context, op, target, layout);
	}

	switch (event->type) {
		case ButtonPress: {
			struct pane *p;

	p = find_pane_by_window(event->xbutton.window, layout);

			if (p && p->window != event->xbutton.window) {
#if 0
			XAllowEvents(layout->display, AsyncPointer, CurrentTime);
#endif


#if 0
			XAllowEvents(layout->display, SyncPointer, CurrentTime);
#endif

#if 0
			/* TODO: This does nothing, remove? */
			XChangeActivePointerGrab(display, ButtonMotionMask | ButtonReleaseMask, None, CurrentTime);
#endif

			XGrabServer(display);
			handle_button_press(display, context, event, layout);
			} else {
		if (!(p->flags & PF_FOCUSED)) {
				focus_pane(p, layout);
			XAllowEvents(layout->display, SyncPointer, CurrentTime);
		} else {
				TRACE("Buttonpress here, let's replay pointer");
			XAllowEvents(layout->display, ReplayPointer, CurrentTime);
		}
			}

	}
			break;
		case ButtonRelease: {
			struct pane *p;
			p = find_pane_by_window(event->xbutton.window, layout);

			if (p && event->xbutton.window != p->window)
/*event->xbutton.y < 20) */ {
#if 0
			XAllowEvents(layout->display, SyncPointer, CurrentTime);
#endif
				if (layout->has_outline) {
					draw_outline(layout->active,
					             layout->outline_x,
				        	     layout->outline_y,
					             layout);
					layout->has_outline = false;
				}
				XUngrabServer(display);

				handle_button_release(event, layout);
			} else {
#if 0
			struct pane *p;
	p = find_pane_by_window(event->xbutton.window, layout);
#endif
			TRACE("buttonrelease, lets replay pointer");
			XAllowEvents(layout->display, ReplayPointer, CurrentTime);
/*			XSync(layout->display, False);*/
/*			XSendEvent(layout->display, p->window, False, ButtonReleaseMask, event);*/
			}
	}
			break;
		case MotionNotify: {
			if (1 || event->xbutton.y < 20) {
			/*
			 * TODO: Disallow motion events unless we've
			 *       got an active click to right place
			 */
#if 0
			assert(layout->active != NULL);
#endif
			if (layout->active != NULL) {
				layout->dblclick_time = 0;

				/* Undraw previous frame */
				if (layout->has_outline)
					draw_outline(layout->active,
					             layout->outline_x,
					             layout->outline_y,
					             layout);

				/* Draw */
				layout->outline_x = event->xmotion.x_root -
				                    layout->active_x;
				layout->outline_y = event->xmotion.y_root -
				                    layout->active_y;
				draw_outline(layout->active,
				             layout->outline_x,
				             layout->outline_y, layout);
				layout->has_outline = true;
			}
		}

		}

			break;
		case Expose:
			if (event->xexpose.count == 0)
				draw(event->xexpose.window, layout);
			break;
		case DestroyNotify:
			TRACE("xdestroywindow.window: %lx",
			      event->xdestroywindow.window);
			observedestroy(display, context, event->xdestroywindow.window, layout); /* was xany.window */
			break;
		case UnmapNotify:
			TRACE("xmap.window: %lx", event->xmap.window);
			observe_unmap(display, context, event->xmap.window, layout);
			break;
		case ConfigureRequest:
			TRACE("xconfigurerequest.window: %lx",
			      event->xconfigurerequest.window);
			interceptconfigure(context, event->xany.window,
			  event->xconfigurerequest);
			break;
		case ConfigureNotify: {
			TRACE("- configurenotify - xconfigure.window: %lx",
			      event->xconfigure.window);
#if 0
			struct pane *p;

			/*
			 * We catch have taken control of Configure so
			 * we need to remember to relay this event back
			 * to the original client in case they want to
			 * do something with it.
			 */
			p = find_pane_by_window(event->xbutton.window, layout);
			if (p != NULL && p->window == event->xconfigure.window) {
				XSendEvent(layout->display, p->window, False, StructureNotifyMask, event);
				TRACE("sent the configurenotify forward");
			}
				
			
#endif
			break;
		}
		case ResizeRequest: {
			struct pane *p;
			TRACE("- resize request");

			p = find_pane_by_window(event->xbutton.window, layout);
			if (p != NULL && !(p->flags & PF_MINIMIZED)) {
				TRACE("from pane %s", PANE_STR(p));
				p->height = event->xresizerequest.height + 20;
				TRACE("...height is set to: %d", p->height);
			}
			if (p->column != NULL)
				resize(p->column);

#if 0
			XSync(layout->display, False);
#endif
			break;
		}
		case CreateNotify: {
			char *s;

			TRACE("xcreatewindow.window=%lx parent=%lx",
			      event->xcreatewindow.window,
			      event->xcreatewindow.parent);
			XFetchName(event->xcreatewindow.display,
			           event->xcreatewindow.window, &s);
			TRACE("Created size: %d,%d, bw=%d",
			      event->xcreatewindow.width,
			      event->xcreatewindow.height,
			      event->xcreatewindow.border_width);
			TRACE("Created name: %s", s);

			free(s);
			break;
		}
		case PropertyNotify: {
			struct pane *p;

			p = find_pane_by_window(event->xproperty.window, layout);
			if (p != NULL) {
				TRACE("handling property notify the ugly way");
				XFetchName(layout->display, p->window, &p->name);
				XGetIconName(layout->display, p->window, &p->icon_name);
				draw_frame(p, layout);
			}
			break;
		}
		case MapRequest: {
			struct pane *p;

			p = find_pane_by_window(event->xmaprequest.window, layout);
			if (p == NULL)
				create_pane(event->xmaprequest.window, layout);
			else
				TRACE("double map, ignore");
			break;
		} case MapNotify:
			TRACE("xmap.window: %lx", event->xmap.window);

			/* add_window */
			observemap(display, context, event->xmap.window,
			           layout);
			break;
		case ReparentNotify:
			TRACE("xreparent.window: %lx (parent %lx)",
			      event->xreparent.window, event->xreparent.parent);

			/*
			 * At this point we can safely focus and get events
			 * because the window is mapped and reparented.
			 */

			XSelectInput(display, event->xreparent.window,
			    SubstructureNotifyMask | ResizeRedirectMask | PropertyChangeMask);

			XSelectInput(display, event->xreparent.parent,
			             ButtonPressMask | ButtonReleaseMask |
			             ButtonMotionMask | ExposureMask |
			             SubstructureNotifyMask);
			break;
		default:
			TRACE_ERR("unhandled event: %s", EVENT_STR(event));
			break;
		}

	return NoAction;
}

static void
handle_button_release(XEvent *event, struct layout *layout)
{
	XEvent junk;

	if (event->xbutton.y > 20 && layout->active != NULL) {
		TRACE("button release event was over titlebar area!");
/*
		XSendEvent(layout->display, layout->active->window, True, ButtonReleaseMask, event);
*/
/*		return;*/
	}

	/*
	 * We may have got motion notify events in the queue,
	 * let's get rid of those.
	 */
	XCheckMaskEvent(layout->display, ButtonMotionMask, &junk);

	TRACE("buttonrelease - buttonpress time: %ld",
	      event->xbutton.time - layout->dblclick_time);

	if (layout->active != NULL &&
	    event->xbutton.time - layout->dblclick_time < 500) {
		TRACE("minimize on doubleclick");
		layout->active->flags ^= PF_MINIMIZED;
		layout->dblclick_time = 0;
		minimize(layout->active, layout);
	} else {
		TRACE("not minimize time is: %ld", event->xbutton.time);
		layout->dblclick_time = event->xbutton.time;
	}

	snap_pane(event, layout);
}

static void
handle_button_press(Display *display, XContext context, XEvent *event,
                    struct layout *layout)
{
	struct pane *pane;

	TRACE("button press on %lx\n", event->xbutton.window);

	layout->active = NULL;

	pane = find_pane_by_window(event->xbutton.window, layout);

	if (pane == NULL) {
		TRACE_ERR("NULL mw in buttonpress");
	} else {
		if (event->xbutton.window == pane->maximize_button) {
			TRACE("button was maximize button");
			pane->flags ^= PF_MAXIMIZED;
			draw(event->xbutton.window, layout);
			resize(pane->column);
			return;
		} else if (event->xbutton.window == pane->close_button) {
			TRACE("close button pressed");
			close_pane(pane, layout);
			return;
		}

		TRACE("...was pane %ld %s", PANE_NUMBER(pane), pane->name);
		TRACE("pane->frame = %lx, pane->window = %lx "
		      "event->xbutton.window = %lx",
		      pane->frame, pane->window,
		      event->xbutton.window);	

		/*
		 * TODO: Remove this or actually implement adding motionmask
		 *       handling only whenever required
		 */
		if (pane->frame == event->xbutton.window) {
			TRACE("added motionmask");
		}

		/*
		 * Raising window in a tiling window manager sounds stupid
		 * but ... we're doing it just in case ;)
		 */
		XRaiseWindow(display, pane->frame);

		TRACE("lets focus it");	
		focus_pane(pane, layout);

		/*
		 * TODO: This is a bit ugly. Actually active window is
		 *       same as focused window i.e. we could make it so.
		 */
		layout->active = pane;
		layout->active_x = event->xbutton.x;
		layout->active_y = event->xbutton.y;

		if (event->xbutton.y > 20) {
			TRACE("event was over titlebar area!");
	/*
			XSendEvent(layout->display, pane->window, True, ButtonPressMask, event);
	*/
		}
	}
}

static void draw(Window w, struct layout *l)
{
	struct pane *p;

	assert(l != NULL);

	if ((p = find_pane_by_window(w, l)) == NULL)
		return;

	if (w == p->frame)
		draw_frame(p, l);
	else if (w == p->close_button)
		draw_close_button(p, l);
	else if (w == p->maximize_button)
		draw_maximize_button(p, l);
}

static void
snap_pane(XEvent *event, struct layout *layout)
{
	struct column *column;
	struct pane *pane;

	if ((pane = layout->active) == NULL)
		return;

	TRACE("snap %s", PANE_STR(pane));

	column = find_column(layout->head, event->xbutton.x_root);

	TRACE("...xbutton.x_root=%d", event->xbutton.x_root);

	TRACE("...active column x was=%d", layout->active->column->x);

	if (column != layout->active->column) {
		TRACE("snap pane column changed?");
		remove_pane(layout->active, 1);
/*		resize(layout->active->column);*/
		manage_pane(layout->active, column, NULL);
		focus_pane(layout->active, layout);
	}

	layout->active = NULL;
}

static void
observemap(Display *display, XContext context, Window window,
           struct layout *layout)
{
	struct pane       *pane;
	struct column     *column;
	XWindowAttributes  attrib;

	pane = find_pane_by_window(window, layout);
	if (pane == NULL) {
		TRACE_ERR("NULL pane in observemap");
		return;
	}

	TRACE("observemap for %s", PANE_STR(pane));
	pane->flags |= (PF_MAPPED | PF_DIRTY | PF_FOCUSED | PF_FOCUS);


	if (XGetWindowAttributes(display, window, &attrib) == 0)
		assert(0);

	column = find_column_by_hpos(attrib.x, layout->head);
	if (column != NULL && pane->column == NULL) {
		TRACE("add pane from observemap");

/* find_previous_focus(l->head) */

		if (!(pane->flags & PF_CAPTURE_EXISTING) &&
		    layout->column != NULL)
			manage_pane(pane, layout->column,
			            layout->focus);
		else
			manage_pane(pane, column, column->last);

		pane->flags &= ~PF_CAPTURE_EXISTING;

		TRACE("focus pane from observemap");
		focus_pane(pane, layout);
	} else {
		TRACE("Got back old window from withdrawn/iconified?");
		if (pane->column != NULL)
			resize(pane->column);
		if (pane->flags & PF_FOCUSED)
			focus_pane(pane, layout);
	}

	draw_frame(pane, layout);
	draw_close_button(pane, layout);
	draw_maximize_button(pane, layout);
}

static void
observe_unmap(Display *display, XContext context, Window window,
    struct layout *l)
{
	struct pane *pane;
#if 0
	XEvent event;
#endif

	if ((pane = find_pane_by_window(window, l)) == NULL) {
		TRACE_ERR("destroy: NULL mw in observe_unmap");
		return;
	}

	if (pane->window == window) {
		TRACE("destroy: unmapping window %lx from %s",
		      pane->window, PANE_STR(pane));

#if 0
		pane->flags &= PF_MINIMIZED;
#endif

		pane->flags &= ~PF_MAPPED;
#if 0
		minimize(pane, l);
#endif

#if 0	
		if (XCheckTypedEvent(display, UnmapNotify, &event) ==
		    True) {
			TRACE("destroy: there was another unmap in queue");
			XPutBackEvent(display, &event);
			remove_pane(pane, 0);	/* No resize, please */
		} else {
			remove_pane(pane, 1);	/* Want resize */
		}
#endif

#if 0
		if (pane->column != NULL)
			remove_pane(pane, 1);

		l->focus = NULL;

		/* Need to move focus to somewhere else */
		if (!(pane->flags & PF_WANT_RESTART))
			focus_pane(find_previous_focus(l->head, NULL), l);

#endif
	} else {
		TRACE_ERR("destroy: did not find window to unmap from %s",
		          PANE_STR(pane));
	}
}

void
observedestroy(Display *display, XContext context, Window window, struct layout *l)
{
	struct pane *pane;
	struct column *c;
	int flags;
#if 0
	XEvent event;
#endif

	TRACE("destroy begin");

	if ((pane = find_pane_by_window(window, l)) == NULL) {
		TRACE_ERR("destroy: NULL mw in observedestroy");
		return;
	}

	/*
	 * TODO: remove_pane calls resize column, which does operations with
	 * windows such as moving and resizing windows. However, if multiple
	 * windows have been "simultaneously" destroyed, meaning we have
	 * DestroyNotify's in the queue, we may have "stale" windows attached
	 * to a column. We could either XGetGeometry to find out whether
	 * the window still lives, or we could process the queue with
	 * XCheckEvent or similar to find out if we have pending
	 * DestroyNotify. Using XGetGeometry requires round-trips and is
	 * probably the slower approach.
	 * TODO: Remove this comment if it works now
	 * TODO: Remove this logic because it might not be needed if
	 * the unmapping logic does the job. In practice:
	 * 1. Unmap removes the window from the frame and runs resizing
	 * 2. Destroy removes the frame and runs resizing
	 * And this works because we control destroying the frame.
	 * Two modes are needed: unmapped and minimized. Minimized
	 * is always also unmapped.
	 */
	c = pane->column;
#if 0
	if (XCheckTypedEvent(display, DestroyNotify, &event) == True) {
		TRACE("destroy: there was another destroy in queue");
		XPutBackEvent(display, &event);
		remove_pane(pane, 0);	/* No resize, please */
	} else {
		remove_pane(pane, 0);	/* Want resize */
	}
#elif 1
	remove_pane(pane, 1);
#endif

	if (pane->flags & PF_WANT_RESTART) {
		if (c != NULL)
			focus_column(c, l);
		restart_pane(pane, l->display);
	}
	flags = pane->flags;

	if (pane->frame) {
		TRACE("destroy: destroying frame %lx in %s", pane->frame,
		      PANE_STR(pane));
		/*
		 * This also destroys its children, obviously.
		 */
		XDestroyWindow(display, pane->frame);
	}
	XDeleteContext(display, pane->window, context);
	XDeleteContext(display, pane->frame, context);
	XDeleteContext(display, pane->maximize_button, context);
	XDeleteContext(display, pane->close_button, context);

	if (pane->name != NULL)
		XFree(pane->name);
	if (pane->icon_name != NULL)
		XFree(pane->icon_name);

	free(pane);

#if 1
	/*
	 * This is currently a special case handling. We could do
	 * this inside other functions automatically, but we don't
	 * wish to clear focus and re-set it all the time in the
	 * normal cases, so we clear focus only here.
	 */
	l->focus = NULL;

	/* Need to move focus to somewhere else */
	if (!(flags & PF_WANT_RESTART))
		focus_pane(find_previous_focus(l->head, NULL), l);
#endif
}

static void
interceptconfigure(XContext context, Window w,
                   XConfigureRequestEvent e)
{
	XWindowChanges xwc;
	unsigned long xwcm;

	TRACE("intercepted %d,%d to %d,%d", e.x, e.y, e.width, e.height);

	xwcm = e.value_mask &
	    (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
	xwc.x = e.x;
	xwc.y = e.y;

#if 0
	xwc.width = e.width;
#else
	/*
	 * Forcing all to be same width regardless of what is requested.
	 */
	xwc.width = WSWIDTH;
#endif

	xwc.height = e.height;
	xwc.border_width = 0;

	TRACE("configuring window %lx to %d,%d, bw width %d",
	      e.window, xwc.width, xwc.height, xwc.border_width);
	XConfigureWindow(e.display, e.window, xwcm, &xwc);
}
