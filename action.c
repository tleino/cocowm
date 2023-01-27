#include "extern.h"

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
 * We need to track current column.
 * We always can have a current column even if there is no windows.
 * And current column gives the current window if we have one.
 */

static void move_pane(struct pane *, struct pane *);
void send_delete_window(struct pane *, Display *);
void send_take_focus(struct pane *p, Display *d);
void send_message(Atom a, Window w, Display *d);

int
handle_action(Display *display, XContext context, int op, int target,
              struct layout *layout)
{
	struct column *c;
	struct pane *focus;

	focus = layout->focus;

	TRACE("command: %s", OP_STR(op));

	switch (op) {
#ifdef STICKYKEYS
	case ToggleMode:
		toggle_keybind_mode(display);
		break;
#endif
	case RestartCommand:
		if (focus != NULL) {
			if (XGetCommand(layout->display, focus->window,
			    &focus->argv, &focus->argc) != 0) {
				focus->flags |= PF_WANT_RESTART;
				close_pane(focus, layout);
			}
		}
		break;
	case FocusColumn:
		c = get_column(layout->column, 1, target);

		/*
		 * We tried to find columns that have some panes,
		 * but if we failed, we still need to have focus on
		 * some column.
		 */
		if (c == NULL)
			focus_column(layout->head, layout);
		else
			focus_column(c, layout);

		/*
		 * If we simply didn't have panes, c->first will be
		 * NULL in which case we have column focus but no
		 * pane focus. That's okay.
		 */
		focus_pane(find_pane_by_vpos(focus->y, c), layout);
		break;
	case MoveColumn:
		if (focus != NULL) {
			c = get_column(layout->column, 0, target);
			remove_pane(focus, 1);
/*			resize(focus->column);*/
			manage_pane(focus, c, c->first);
			focus_pane(focus, layout);
		}
		break;
	case MovePane:
		if (focus != NULL && focus->column->n > 1)
			move_pane(focus, (target == Forward) ?
			          get_next_pane(focus) :
			          get_prev_pane(focus));
		break;
	case FocusPane:
		if (focus != NULL)
			focus_pane((target == Forward) ?
			           get_next_pane(focus) :
			           get_prev_pane(focus), layout);
#ifdef WANT_ONE_PER_COLUMN
		force_one_maximized(focus->column);
		resize(focus->column);
#endif
		break;
	case KillPane:
		if (focus != NULL)
			close_pane(focus, layout);
		break;
	case Fullscreen:
		if (focus != NULL) {
			focus->flags ^= PF_FULLSCREEN;
			resize_relayout(layout->focus->column);
		}
		break;
	case Maximize:
		if (focus != NULL) {
			focus->flags ^= PF_MAXIMIZED;
			draw_maximize_button(layout->focus, layout);
			resize_relayout(layout->focus->column);
		}
		break;
	case Minimize:
		if (focus != NULL) {
			focus->flags ^= PF_MINIMIZED;
			minimize(focus, layout);
			draw_frame(layout->focus, layout);
			resize_relayout(layout->focus->column);
		}
		break;
	case PrevFocus:
		if (focus != NULL) {
			TRACE("Focusing prev");
			focus_pane(find_previous_focus(layout->head, focus), layout);
		}
		break;
	default:
		TRACE("unhandled: %s", OP_STR(op));
		break;
	}

	return NoAction;
}

void
close_pane(struct pane *p, struct layout *l)
{
	if (!(p->flags & PF_HAS_DELWIN)) {
		XKillClient(l->display, p->window);
		observedestroy(l->display, l->context, p->window, l);
		return;
	}

	TRACE("delete window is sent!");
	send_delete_window(p, l->display);
}

/*
 * Move pane (p) to be after another pane (a) in a layout.
 */
static void
move_pane(struct pane *p, struct pane *a)
{
	assert(p != NULL && a != NULL);

	remove_pane(p, 1);
/*	resize(p->column);*/
	manage_pane(p, a->column, get_prev_pane(a));
}

void
minimize(struct pane *p, struct layout *l)
{
	assert(p != NULL && l != NULL);

	if (p->flags & PF_MINIMIZED) {
		transition_pane_state(p, IconicState, l->display);
		XUnmapWindow(l->display, p->window);
	} else {
		transition_pane_state(p, NormalState, l->display);
		XMapWindow(l->display, p->window);
	}

	if (p->column != NULL) {
		draw_frame(p, l);
		resize_remove(p->column, p);
		resize_add(p->column, p);
		resize_relayout(p->column);
	}
}

/*
 * Send atom (a) message to a client window (w).
 * Follows ICCCM conventions.
 */
void
send_message(Atom a, Window w, Display *d)
{
	XClientMessageEvent e;
	Atom wmp;

	e.type = ClientMessage;
	e.window = w;

	wmp = XInternAtom(d, "WM_PROTOCOLS", False);
	e.message_type = wmp;
	e.format = 32;
	e.data.l[0] = a;
	e.data.l[1] = CurrentTime;

	XSendEvent (d, w, False, 0, (XEvent *) &e);
}

void
send_delete_window(struct pane *p, Display *d)
{
	send_message(XInternAtom(d, "WM_DELETE_WINDOW", False), p->window, d);
}

void
send_take_focus(struct pane *p, Display *d)
{
	send_message(XInternAtom(d, "WM_TAKE_FOCUS", False), p->window, d);
}

void
restart_pane(struct pane *p, Display *d)
{
	int i;
	static char cmd[256];

	TRACE("should restart");
	cmd[0] = 0;
	strlcat(cmd, "DISPLAY=:0 ", sizeof(cmd));
	for (i = 0; i < p->argc; i++) {
		strlcat(cmd, p->argv[i], sizeof(cmd));
		strlcat(cmd, " ", sizeof(cmd));
	}
	strlcat(cmd, "&", sizeof(cmd));
	TRACE("starting '%s'", cmd);
	system(cmd);
}
