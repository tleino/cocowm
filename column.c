#include "extern.h"

#include <stdio.h>

struct column *
find_column_by_hpos(int x, struct column *head)
{
	struct column *c;

	for (c = head; c != NULL; c = c->next)
		if (x >= c->x && (x - c->x) < (WSWIDTH + SPACING))
			return c;

	return head;
}

/*
 * Set focus to a column (c) in a layout (l).
 */
void
focus_column(struct column *c, struct layout *l)
{
	assert(c != NULL && l != NULL);

	TRACE("focus column, c->x = %d", c->x);
	l->column = c;
/*
	if (l->focus != NULL)
		l->focus->column = c;
*/
}

/*
 * Returns previous/next column relative to source column (s).
 * Can return empty/non-empty columns based on a number of panes target (n).
 * Returns NULL if there was no columns that matched the target.
 */
struct column *
get_column(struct column *s, int n, int direction)
{
	struct column *d;

	assert(s != NULL);

	d = s;
	do {
		if (direction == Forward)
			d = d->next;
		else
			d = d->prev;

		if (d == NULL) {
			if (direction == Forward)
				d = s->layout->head;
			else
				d = s->layout->tail;
		}

		if (d->n >= n)
			return d;
	} while (d != s);

	return NULL;
}

