#include "extern.h"

/*
 * Manage pane (p) in a column (c) after pane (a).
 */
void
manage_pane(struct pane *p, struct column *c, struct pane *a)
{
	assert(c != NULL);
	assert(p != NULL);
	assert(p->next == NULL);
	assert(p->prev == NULL);
	assert(p->column == NULL);

	TRACE("manage_pane: adding %s after %s in column x=%d",
	      PANE_STR(p), PANE_STR(a), c->x);

	resize_add(c, p);
	p->column = c;

	if (a == NULL) {	/* Add to top */
		p->next = c->first;
		if (c->first)
			c->first->prev = p;
		p->prev = NULL;
	} else {		/* Add after 'after' */
		assert(a != c->last || a->next == NULL);
		p->next = a->next;
		p->prev = a;
		if (a->next)
			a->next->prev = p;
		a->next = p;
	}

	if (p->next == NULL)
		c->last = p;
	if (p->prev == NULL)
		c->first = p;

	p->column = c;
	c->n++;

	assert(c->last != NULL);
	assert(c->first != NULL);
	assert(p->column == c);
	assert(c->n > 0);
	assert(p != c->last || p->next == NULL);
	assert(p != c->first || p->prev == NULL);

	resize_relayout(c);

	TRACE_END("add to column done for pane %ld (after %ld)",
	          PANE_NUMBER(p), PANE_NUMBER(a));
	dump_column(c);
}

/*
 * Removes a pane (p) from its column.
 * Practically the pane will be orphan and not referenced until re-added.
 * If focus is not changed, focus stays the same (focus is
 * column-agnostic).
 */
void
remove_pane(struct pane *p, int want_resize)
{
	struct column *c;

	assert(p != NULL);
	assert(p->column != NULL);

	TRACE_BEGIN("remove_pane: from column begin for pane %ld",
	            PANE_NUMBER(p));

	if (p->column->first == p) {
		p->column->first = p->next;
		if (p->column->first != NULL)
			p->column->first->prev = NULL;
	} else {
		assert(p->prev != NULL);
		p->prev->next = p->next;
	}

	if (p->column->last == p) {
		p->column->last = p->prev;
		if (p->column->last != NULL)
			p->column->last->next = NULL;
	} else {
		assert(p->next != NULL);
		p->next->prev = p->prev;
	}

	p->column->n--;

	p->next = p->prev = NULL;

	assert(p->column->n == 0 || p->column->first != NULL);
	assert(p->column->n == 0 || p->column->last != NULL);

	assert(p->column->n != 0 || p->column->first == NULL);
	assert(p->column->n != 0 || p->column->last == NULL);

	assert(p->column->first != p);
	assert(p->column->last != p);

	assert(p->next == NULL);
	assert(p->prev == NULL);

	TRACE_END("remove from column done for pane %ld",
	          PANE_NUMBER(p));
	dump_column(p->column);

	c = p->column;
	p->column = NULL;

	/*
	 * We need to call resize from outside of this function,
	 * because we might be in middle destroying windows,
	 * which might no longer have a workable Window pointer.
	 */
	resize_remove(c, p);
	if (want_resize == 1) {
		TRACE("remove_pane: want_resize");
		resize_relayout(c);
	} else {
		TRACE("remove_pane: did not want resize");
	}
}
