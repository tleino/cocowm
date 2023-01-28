#include "extern.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

const char *
str_event(XEvent *event)
{
	static const struct event_str {
		int type;
		char *name;
	} es[] = {
		{ KeyPress, "KeyPress" },
		{ KeyRelease, "KeyRelease" },
		{ ButtonPress, "ButtonPress" },
		{ ButtonRelease, "ButtonRelease" },
		{ MotionNotify, "MotionNotify" },
		{ EnterNotify, "EnterNotify" },
		{ LeaveNotify, "LeaveNotify" },
		{ FocusIn, "FocusIn" },
		{ FocusOut, "FocusOut" },
		{ KeymapNotify, "KeymapNotify" },
		{ Expose, "Expose" },
		{ GraphicsExpose, "GraphicsExpose" },
		{ NoExpose, "NoExpose" },
		{ VisibilityNotify, "VisibilityNotify" },
		{ CreateNotify, "CreateNotify" },
		{ DestroyNotify, "DestroyNotify" },
		{ UnmapNotify, "UnmapNotify" },
		{ MapNotify, "MapNotify" },
		{ MapRequest, "MapRequest" },
		{ ReparentNotify, "ReparentNotify" },
		{ ConfigureNotify, "ConfigureNotify" },
		{ ConfigureRequest, "ConfigureRequest" },
		{ GravityNotify, "GravityNotify" },
		{ ResizeRequest, "ResizeRequest" },
		{ CirculateNotify, "CirculateNotify" },
		{ CirculateRequest, "CirculateRequest" },
		{ PropertyNotify, "PropertyNotify" },
		{ SelectionClear, "SelectionClear" },
		{ SelectionRequest, "SelectionRequest" },
		{ SelectionNotify, "SelectionNotify" },
		{ ColormapNotify, "ColormapNotify" },
		{ ClientMessage, "ClientMessage" },
		{ MappingNotify, "MappingNotify" },
		{ -1 }
	};
	int i;
	char *s;

	for (i = 0; es[i].type != -1; i++)
		if (es[i].type == event->type)
			break;

	if (es[i].type != -1)
		s = es[i].name;
	else
		s = "<unknown event>";

	return s;
}

const char *
str_op(int op)
{
	static const char *s[] = {
		"NoAction", "FocusPane", "MovePane", "FocusColumn",
		"MoveColumn", "KillPane", "RestartManager",
		"ToggleTagMode", "Maximize", "QuitManager", "Minimize",
		"Fullscreen", "PrevFocus", "EditCommand", "NewCommand",
		"RestartCommand", "ToggleMode",
		NULL
	};

	return s[op];
}

const char *
str_pane(struct pane *pane)
{
	static char buf[256];

	if (pane == NULL)
		snprintf(buf, sizeof(buf), "null pane");
	else
		snprintf(buf, sizeof(buf), "pane %ld (%s)", PANE_NUMBER(pane),
		         pane->name);

	return buf;
}

const char *
str_target(int target)
{
	static const char *s[] = {
		"NoTarget", "Forward", "Backward", NULL
	};

	return s[target];
}

const char *
dump_flags(int flags)
{
	static const char *s[] = {
		"FOCUSED", "MAXIMIZED", "MINIMIZED", "MAPPED", "DIRTY",
		"ACTIVE ", "MOVE", "RESIZE", "FOCUS", "UNFOCUS"
	};
	static char buf[256];
	int i;

	buf[0] = '\0';
	for (i = 0; i <= 9; i++) {
		if (flags & (1 << i)) {
			strlcat(buf, s[i], sizeof(buf));
			strlcat(buf, " ", sizeof(buf));
		}
	}

	return buf;
}

void
dump_column(struct column *column)
{
	struct pane *pane;

	TRACE_BEGIN("column at x=%d (n=%d first=%ld last=%ld):",
	            column->x, column->n, PANE_NUMBER(column->first),
	            PANE_NUMBER(column->last));

	for (pane = column->first; pane != NULL; pane = pane->next) {
		TRACE("...%s:", PANE_STR(pane));
		TRACE("......frame=%lx window=%lx prev=%ld next=%ld height=%d ",
		      pane->frame, pane->window,
		      PANE_NUMBER(pane->prev), PANE_NUMBER(pane->next), pane->height);
		TRACE("......flags=%s", dump_flags(pane->flags));
	}
}
