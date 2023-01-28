#include "extern.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

struct pane *
get_next_pane(struct pane *p)
{
	assert(p != NULL);
	assert(p->column != NULL);

	if (p->next != NULL)
		return p->next;

	return p->column->first;
}

struct pane *
get_prev_pane(struct pane *p)
{
	assert(p != NULL);
	assert(p->column != NULL);

	if (p->prev != NULL)
		return p->prev;

	return p->column->last;
}

struct pane *
find_pane_by_vpos(int y, struct column *column)
{
	struct pane *p;

	assert(column != NULL);

	p = column->last;
	while (p != NULL && y <= p->y && column->first != p)
		p = p->prev;

	return p;
}

/*
 * Find pane by window (w) from layout (l).
 */
struct pane *
find_pane_by_window(Window w, struct layout *l)
{
	XPointer     data;

	TRACE("finding window %lx", w);

	if (XFindContext(l->display, w, l->context, &data) != 0) {
		TRACE_ERR("did not find pane by window %lx", w);
		return NULL;
	}

	TRACE("...found!");
	return (struct pane *) data;
}

/*
 * Focus pane in a layout.
 * Only one pane can have a focus at a given time.
 * It is possible for focus to be NULL, e.g. in case there is no
 * panes at all.
 */
void
focus_pane(struct pane *p, struct layout *l)
{
	XGCValues v;
	static unsigned int return_priority;

	TRACE_BEGIN("focus_pane %s", PANE_STR(p));

	assert(l != NULL);

	if (l->focus != NULL && l->focus != p) {
		l->focus->flags &= ~PF_FOCUSED;

		XGetGCValues(l->display, l->normal_gc, GCBackground, &v);
		XSetWindowBackground(l->display, l->focus->frame,
		                     v.background);
		draw_frame(l->focus, l);
	}
	if (p != NULL) {
		p->flags |= PF_FOCUSED;

		XGetGCValues(l->display, l->focus_gc, GCBackground, &v);
		XSetWindowBackground(l->display, p->frame, v.background);

		if (p->flags & PF_WITHOUT_WINDOW) {
			TRACE("forcing focus to frame");
			XSetInputFocus(l->display, p->frame,
			               RevertToPointerRoot, CurrentTime);
		} else if (p->flags & PF_HAS_TAKEFOCUS) {
			TRACE("Sending takefocus");
			send_take_focus(p, l->display);
		} else if (p->flags & PF_MAPPED) {
			TRACE("forcing input focus");
			XSetInputFocus(l->display, p->window,
			               RevertToPointerRoot, CurrentTime);
		}
		draw_frame(p, l);

		l->column = p->column;

		p->return_priority = return_priority++;
		if (p->return_priority == 0) {	/* Wraparound */
			/* TODO: Handle wraparound */
		}
	}

	l->focus = p;

	assert(l->column != NULL);

	/* TODO: Push to focus history */
}

void
transition_pane_state(struct pane *p, int state, Display *d)
{
	unsigned long data[2];
	Atom wmstate;

	data[0] = (unsigned long) state;

#if 0
	if (state == IconicState)
		data[1] = p->frame;
	else
		data[1] = None;
#endif
	/*
	 * We set icon window always to None because we're not having
	 * a separate icon window in any case: all we have is the frame
	 * without an attached window.
	 */
	data[1] = None;

	/* TODO: Manage XInternAtom centrally in one place and cache it */
	wmstate = XInternAtom(d, "WM_STATE", False);

	XChangeProperty (d, p->window, wmstate, wmstate, 32,
	                 PropModeReplace, (unsigned char *) data,
	                 2);

	TRACE("WM_STATE is set to %d", state);
}

int
read_pane_state(struct pane *p, Display *d)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop_return = NULL;
	unsigned long data[2];
	Atom wmstate;

	wmstate = XInternAtom(d, "WM_STATE", False);
	if (XGetWindowProperty(d, p->window, wmstate, 0L, 2L, False, wmstate,
	                       &actual_type, &actual_format, &nitems,
	                       &bytes_after, &prop_return) != Success ||
            !prop_return || nitems > 2)
		return NormalState;

	data[0] = prop_return[0];
	XFree(prop_return);

	return data[0];
}

void
read_pane_protocols(struct pane *p, Display *d)
{
	Atom delwin, takefocus, *protocols = NULL, *ap;
	int n, i;

	delwin = XInternAtom(d, "WM_DELETE_WINDOW", False);
	takefocus = XInternAtom(d, "WM_TAKE_FOCUS", False);

	if (XGetWMProtocols(d, p->window, &protocols, &n)) {
		for (i = 0, ap = protocols; i < n; i++, ap++) {
			if (*ap == delwin) p->flags |= PF_HAS_DELWIN;
			if (*ap == takefocus) p->flags |= PF_HAS_TAKEFOCUS;
		}
		if (protocols != NULL)
			XFree(protocols);
	}
}

/*
 * Not needed often and n will be small, we can be naive here.
 * Find it after (a).
 */
struct pane *
find_previous_focus(struct column *head, struct pane *a)
{
	struct column *c;
	struct pane *p, *highest;

	highest = NULL;
	for (c = head; c != NULL; c = c->next) {
		for (p = c->first; p != NULL; p = p->next) {
			if (highest == NULL ||
			    p->return_priority > highest->return_priority) {
				if (p != a)
					highest = p;
			}
		}
	}

	return highest;
}
