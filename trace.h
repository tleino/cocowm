/* trace.c */
const char *str_event   (XEvent *);
const char *str_op      (int);
const char *str_pane    (struct pane *pane);
void        dump_column (struct column *);
const char *dump_flags  (int flags);

#define PANE_NUMBER(x) ((x) ? (x)->number : 0)

#ifndef NDEBUG
#define TRACE_LOG(...)  \
	do { \
		fprintf(stderr, "trace: "); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
	} while(0)
#define EVENT_STR(event) str_event(event)
#define OP_STR(op)       str_op(op)
#define TARGET_STR(tgt)  str_target(tgt)
#define PANE_STR(pane)   str_pane(pane)
#else
#define TRACE_LOG(...)   ((void) 0)
#define EVENT_STR(...)   ((void) 0)
#define OP_STR(...)      ((void) 0)
#define TARGET_STR(...)  ((void) 0)
#define PANE_STR(...)    ((void) 0)
#endif

#define TRACE_BEGIN(...) TRACE_LOG("* " __VA_ARGS__)
#define TRACE_END(...)   TRACE_LOG("+ " __VA_ARGS__)
#define TRACE_INFO(...)  TRACE_LOG("  " __VA_ARGS__)
#define TRACE_ERR(...)   TRACE_LOG("! " __VA_ARGS__)
#define TRACE(...)       TRACE_INFO(__VA_ARGS__)
