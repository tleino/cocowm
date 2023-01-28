#include "extern.h"

#include <X11/keysym.h>

#include <err.h>

#define DONT_CHECK_MASK -1

struct binding {
	KeySym keysym;
	int modifiermask;
	int op;
	int target;
};

#if 0
#define STICKYKEYS
#endif

/*
 * Assuming normal PC keyboard where Mod4Mask corresponds to Win key,
 * and Mod1Mask to Alt key.
 */
#ifndef STICKYKEYS
static const struct binding keybinding[] = {
/* Focus */
	{ XK_Right, Mod4Mask,                FocusColumn,    Forward },
	{ XK_Left,  Mod4Mask,                FocusColumn,    Backward },
	{ XK_Down,  Mod4Mask,                FocusPane,      Forward },
	{ XK_Up,    Mod4Mask,                FocusPane,      Backward },
	{ XK_Tab,   Mod4Mask,                PrevFocus,      0 },

/* Moving */
	{ XK_Right, Mod4Mask | ControlMask,  MoveColumn,     Forward },
	{ XK_Left,  Mod4Mask | ControlMask,  MoveColumn,     Backward },
	{ XK_Down,  Mod4Mask | ControlMask,  MovePane,       Forward },
	{ XK_Up,    Mod4Mask | ControlMask,  MovePane,       Backward },

/* Actions */
	{ XK_o,     Mod4Mask,                Maximize,       0 },
	{ XK_m,     Mod4Mask,                Minimize,       0 },
	{ XK_h,     Mod4Mask,                Minimize,       Others },
	{ XK_f,     Mod4Mask,                Fullscreen,     0 },
	{ XK_r,     Mod4Mask,                RestartCommand, 0 },
	{ XK_space, Mod4Mask,                EditCommand,    0 },
	{ XK_q,     Mod4Mask,                KillPane,       0 },

/* Window manager */
	{ XK_F11,   0,                       RestartManager, 0 },
	{ XK_F12,   0,                       QuitManager,    0 },
	{ 0 }
};
#else
static const struct binding keybinding[] = {
	{ XK_F3,    0,                       KillPane,       0 },
	{ XK_F4,    0,                       RestartManager, 0 },
	{ XK_Right, 0,               FocusColumn,    Forward },
	{ XK_Left,  0,               FocusColumn,    Backward },
	{ XK_Down,  0,               FocusPane,      Forward },
	{ XK_Up,    0,               FocusPane,      Backward },
	{ XK_Right, ControlMask, MoveColumn,     Forward },
	{ XK_Left,  ControlMask, MoveColumn,     Backward },
	{ XK_Down,  ControlMask, MovePane,       Forward },
	{ XK_Up,    ControlMask, MovePane,       Backward },
	{ XK_F2,    0,                       Maximize,       0 },
	{ XK_F12,   0,                       QuitManager,    0 },
	{ XK_F1,    0,                       Minimize,       0 },
	{ XK_f,     ControlMask, Fullscreen,     0 },
	{ XK_Tab,   0,                PrevFocus,      0 },
	{ XK_F5,    0,                       RestartCommand, 0 },
	{ XK_Super_R, DONT_CHECK_MASK, ToggleMode, 0 },
	{ XK_Super_L, DONT_CHECK_MASK, ToggleMode, 0 },
	{ 0 }
};
#endif

#ifdef STICKYKEYS
static const struct binding modebind[] = {
	{ XK_Super_R, DONT_CHECK_MASK, ToggleMode, 0 },
	{ XK_Super_L, DONT_CHECK_MASK, ToggleMode, 0 },
	{ 0 }
};
static int _mode;
#endif

static void _unbind_keys(Display *, Window, const struct binding *);
static void _bind_keys(Display *, Window, const struct binding *);
static int _find_binding(XKeyEvent *, int *target, const struct binding *);

#include <stdlib.h>

#ifdef STICKYKEYS
void toggle_keybind_mode(Display *display)
{
	TRACE("togglemode %d", _mode);
	_mode ^= 1;
	TRACE("togglemode now %d", _mode);
	if (_mode == 1) {
		_unbind_keys(display, DefaultRootWindow(display), modebind);
		bind_keys(display, DefaultRootWindow(display));
		system("xsetroot -solid red");
	} else {
		unbind_keys(display, DefaultRootWindow(display));
		_bind_keys(display, DefaultRootWindow(display), modebind);
		system("xsetroot -solid black");
	}
}
#endif

int
find_binding(XKeyEvent *xkey, int *target)
{
#ifdef STICKYKEYS
	if (_mode == 0) {
		TRACE("finding modebind");
		return _find_binding(xkey, target, modebind);
	}
#endif

	TRACE("finding keybinding");
	return _find_binding(xkey, target, keybinding);
}

static int
_find_binding(XKeyEvent *xkey, int *target, const struct binding *bindings)
{
	int                   i, mask;
	KeySym                sym;
	const struct binding *kb;

	sym = XLookupKeysym(xkey, 0);
	mask = xkey->state;
	for (i = 0; bindings[i].keysym != 0; i++) {
		kb = &(bindings[i]);
		TRACE("checking binding sym vs sym %d %d", (int) kb->keysym, (int) sym);
		TRACE("checking mask vs mask %d %d", (int) kb->modifiermask, (int) mask);
		if (kb->keysym == sym &&
		   (kb->modifiermask == mask ||
		   kb->modifiermask == DONT_CHECK_MASK)) {
			*target = kb->target;
			return kb->op;
		}
	}

	warnx("binding not found");
	return -1;
}

static void
_bind_keys(Display *display, Window root, const struct binding *bindings)
{
	int i, code, mask, mouse, kbd;
	Bool owner_events;

	for (i = 0; bindings[i].keysym != 0; i++) {
		code = XKeysymToKeycode(display, bindings[i].keysym);
		if (bindings[i].modifiermask == DONT_CHECK_MASK)
			mask = 0;
		else
			mask = bindings[i].modifiermask;
		mouse = GrabModeSync;
		kbd = GrabModeAsync;
		owner_events = False;
		XGrabKey(display, code, mask, root, owner_events, mouse, kbd);
	}
}

static void
_unbind_keys(Display *display, Window root, const struct binding *bindings)
{
	int i, code, mask;

	for (i = 0; bindings[i].keysym != 0; i++) {
		code = XKeysymToKeycode(display, bindings[i].keysym);
		if (bindings[i].modifiermask == DONT_CHECK_MASK)
			mask = 0;
		else
			mask = bindings[i].modifiermask;
		XUngrabKey(display, code, mask, root);
	}
}

void
unbind_keys(Display *display, Window root)
{
	_unbind_keys(display, root, keybinding);
}

void
bind_keys(Display *display, Window root)
{
	_bind_keys(display, root, keybinding);
}

#ifdef STICKYKEYS
void
bind_mode_keys(Display *display, Window root)
{
	_bind_keys(display, root, modebind);
}
#endif
