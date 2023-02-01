#include "extern.h"

#include <err.h>

static int has_resize_err;

static int
wm_resize_error(Display *display, XErrorEvent *event)
{
	TRACE("error in resizing, should automatically retry");
	has_resize_err = 1;
	return 1;
}

void
force_one_maximized(struct column *ws)
{
	struct pane *np;

	for (np = ws->first; np != NULL; np = np->next) {
		if (np == ws->layout->focus || np->flags & PF_KEEP_OPEN) {
			if (np->flags & PF_MINIMIZED) {
				np->flags &= ~PF_MINIMIZED;
				minimize(np, ws->layout);
				draw_frame(np, ws->layout);
			}
			continue;
		}

		if (np->flags & PF_MINIMIZED)
			continue;

		np->flags |= PF_MINIMIZED;
		minimize(np, ws->layout);
		draw_frame(np, ws->layout);
	}

	resize_relayout(ws->layout->focus->column);
}

/*
 * - Adding a pane results in SHRINKING other panes.
 *   - Unless all other panes are MINIMIZED.
 *
 * - Added pane gets newly calculated 'equal' space.
 *   - Unless it is MINIMIZED, it gets MINIMIZED SIZE.
 *   - Unless MORE is available, it gets MORE.
 *
 * - In the SHRINKING, EQUAL is substracted from panes larger than 'equal'.
 *   - Panes that are less than 'equal' require no action.
 *   - EQUAL is the equally divided requirement for more space.
 *   - No pane is made smaller than 'equal'.
 */
static int calculate_new_equal(struct column *, struct pane *);
static int sum_heights(struct column *, struct pane *);
static void shrink_other_panes(struct column *, struct pane *, int, int);

void
resize_add(struct column *ws, struct pane *pane)
{
	int equal;
	int total;
	int required;
	int toolittle;

	equal = calculate_new_equal(ws, pane);

	if (pane->flags & PF_WITHOUT_WINDOW)
		required = ws->layout->titlebar_height_px;
	else {
		if (pane->adjusted_height > 0 &&
		    pane->adjusted_height < equal)
			required = pane->adjusted_height;
		else
			required = equal;
	}

	TRACE("required %d equal %d", required, equal);

	shrink_other_panes(ws, pane, required, equal);

	pane->height = required;

	/*
	 * Nothing to do anymore if we were minimized.
	 */
	if (pane->flags & PF_WITHOUT_WINDOW)
		return;

	/*
	 * Did we have more space after all? Make our pane larger.
	 */
	total = sum_heights(ws, pane) + required;
	toolittle = ws->max_height - total;

	TRACE("toolittle %d", toolittle);
	if (toolittle > 0)
		pane->height += toolittle;
}

static int
calculate_new_equal(struct column *ws, struct pane *ignore)
{
	struct pane *p;
	int n = 0;
	int minimized_px = 0;
	int total_vspace;

	if (ignore->flags & PF_WITHOUT_WINDOW) {
		minimized_px += ws->layout->titlebar_height_px;
		minimized_px += ws->layout->vspacing;
	} else
		n++;

	for (p = ws->first; p != NULL; p = p->next) {
		if (p == ignore)
			continue;
		if (p->flags & PF_WITHOUT_WINDOW) {
			minimized_px += ws->layout->titlebar_height_px;
			minimized_px += ws->layout->vspacing;
		} else
			n++;
	}

	TRACE("equal n=%d minimized_px=%d", n, minimized_px);
	if (n == 0)
		return (ws->max_height - minimized_px);
	else
		return (ws->max_height - minimized_px - (n * ws->layout->vspacing)) / n;
}

static int
sum_heights(struct column *ws, struct pane *ignore)
{
	struct pane *p;
	int total = 0;

	for (p = ws->first; p != NULL; p = p->next) {
		if (p == ignore)
			continue;
		total += p->height;
	}

	return total;
}

static void
shrink_other_panes(struct column *ws, struct pane *ignore, int required, int equal)
{
	struct pane *p;
	int n = 0;
	int slice;
	int tries = 0;

	if (required <= 0)
		return;

	/*
	 * Shrink panes that are larger than 'equal', shrink down to 'equal'.
	 */
	while (required > 0 && tries-- < 100) {
		for (p = ws->first; p != NULL; p = p->next) {
			if (p == ignore)
				continue;
			if (p->flags & PF_WITHOUT_WINDOW)
				continue;
			if (p->height > equal)
				n++;
		}

		/*
		 * All existing panes minimized or smaller than 'equal' ?
		 */
		if (n == 0)
			return;	/* Nothing to do. */

		for (p = ws->first; p != NULL; p = p->next) {
			if (p == ignore)
				continue;
			if (p->flags & PF_WITHOUT_WINDOW)
				continue;

			slice = required;
			assert(n > 0);
			if (n > 0)
				slice /= n;
			TRACE("shrink candidate n=%d, slice=%d required=%d", n, slice, required);

			if (p->height > equal) {
				TRACE("p->height - equal == %d", p->height - equal);
				TRACE("p->height == %d", p->height);
				if (p->height - equal >= slice) {
					required -= slice;
					p->height -= slice;
					TRACE("sub slice (slice=%d), required now %d, height now %d", slice, required, p->height);
				} else {
					required -= (p->height - equal);
					p->height = equal;
					TRACE("equalize, height is now %d required now %d", p->height, required);
				}
				if (--n == 0) {
					TRACE("required left %d", required);
					break;
				}
			}
		}
	}

	TRACE("required left final %d", required);
	assert(required == 0);
}

