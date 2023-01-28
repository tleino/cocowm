/*
 * cocowm - Column Commander Window Manager for X11 Window System
 * Copyright (c) 2023, Tommi Leino <namhas@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "extern.h"

static int isu8cont(unsigned char);
static int is_empty(struct prompt *);
static int is_full(struct prompt *);
static int handle_keycode(struct prompt *, Display *, XKeyEvent *);

/*
 * Backspace UTF-8.
 */
void
prompt_backspace(struct prompt *p)
{
	do {
		if (is_empty(p))
			break;
		*(p->cursor)-- = '\0';
	} while(isu8cont(*p->cursor));
}

static int
is_empty(struct prompt *p)
{
	return (p->cursor == p->text);
}

static int
isu8cont(unsigned char c)
{
	return (c & (0x80 | 0x40)) == 0x80;
}

/*
 * Insert UTF-8.
 */
void
prompt_insert(struct prompt *p, char *s)
{
	TRACE("Insert char %s", s);
	while (*s != '\0') {
		if (is_full(p))
			break;
		*(p->cursor)++ = *s++;
	}

	*p->cursor = '\0';
	TRACE("text is now %s", p->text);
}

static int
is_full(struct prompt *p)
{
	return ((p->cursor - p->text) == sizeof(p->text) - 2);
}

/*
 * Initialize prompt.
 */
void
prompt_init(struct prompt *p, struct pane *pane, struct layout *layout)
{
	Display *dpy;

	assert(p != NULL);
	assert(pane != NULL);
	assert(layout != NULL);

	TRACE("prompt init");

	p->cursor = &p->text[0];
	p->text[0] = '\0';
	p->pane = pane;

	dpy = layout->display;
	assert(pane->frame != 0);
	p->im = XOpenIM(dpy, NULL, NULL, NULL);
	p->ic = XCreateIC(p->im, XNInputStyle,
	    XIMPreeditNothing | XIMStatusNothing,
	    XNClientWindow, pane->frame, NULL);
}

/*
 * Read text from the prompt.
 */
int
prompt_read(struct prompt *p)
{
	Display *dpy;
	int ret = 1;

	assert(p != NULL);
	assert(p->pane != NULL);

	if (p->pane->column == NULL)
		return 0;

	dpy = p->pane->column->layout->display;

	TRACE("Grabkeyb");
	XGrabKeyboard(dpy, p->pane->frame, True, GrabModeAsync, GrabModeAsync,
	    CurrentTime);
	XSetICFocus(p->ic);
	XSync(dpy, False);

	while (ret == 1) {
		XEvent e;

		XNextEvent(dpy, &e);
		switch (e.type) {
		case KeyPress:
			ret = handle_keycode(p, dpy, &e.xkey);
			if (ret == -1 || ret == 0) {
				p->pane->flags &= ~PF_EDIT;
				if (ret == 0) {
					ret = 1;
					goto exitloop;
				} else
					ret = 0;
			}
			draw_frame(p->pane, p->pane->column->layout);
			break;
		case KeyRelease:
			break;
		case ButtonPress:
		case ButtonRelease:
			ret = 0;
			break;
		default:
			handle_event(dpy, &e, p->pane->column->layout->context,
		                     p->pane->column->layout);
			break;
		}
	}
exitloop:

	draw_frame(p->pane, p->pane->column->layout);
	XUnsetICFocus(p->ic);
	XUngrabKeyboard(dpy, CurrentTime);
	XSync(dpy, False);

	return ret;
}

static int
handle_keycode(struct prompt *p, Display *dpy, XKeyEvent *e)
{
	KeySym sym;
	int n;
	char ch[4 + 1];

	sym = XkbKeycodeToKeysym(dpy, e->keycode, 0,
	    (e->state & ShiftMask) ? 1 : 0);

	switch (sym) {
	case XK_Escape:
		TRACE("Got escape, return -1");
		return -1;
	case XK_Return:
		TRACE("Got return, return 0");
		return 0;
	case XK_BackSpace:
	case XK_Delete:
		prompt_backspace(p);
		break;
	default:
		n = Xutf8LookupString(p->ic, e, ch, sizeof(ch)-1, &sym, NULL);
		if (n > 0) {
			ch[n] = '\0';
			prompt_insert(p, ch);
		}
		break;
	}

	return 1;
}
