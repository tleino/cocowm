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

#include <err.h>

struct history {
	char *line;
	struct history *prev;
	struct history *next;
};

static struct history *head;
static struct history *tail;

static char *file = ".cocowm_history";

static const char *history_path();

void history_save();

static const char *
history_path()
{
	const char *home;
	static char s[PATH_MAX];

	home = getenv("HOME");
	if (home == NULL)
		home = "/";

	if (snprintf(s, sizeof(s), "%s/%s", home, file) >= sizeof(s))
		err(1, "history path name overflow");

	return s;
}

const char *
history_match(const char *s)
{
	ssize_t i;
	size_t best_pts = 0, pts;
	const char *best_match = NULL, *p, *q;
	struct history *h;

	if (s == NULL)
		return NULL;

	if (head == NULL)
		history_load();

	for (h = head; h != NULL; h = h->next) {
		q = s;
		p = h->line;
		pts = 0;
		while (*p != '\0' && *q != '\0')
			if (*p++ == *q++)
				pts++;
			else {
				pts = 0;
				break;
			}
		if (*p == '\0' && *q != '\0')
			pts = 0;
		if (pts > best_pts) {
			best_pts = pts;
			best_match = h->line;
		}
	}

	return best_match;	
}

void
history_remove(const char *s)
{
	struct history *h;

	for (h = head; h != NULL; h = h->next)
		if (strcmp(h->line, s) == 0)
			break;
	if (h == NULL)
		return;

	if (h->prev)
		h->prev->next = h->next;
	if (h->next)
		h->next->prev = h->prev;
	if (h == head)
		head = h->next;
	if (h == tail)
		tail = h->prev;

	free(h->line);
	free(h);
}

void
history_add(const char *s)
{
	struct history *h;

	history_remove(s);

	h = calloc(1, sizeof(*h));
	if (h == NULL) {
		warn("saving history");
		return;
	}

	h->line = strdup(s);
	if (h->line == NULL) {
		free(h);
		warn("strdup history");
		return;
	}

	h->next = head;
	if (h->next != NULL)
		h->next->prev = h;
	head = h;
	if (tail == NULL)
		tail = h;

	history_save();
}

void
history_load()
{
	FILE *fp;
	char *line = NULL;
	size_t sz = 0;
	ssize_t len;
	const char *s;

	s = history_path();
	fp = fopen(s, "r");
	if (fp == NULL) {
		warnx("fopen history for reading");
		return;
	}

	while ((len = getline(&line, &sz, fp)) > 0) {
		line[strcspn(line, "\r\n")] = '\0';
		history_add(line);
	}
	if (ferror(fp))
		warnx("getline %s", s);

	if (line != NULL)
		free(line);
}

void
history_save()
{
	FILE *fp;
	struct history *h;
	const char *s;

	s = history_path();
	fp = fopen(s, "w");
	if (fp == NULL)
		err(1, "fopen history for writing");

	for (h = tail; h != NULL; h = h->prev)
		fprintf(fp, "%s\n", h->line);

	if (ferror(fp))
		warn("%s", s);
	fclose(fp);
}
