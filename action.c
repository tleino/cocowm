#include "extern.h"

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

/*
 * We need to track current column.
 * We always can have a current column even if there is no windows.
 * And current column gives the current window if we have one.
 */

static void move_pane(struct pane *, int);
void send_delete_window(struct pane *, Display *);
void send_take_focus(struct pane *p, Display *d);
void send_message(Atom a, Window w, Display *d);

static int edit_command(struct pane *p, struct layout *l);
static void new_command(struct pane *p, struct layout *l);

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
	case EditCommand:
		edit_command(focus, layout);
		break;
	case NewCommand:
		new_command(focus, layout);
		break;
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

#ifdef WANT_ONE_PER_COLUMN
		force_one_maximized(layout->focus->column);
#endif
		break;
	case MoveColumn:
		if (focus != NULL) {
			c = get_column(layout->column, 0, target);
			remove_pane(focus, 1);
			manage_pane(focus, c, c->first);
			focus_pane(focus, layout);
#ifdef WANT_ONE_PER_COLUMN
			force_one_maximized(layout->focus->column);
#endif
		}
		break;
	case MovePane:
		if (focus != NULL && focus->column->n > 1) {
			move_pane(focus, target);
		}
		break;
	case FocusPane:
		if (focus != NULL)
			focus_pane((target == Forward) ?
			           get_next_pane(focus) :
			           get_prev_pane(focus), layout);
#ifdef WANT_ONE_PER_COLUMN
		force_one_maximized(focus->column);
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
			focus->flags ^= PF_KEEP_OPEN;
			draw_maximize_button(layout->focus, layout);
#ifdef WANT_ONE_PER_COLUMN
			force_one_maximized(focus->column);
#else
			resize_relayout(layout->focus->column);
#endif
		}
		break;
	case Minimize:
		if (focus != NULL) {
			if (target == Others) {
				minimize_others(focus, layout);
			} else {
				focus->flags ^= PF_MINIMIZED;
				focus->flags &= ~PF_HIDDEN;
				minimize(focus, layout);
			}
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

static void
new_command(struct pane *focus, struct layout *l)
{
	struct pane *p;

	p = create_empty_pane(l, l->column->x);
	manage_pane(p, l->column, focus);

	XMapWindow(l->display, p->frame);
	XMapSubwindows(l->display, p->frame);

	draw_frame(p, l);
	focus_pane(p, l);
	if (edit_command(p, l) == 0)
		close_pane(p, l);
}

static int
edit_command(struct pane *p, struct layout *l)
{
	p->flags |= PF_EDIT;
	draw_frame(p, l);
	if (prompt_read(&p->prompt)) {
		p->flags |= PF_WANT_RESTART;
		close_pane(p, l);
		return 1;
	}
	return 0;
}

void
close_pane(struct pane *p, struct layout *l)
{
	if (p->flags & PF_EMPTY) {
		p->name = NULL;
		observedestroy(l->display, l->context, p->frame, l);
	} else if (!(p->flags & PF_HAS_DELWIN)) {
		XKillClient(l->display, p->window);
		observedestroy(l->display, l->context, p->window, l);
		return;
	}

	TRACE("delete window is sent!");
	send_delete_window(p, l->display);
}

/*
 * Move pane 'p' in a column to 'direction'.
 */
static void
move_pane(struct pane *p, int direction)
{
	struct pane *after, *target;
	struct column *c;

	assert(p != NULL);

	if (direction == Forward) {
		target = p;
		after = p->next;
		/* If after is NULL, the move goes to the top */
	} else {
		target = p->prev;
		after = p;
		if (target == NULL) {
			target = p;
			after = p->column->last;
		}
	}

	assert(target != NULL);
	c = target->column;

	remove_pane(target, 0);
	manage_pane(target, c, after);
}

void
minimize_others(struct pane *focus, struct layout *l)
{
	struct pane *p;
	struct column *ws;
	int was_leader;

	if (focus == NULL)
		return;

	ws = focus->column;
	if (ws == NULL)
		return;

	if (focus->flags & PF_MINIMIZED) {
		focus->flags &= ~(PF_MINIMIZED | PF_HIDDEN);
		minimize(focus, l);
	}
	was_leader = focus->flags & PF_HIDE_OTHERS_LEADER;
	if (was_leader == 0)
		focus->flags |= PF_HIDE_OTHERS_LEADER;
	else
		focus->flags &= ~PF_HIDE_OTHERS_LEADER;

	for (p = ws->first; p != NULL; p = p->next) {
		if (p == focus)
			continue;
		if (p->flags & PF_KEEP_OPEN)
			continue;
		p->flags &= ~PF_HIDE_OTHERS_LEADER;
		if (was_leader && p->flags & PF_HIDDEN)
			p->flags &= ~(PF_MINIMIZED | PF_HIDDEN);
		else if (!was_leader && !(p->flags & PF_MINIMIZED))
			p->flags |= (PF_MINIMIZED | PF_HIDDEN);
		minimize(p, l);
	}
}

void
minimize(struct pane *p, struct layout *l)
{
	assert(p != NULL && l != NULL);

	if (p->flags & PF_MINIMIZED) {
		transition_pane_state(p, IconicState, l->display);
		if (p->flags & PF_KEEP_OPEN) {
			p->flags &= ~PF_KEEP_OPEN;
			draw_maximize_button(p, l);
		}
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

static int
maybe_url(char *s)
{
	int ret = 0;

	while (*s != '\0' && !isspace(*s))
		if (*s++ == '.')
			ret = 1;

	if (ret && !isspace(*s))
		return ret;

	return 0;
}

static void
run_command(struct pane *pane, char *s)
{
	char *q, *p, *cmd, *sh;
	int i;
	pid_t pid;
	size_t sz;
	char *prefix;

	if (s == NULL || strlen(s) == 0)
		return;

	q = strdup(s);
	if (q == NULL) {
		warn("strdup");
		return;
	}

	/* Strip trailing spaces */
	for (i = strlen(q)-1; i >= 0 && isspace(q[i]); i--)
		q[i] = '\0';

	if (q[0] == '!') {
		prefix = "exec xterm -hold -e ";
		cmd = q+1;
	} else if (maybe_url(q)) {
		prefix = "exec firefox-esr ";
		cmd = q;
	} else {
		prefix = "exec ";
		cmd = q;
	}

	sz = strlen(q) + strlen(prefix) + 1;
	p = malloc(sz);
	if (p == NULL) {
		warn("malloc");
		return;
	}

	if (snprintf(p, sz, "%s%s", prefix, cmd) >= sz)
		assert(0);

#if 0
	if (add_command_to_history(q, 1))
		save_command_history();
#endif

	free(q);

	sh = getenv("SHELL");
	if (sh == NULL) {
		warn("getenv SHELL");
		sh = "/bin/sh";
	}
	pid = fork();
	if (pid == 0) {
		setsid();
		setenv("DISPLAY", ":0", 1);
		execl(sh, sh, "-c", p, NULL);
	} else {
		if (pid == -1)
			warn("fork");
		free(p);
	}
}

void
restart_pane(struct pane *p, Display *d)
{
	int i;
	static char cmd[1024];

	TRACE("should restart");
	if (strlen(p->prompt.text)) {
		run_command(p, p->prompt.text);
	} else {
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
}
