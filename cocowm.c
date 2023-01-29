#include "extern.h"

#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xresource.h>
#include <errno.h>
#include <sys/wait.h>

static void capture_existing_windows (struct layout *l);
static void select_root_events       (Display *);
static int  manageable               (Display *, Window);
static int  wm_rights_error          (Display *, XErrorEvent *);

int
main(int argc, char *argv[])
{
	XEvent event;
	static struct layout layout;
	int columns, running;
	struct pane *focus;
	Display *display;
	XContext context;
	char *denv;

	TRACE_BEGIN("start");

	columns = 5;
	if (argc == 2) {
		columns = atoi(argv[1]);
	}
	TRACE("using %d columns", columns);

        if ((denv = getenv("DISPLAY")) == NULL && errno != 0)
                err(1, "getenv");
        if ((display = XOpenDisplay(denv)) == NULL) {
                if (denv == NULL)
                        errx(1, "X11 connection failed; "
                            "DISPLAY environment variable not set?");
                else
                        errx(1, "failed X11 connection to '%s'", denv);
        }

#if 0
	/*
	 * This is required so that we can start stuff..... or not?
	 */
	if ((fcntl(ConnectionNumber(display), F_SETFD, 1)) == -1)
		err(1, "cannot disinherit TCP fd");
#endif

#ifdef __OpenBSD__
#if 1
	if (pledge("stdio rpath proc exec", NULL) == -1)
		err(1, "pledge");
#else
	if (pledge("stdio rpath", NULL) == -1)
		err(1, "pledge");
#endif
#endif	

	context = XUniqueContext();

	init(display, &layout, columns);

	layout.display = display;
	layout.context = context;

	select_root_events(display);
	capture_existing_windows(&layout);

#ifdef STICKYKEYS
	bind_mode_keys(display, DefaultRootWindow(display));
#else
	bind_keys(display, DefaultRootWindow(display));
#endif

	XSync(display, False);

	running = 1;
	focus = NULL;
	while (running) {
		int status;

#ifndef WAIT_MYPGRP
#define WAIT_MYPGRP 0
#endif
		/*
		 * Reap zombie processes.
		 */
		waitpid(WAIT_MYPGRP, &status, WNOHANG);

		XNextEvent(display, &event);
		switch (handle_event(display, &event, context,
		                     &layout)) {
		case RestartManager:
			execvp(*argv, argv);
			err(1, "restarting");
			break;
		case QuitManager:
			running = 0;
			break;

		}
	}

	XSync(display, False);
	XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
	XCloseDisplay(display);
	return 0;
}

static void
select_root_events(Display *display)
{
	XSync(display, False);
	XSetErrorHandler(wm_rights_error);

	XSelectInput(display, DefaultRootWindow(display),
	             SubstructureNotifyMask |
	             SubstructureRedirectMask |
	             KeyReleaseMask /* |
	             ExposureMask */);

	XSync(display, False);
	XSetErrorHandler(None);
}

static int
wm_rights_error(Display *display, XErrorEvent *event)
{
	errx(1, "couldn't obtain window manager rights, "
	     "is there another running?");
	return 1;
}

static void
capture_existing_windows(struct layout *l)
{
	Window            root, parent, *children;
	int               i;
	unsigned int      nchildren;
#if 0
	XWindowAttributes  attrib;
#endif
	struct pane       *p;

	TRACE_BEGIN("capturing windows");

	if (XQueryTree(l->display, DefaultRootWindow(l->display),
	               &root, &parent, &children, &nchildren) == 0)
		errx(1, "couldn't capture existing windows");

	for (i = 0; i < nchildren; i++) {
		/*
		 * TODO: remove manageable() check because this check
		 *       is already done in create_pane()
		 */
		if (manageable(l->display, children[i])) {
#if 0
			XUnmapWindow(display, children[i]);
#endif

#if 0
			if (XGetWindowAttributes(l->display, children[i],
			                         &attrib) == 0)
				assert(0);
#endif

			if ((p = create_pane(children[i], l)) != NULL) {
				p->flags |= PF_CAPTURE_EXISTING;

				TRACE_END("captured %lx, created %s",
				    children[i], PANE_STR(p));
			}

#if 0
			interceptmap(display, context, children[i], attrib.x);
#endif
		} else {
			TRACE_ERR("did not capture %lx", children[i]);
		}
	}

	if (nchildren > 0)
		XFree(children);
}

static int
manageable(Display *d, Window w)
{
	XWindowAttributes wa;
	int mapped, redirectable;

	XGetWindowAttributes(d, w, &wa);
	mapped = (wa.map_state != IsUnmapped);
	redirectable = (wa.override_redirect != True);

	return (mapped && redirectable);
}
