#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <X11/extensions/XKBrules.h>
#include <X11/Xft/Xft.h>
#include <assert.h>
#include <stdio.h>

#ifndef __OpenBSD__
#define strlcat(_dst, _src, _dstsize) \
	snprintf((&_dst[strlen((_dst))]), \
	    (_dstsize) - strlen((_dst)), "%s", _src)
#endif

#if 1
#define WANT_ZERO_BORDERS
#else
#endif

struct layout
{
	GC             normal_gc;
	GC             focus_gc;
	GC             outline_gc;
	GC             bright_gc;
	GC             shadow_gc;
	GC             column_gc;
	struct column *head;
	struct column *tail;

	struct pane   *focus;

	/*
	 * Even though this is same as focus->column, we wish to
	 * allow focus to be NULL e.g. when moving through empty
	 * columns.
	 */
	struct column *column;

	struct pane   *active;
	int            active_x;
	int            active_y;
	int            outline_x;
	int            outline_y;
	bool           has_outline;
	Time           dblclick_time;
	Display       *display;
	XContext       context;

	int            font_width_px;
	int            font_height_px;
	int            titlebar_height_px;

	int            hspacing;
	int            vspacing;
	int            borderwidth;

	XftColor text_fg;
	XftColor text_active_bg;
	XftColor text_inactive_bg;
	XftColor text_cursor;
	XftFont *ftfont;
};

struct prompt {
	char           text[512];
	char          *cursor;
	struct pane   *pane;
	XIM            im;
	XIC            ic;
};

struct pane
{
	Window         frame;
	Window         window;
	Window         close_button;
	Window         maximize_button;
	Window         column_button;

	unsigned int   return_priority;

#define PF_FOCUSED     (1 << 0)
#define PF_KEEP_OPEN   (1 << 1)
#define PF_MINIMIZED   (1 << 2)
#define PF_MAPPED      (1 << 3)
#define PF_DIRTY       (1 << 4)
#define PF_ACTIVE      (1 << 5)
#define PF_MOVE        (1 << 6)
#define PF_RESIZE      (1 << 7)
#define PF_FOCUS       (1 << 8)
#define PF_UNFOCUS     (1 << 9)
#define PF_FULLSCREEN  (1 << 10)
#define PF_HAS_DELWIN  (1 << 11)
#define PF_HAS_TAKEFOCUS (1 << 12)
#define PF_CAPTURE_EXISTING (1 << 13)
#define PF_WANT_RESTART (1 << 14)
#define PF_REPARENTED (1 << 15)
#define PF_HIDDEN (1 << 16)
#define PF_HIDE_OTHERS_LEADER (1 << 17)
#define PF_EDIT (1 << 18)
#define PF_EMPTY (1 << 19)

#define PF_WITHOUT_WINDOW (PF_EMPTY | PF_MINIMIZED | PF_HIDDEN)

	int            flags;

	/* User-adjusted sizing, can be positive or negative. */
	int            y_adj;

	/* Cache of server data */
	int            y;
	int            height;
	int            adjusted_height;
	int            min_height;
	int            max_height;

	char           *name;
	char           *icon_name;

	struct prompt  prompt;

	char           **argv;
	int            argc;

	/* Relatives */
	struct column *column;
	struct pane   *next;
	struct pane   *prev;

	/* Diagnostics */
	unsigned long  number;

	XftDraw *ftdraw;
};

struct column
{
	int            n;
	int            x;
	int            max_height;
	int            width;

	struct pane   *first;
	struct pane   *last;

	struct layout *layout;

#if 0
	struct pane   *focus;
#endif

	struct column *prev;
	struct column *next;
};

struct button
{
	bool         pressed;
	bool         active;
	Window       window;
	struct pane *pane;	
};

struct frame
{
	struct button *close_button;
	struct button *maximize_button;
	struct pane   *pane;
};

void set_ftcolor(Display *dpy, XftColor *dst, int color);
XftFont *font_load(Display *dpy, char *fontname);
int font_draw(XftDraw *ftdraw, Display *dpy, Window window, XftColor fg, XftColor bg, int x, int sx, int y, const char *text, size_t len);

void restart_pane(struct pane *p, Display *d);