/*
 * - Removing a pane results in ENLARGING other panes.
 *   - Unless all other panes are MINIMIZED.
 *
 * - In the ENLARGING, SURPLUS is added evenly to all non-minimized panes.
 */
void
resize_remove(struct column *ws, struct pane *ignore)
{
	struct pane *p;
	int n = 0;
	int surplus;
	int slice;

	surplus = ws->max_height - sum_heights(ws, ignore);
	TRACE("surplus %d", surplus);
	assert(surplus > 0);
	if (surplus <= 0)
		return;

	for (p = ws->first; p != NULL; p = p->next) {
		if (p == ignore)
			continue;
		if (p->flags & PF_WITHOUT_WINDOW)
			continue;
		n++;
	}

	TRACE("surplus n %d", n);

	/*
	 * All are minimized?
	 */
	if (n == 0)
		return;	/* Nothing to do. */

	for (p = ws->first; p != NULL; p = p->next) {
		if (p == ignore)
			continue;
		if (p->flags & PF_WITHOUT_WINDOW)
			continue;

		slice = surplus;
		assert(slice >= 0);
		if (slice <= 0)
			break;

		assert(n > 0);
		if (n > 0)
			slice /= n;

		TRACE("surplus slice %d", slice);

		p->height += slice;
		surplus -= slice;
		if (--n == 0) {
			assert(surplus == 0);
			break;
		}
	}
}

/*
 * What we adjust is the pane above the currently focused pane.
 * - If it becomes smaller (-adj), we make next panes larger.
 * - If it becomes larger (+adj), we make next panes smaller.
 */
void
resize_adjust(struct column *ws, struct pane *pane, int adj)
{
	struct pane *p;
	int sum_min = 0;
	int title;
	int minsz;

	title = ws->layout->titlebar_height_px;
	minsz = title * 3;

	if (pane->flags & PF_WITHOUT_WINDOW) {
		TRACE("adjust minimized pane by %d, not possible, skip", adj);
		return;
	}
	if (pane->height + adj < (title * 3)) {
		TRACE("adjust pane by %d, too small, skip", adj);
		return;
	}

	/*
	 * How much space is required at minimum below this pane?
	 */
	for (p = pane->next; p != NULL; p = p->next) {
		if (p->flags & PF_WITHOUT_WINDOW)
			sum_min += title;
		else
			sum_min += (title * 3);
	}

	TRACE("sum_min %d", sum_min);

	if (pane->y + pane->height + adj + sum_min > ws->max_height) {
		TRACE("adjust pane by %d, too large, skip", adj);
		return;
	}

	pane->height += adj;
	pane->adjusted_height = pane->height;
	TRACE("normal adjust by %d to %d", adj, pane->height);

	if (pane->next == NULL) {
		if (pane->y + pane->height < ws->max_height)
			pane->height += ws->max_height - (pane->y + pane->height);
		pane->adjusted_height = pane->height;
		return;
	}

	for (p = pane->next; p != NULL; p = p->next) {
		if (p->flags & PF_WITHOUT_WINDOW)
			continue;

		if (p->height - adj < minsz) {
			adj -= (p->height - minsz);
			p->height = minsz;
		} else {
			p->height -= adj;
			adj = 0;
		}

		p->adjusted_height = p->height;
		if (adj == 0)
			break;
	}
}

void
resize_relayout(struct column *ws)
{
	struct pane *p;
	int y = 0;

	TRACE("resize relayout ws->n %d", ws->n);

	/*
	 * If we get error here it means we're in middle of destroying
	 * multiple windows, or similar situation, which means we'll get
	 * back to here very soon and have a run without errors.
	 */
	has_resize_err = 0;
	XSetErrorHandler(wm_resize_error);

	for (p = ws->first; p != NULL; p = p->next) {
		XWindowChanges changes;

		p->y = y;

		changes.x = ws->x;
		changes.y = p->y;
		changes.width = ws->width;
		changes.height = p->height;
		changes.stack_mode = Below;

		if (p->flags & PF_FULLSCREEN && p->flags & PF_FOCUSED && !(p->flags & PF_WITHOUT_WINDOW)) {
			changes.x = region_x_org(ws->layout->display, ws->x);
			changes.y = 0;
			changes.width = region_width(ws->layout->display, ws->x);
			changes.height = ws->max_height;
		}

		TRACE("configuring window x=%d y=%d w=%d h=%d", changes.x,
		    changes.y, changes.width, changes.height);

		XConfigureWindow(ws->layout->display, p->frame,
		    CWX | CWY | CWWidth | CWHeight | CWStackMode, &changes);

		TRACE("relayout y=%d height=%d", p->y, p->height);

		changes.x = 0;
		changes.y = ws->layout->titlebar_height_px;
		changes.height = changes.height - ws->layout->titlebar_height_px;
		changes.stack_mode = Above;

		y += p->height + ws->layout->vspacing;

		if (p->flags & PF_WITHOUT_WINDOW || changes.height == 0)
			continue;

		TRACE("configuring subwindow x=%d y=%d w=%d h=%d", changes.x,
		    changes.y, changes.width, changes.height);

		XConfigureWindow(ws->layout->display, p->window,
		    CWX | CWY | CWWidth | CWHeight | CWStackMode, &changes);
	}

	for (p = ws->first; p != NULL; p = p->next) {
		if (!(p->flags & PF_FULLSCREEN))
			continue;
		if (p->flags & PF_WITHOUT_WINDOW)
			continue;
		if (!(p->flags & PF_FOCUSED))
			continue;

		XRaiseWindow(ws->layout->display, p->frame);
	}

	XSync(ws->layout->display, False);
	XSetErrorHandler(None);
}