struct pane * find_pane_by_window(Window w, struct layout *l);
struct pane * find_previous_focus(struct column *head, struct pane *a);

struct pane *
find_pane_by_vpos(int y, struct column *column);

void
transition_pane_state(struct pane *p, int state, Display *d);
int
read_pane_state(struct pane *p, Display *d);

void read_pane_protocols(struct pane *p, Display *d);

void send_take_focus(struct pane *p, Display *d);

void minimize(struct pane *, struct layout *);
void minimize_others(struct pane *, struct layout *);

void prompt_insert(struct prompt *, char *);
void prompt_init(struct prompt *, struct pane *, struct layout *);
int prompt_read(struct prompt *);

enum action {
	NoAction=0,
	FocusPane,	/* CirculateFocus */
	MovePane,	/* CirculatePane */
	FocusColumn,	/* CirculateColumn */
	MoveColumn,	/* MovePane */
	KillPane,
	RestartManager,
	ToggleTagMode,
	Maximize,
	QuitManager,
	Minimize,
	Fullscreen,
	PrevFocus,
	EditCommand,
	NewCommand,
	RestartCommand,
	ToggleMode
};

enum target {
	NoTarget=0,
	Backward,
	Forward,
	Others
};

/* submit.c */
void submit_pane  (Display *, GC, GC, struct pane *);

void draw_frame(struct pane *p, struct layout *l);
void draw_close_button(struct pane *p, struct layout *l);
void draw_maximize_button(struct pane *p, struct layout *l);
void draw_outline(struct pane *p, int x, int y, struct layout *l);

void
close_pane(struct pane *p, struct layout *l);

/* action.c */
int handle_action (Display *, XContext, int, int, struct layout *);

/* keyboard.c */
int  find_binding (XKeyEvent *, int *);
void bind_keys    (Display *, Window);
void bind_mode_keys    (Display *, Window);
void unbind_keys  (Display *, Window);
void toggle_keybind_mode(Display *);

/* event.c */
int  handle_event   (Display *, XEvent *, XContext, struct layout *);
void interceptmap   (Display *, XContext, Window, int);
void observedestroy (Display *, XContext, Window, struct layout *);

/* layout.c */
#if 0
void           draw_frame         (Display *, Window, const char *, int);
#endif

int            region_height      (Display *, int);
int            region_width       (Display *, int);
int            region_x_org       (Display *, int);
int            region             (Display *, int);
void           init               (Display *, struct layout *, int);
struct pane   *layout_manage      (Display *, XContext, Window, int);
void           layout_draw        (Display *, struct pane *, Bool);
struct column *cycle_column       (struct column *, struct column *, int);
void           draw_all_columns   (Display *, struct layout *);
struct column *find_column        (struct column *, int x);
#if 0
struct pane   *find_pane          (Display *, XContext, Window);
#endif
struct pane   *align_nicely       (struct pane *, struct column *);

struct column *
find_column_by_hpos(int x, struct column *head);

/* pane.c */
#if 0
struct pane* create_pane (Window, Window, int);
#endif

struct pane *create_empty_pane(struct layout *, int);
struct pane * create_pane(Window w, struct layout *l);

struct pane * get_prev_pane(struct pane *pane);
struct pane * get_next_pane(struct pane *pane);

#if 0
struct column * get_prev_column(struct column *column);
struct column * get_next_column(struct column *column);
#endif

struct column *get_column(struct column *, int, int);

/* column.c */
void draw_column        (Display *, GC, GC, struct column *);

void manage_pane           (struct pane *, struct column *, struct pane *);

void remove_pane(struct pane *pane, int);

#if 0
void focus_pane         (struct column *, struct pane *, struct pane *);
#endif

void focus_pane(struct pane *pane, struct layout *layout);
void focus_column(struct column *c, struct layout *l);

struct pane* cycle_focus        (struct column *, int);

void                    resize_add(struct column *, struct pane *);
void                    resize_remove(struct column *, struct pane *);
void                    resize_adjust(struct column *, struct pane *, int);
void                    resize_relayout(struct column *);

void cycle_placement    (struct column *, struct pane *, int);

void			force_one_maximized(struct column *);

#include "trace.h"
