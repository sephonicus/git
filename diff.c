#include "submodule.h"
#include "ll-merge.h"
static int diff_no_prefix;
static struct diff_options default_diff_options;
	GIT_COLOR_NORMAL,	/* FUNCINFO */
	if (!strcasecmp(var+ofs, "func"))
		return DIFF_FUNCINFO;
	return -1;
	if (!strcmp(var, "diff.noprefix")) {
		diff_no_prefix = git_config_bool(var, value);
		return 0;
	}
	if (!strcmp(var, "diff.ignoresubmodules"))
		handle_ignore_submodules_arg(&default_diff_options, value);

		if (slot < 0)
			return 0;
	if (!prefixcmp(var, "submodule."))
		return parse_submodule_config_option(var, value);

typedef unsigned long (*sane_truncate_fn)(char *line, unsigned long len);

struct emit_callback {
	int color_diff;
	unsigned ws_rule;
	int blank_at_eof_in_preimage;
	int blank_at_eof_in_postimage;
	int lno_in_preimage;
	int lno_in_postimage;
	sane_truncate_fn truncate;
	const char **label_path;
	struct diff_words_data *diff_words;
	struct diff_options *opt;
	int *found_changesp;
	struct strbuf *header;
};

static int count_lines(const char *data, int size)
{
	int count, ch, completely_empty = 1, nl_just_seen = 0;
	count = 0;
	while (0 < size--) {
		ch = *data++;
		if (ch == '\n') {
			count++;
			nl_just_seen = 1;
			completely_empty = 0;
		}
		else {
			nl_just_seen = 0;
			completely_empty = 0;
		}
	}
	if (completely_empty)
		return 0;
	if (!nl_just_seen)
		count++; /* no trailing newline */
	return count;
}

static int fill_mmfile(mmfile_t *mf, struct diff_filespec *one)
{
	if (!DIFF_FILE_VALID(one)) {
		mf->ptr = (char *)""; /* does not matter */
		mf->size = 0;
		return 0;
	}
	else if (diff_populate_filespec(one, 0))
		return -1;

	mf->ptr = one->data;
	mf->size = one->size;
	return 0;
}

static int count_trailing_blank(mmfile_t *mf, unsigned ws_rule)
{
	char *ptr = mf->ptr;
	long size = mf->size;
	int cnt = 0;

	if (!size)
		return cnt;
	ptr += size - 1; /* pointing at the very end */
	if (*ptr != '\n')
		; /* incomplete line */
	else
		ptr--; /* skip the last LF */
	while (mf->ptr < ptr) {
		char *prev_eol;
		for (prev_eol = ptr; mf->ptr <= prev_eol; prev_eol--)
			if (*prev_eol == '\n')
				break;
		if (!ws_blank_line(prev_eol + 1, ptr - prev_eol, ws_rule))
			break;
		cnt++;
		ptr = prev_eol - 1;
	}
	return cnt;
}

static void check_blank_at_eof(mmfile_t *mf1, mmfile_t *mf2,
			       struct emit_callback *ecbdata)
{
	int l1, l2, at;
	unsigned ws_rule = ecbdata->ws_rule;
	l1 = count_trailing_blank(mf1, ws_rule);
	l2 = count_trailing_blank(mf2, ws_rule);
	if (l2 <= l1) {
		ecbdata->blank_at_eof_in_preimage = 0;
		ecbdata->blank_at_eof_in_postimage = 0;
		return;
	}
	at = count_lines(mf1->ptr, mf1->size);
	ecbdata->blank_at_eof_in_preimage = (at - l1) + 1;

	at = count_lines(mf2->ptr, mf2->size);
	ecbdata->blank_at_eof_in_postimage = (at - l2) + 1;
}

static void emit_line_0(struct diff_options *o, const char *set, const char *reset,
			int first, const char *line, int len)
{
	int has_trailing_newline, has_trailing_carriage_return;
	int nofirst;
	FILE *file = o->file;

	if (o->output_prefix) {
		struct strbuf *msg = NULL;
		msg = o->output_prefix(o, o->output_prefix_data);
		assert(msg);
		fwrite(msg->buf, msg->len, 1, file);
	}

	if (len == 0) {
		has_trailing_newline = (first == '\n');
		has_trailing_carriage_return = (!has_trailing_newline &&
						(first == '\r'));
		nofirst = has_trailing_newline || has_trailing_carriage_return;
	} else {
		has_trailing_newline = (len > 0 && line[len-1] == '\n');
		if (has_trailing_newline)
			len--;
		has_trailing_carriage_return = (len > 0 && line[len-1] == '\r');
		if (has_trailing_carriage_return)
			len--;
		nofirst = 0;
	}

	if (len || !nofirst) {
		fputs(set, file);
		if (!nofirst)
			fputc(first, file);
		fwrite(line, len, 1, file);
		fputs(reset, file);
	}
	if (has_trailing_carriage_return)
		fputc('\r', file);
	if (has_trailing_newline)
		fputc('\n', file);
}

static void emit_line(struct diff_options *o, const char *set, const char *reset,
		      const char *line, int len)
{
	emit_line_0(o, set, reset, line[0], line+1, len-1);
}

static int new_blank_line_at_eof(struct emit_callback *ecbdata, const char *line, int len)
{
	if (!((ecbdata->ws_rule & WS_BLANK_AT_EOF) &&
	      ecbdata->blank_at_eof_in_preimage &&
	      ecbdata->blank_at_eof_in_postimage &&
	      ecbdata->blank_at_eof_in_preimage <= ecbdata->lno_in_preimage &&
	      ecbdata->blank_at_eof_in_postimage <= ecbdata->lno_in_postimage))
		return 0;
	return ws_blank_line(line, len, ecbdata->ws_rule);
}

static void emit_add_line(const char *reset,
			  struct emit_callback *ecbdata,
			  const char *line, int len)
{
	const char *ws = diff_get_color(ecbdata->color_diff, DIFF_WHITESPACE);
	const char *set = diff_get_color(ecbdata->color_diff, DIFF_FILE_NEW);

	if (!*ws)
		emit_line_0(ecbdata->opt, set, reset, '+', line, len);
	else if (new_blank_line_at_eof(ecbdata, line, len))
		/* Blank line at EOF - paint '+' as well */
		emit_line_0(ecbdata->opt, ws, reset, '+', line, len);
	else {
		/* Emit just the prefix, then the rest. */
		emit_line_0(ecbdata->opt, set, reset, '+', "", 0);
		ws_check_emit(line, len, ecbdata->ws_rule,
			      ecbdata->opt->file, set, reset, ws);
	}
}

static void emit_hunk_header(struct emit_callback *ecbdata,
			     const char *line, int len)
{
	const char *plain = diff_get_color(ecbdata->color_diff, DIFF_PLAIN);
	const char *frag = diff_get_color(ecbdata->color_diff, DIFF_FRAGINFO);
	const char *func = diff_get_color(ecbdata->color_diff, DIFF_FUNCINFO);
	const char *reset = diff_get_color(ecbdata->color_diff, DIFF_RESET);
	static const char atat[2] = { '@', '@' };
	const char *cp, *ep;
	struct strbuf msgbuf = STRBUF_INIT;
	int org_len = len;
	int i = 1;

	/*
	 * As a hunk header must begin with "@@ -<old>, +<new> @@",
	 * it always is at least 10 bytes long.
	 */
	if (len < 10 ||
	    memcmp(line, atat, 2) ||
	    !(ep = memmem(line + 2, len - 2, atat, 2))) {
		emit_line(ecbdata->opt, plain, reset, line, len);
		return;
	}
	ep += 2; /* skip over @@ */

	/* The hunk header in fraginfo color */
	strbuf_add(&msgbuf, frag, strlen(frag));
	strbuf_add(&msgbuf, line, ep - line);
	strbuf_add(&msgbuf, reset, strlen(reset));

	/*
	 * trailing "\r\n"
	 */
	for ( ; i < 3; i++)
		if (line[len - i] == '\r' || line[len - i] == '\n')
			len--;

	/* blank before the func header */
	for (cp = ep; ep - line < len; ep++)
		if (*ep != ' ' && *ep != '\t')
			break;
	if (ep != cp) {
		strbuf_add(&msgbuf, plain, strlen(plain));
		strbuf_add(&msgbuf, cp, ep - cp);
		strbuf_add(&msgbuf, reset, strlen(reset));
	}

	if (ep < line + len) {
		strbuf_add(&msgbuf, func, strlen(func));
		strbuf_add(&msgbuf, ep, line + len - ep);
		strbuf_add(&msgbuf, reset, strlen(reset));
	}

	strbuf_add(&msgbuf, line + len, org_len - len);
	emit_line(ecbdata->opt, "", "", msgbuf.buf, msgbuf.len);
	strbuf_release(&msgbuf);
}

static void emit_rewrite_lines(struct emit_callback *ecb,
			       int prefix, const char *data, int size)
	const char *endp = NULL;
	static const char *nneof = " No newline at end of file\n";
	const char *old = diff_get_color(ecb->color_diff, DIFF_FILE_OLD);
	const char *reset = diff_get_color(ecb->color_diff, DIFF_RESET);

	while (0 < size) {
		int len;

		endp = memchr(data, '\n', size);
		len = endp ? (endp - data + 1) : size;
		if (prefix != '+') {
			ecb->lno_in_preimage++;
			emit_line_0(ecb->opt, old, reset, '-',
				    data, len);
		} else {
			ecb->lno_in_postimage++;
			emit_add_line(reset, ecb, data, len);
		size -= len;
		data += len;
	}
	if (!endp) {
		const char *plain = diff_get_color(ecb->color_diff,
						   DIFF_PLAIN);
		emit_line_0(ecb->opt, plain, reset, '\\',
			    nneof, strlen(nneof));
			      struct userdiff_driver *textconv_one,
			      struct userdiff_driver *textconv_two,
	char *data_one, *data_two;
	struct emit_callback ecbdata;
	char *line_prefix = "";
	struct strbuf *msgbuf;

	if (o && o->output_prefix) {
		msgbuf = o->output_prefix(o, o->output_prefix_data);
		line_prefix = msgbuf->buf;
	}
	size_one = fill_textconv(textconv_one, one, &data_one);
	size_two = fill_textconv(textconv_two, two, &data_two);

	memset(&ecbdata, 0, sizeof(ecbdata));
	ecbdata.color_diff = color_diff;
	ecbdata.found_changesp = &o->found_changes;
	ecbdata.ws_rule = whitespace_rule(name_b ? name_b : name_a);
	ecbdata.opt = o;
	if (ecbdata.ws_rule & WS_BLANK_AT_EOF) {
		mmfile_t mf1, mf2;
		mf1.ptr = (char *)data_one;
		mf2.ptr = (char *)data_two;
		mf1.size = size_one;
		mf2.size = size_two;
		check_blank_at_eof(&mf1, &mf2, &ecbdata);
	ecbdata.lno_in_preimage = 1;
	ecbdata.lno_in_postimage = 1;
		"%s%s--- %s%s%s\n%s%s+++ %s%s%s\n%s%s@@ -",
		line_prefix, metainfo, a_name.buf, name_a_tab, reset,
		line_prefix, metainfo, b_name.buf, name_b_tab, reset,
		line_prefix, fraginfo);
		emit_rewrite_lines(&ecbdata, '-', data_one, size_one);
		emit_rewrite_lines(&ecbdata, '+', data_two, size_two);
	if (textconv_one)
		free((char *)data_one);
	if (textconv_two)
		free((char *)data_two);
struct diff_words_style_elem
{
	const char *prefix;
	const char *suffix;
	const char *color; /* NULL; filled in by the setup code if
			    * color is enabled */
};

struct diff_words_style
{
	enum diff_words_type type;
	struct diff_words_style_elem new, old, ctx;
	const char *newline;
};

struct diff_words_style diff_words_styles[] = {
	{ DIFF_WORDS_PORCELAIN, {"+", "\n"}, {"-", "\n"}, {" ", "\n"}, "~\n" },
	{ DIFF_WORDS_PLAIN, {"{+", "+}"}, {"[-", "-]"}, {"", ""}, "\n" },
	{ DIFF_WORDS_COLOR, {"", ""}, {"", ""}, {"", ""}, "\n" }
};

	int last_minus;
	struct diff_options *opt;
	enum diff_words_type type;
	struct diff_words_style *style;
static int fn_out_diff_words_write_helper(FILE *fp,
					  struct diff_words_style_elem *st_el,
					  const char *newline,
					  size_t count, const char *buf,
					  const char *line_prefix)
{
	int print = 0;

	while (count) {
		char *p = memchr(buf, '\n', count);
		if (print)
			fputs(line_prefix, fp);
		if (p != buf) {
			if (st_el->color && fputs(st_el->color, fp) < 0)
				return -1;
			if (fputs(st_el->prefix, fp) < 0 ||
			    fwrite(buf, p ? p - buf : count, 1, fp) != 1 ||
			    fputs(st_el->suffix, fp) < 0)
				return -1;
			if (st_el->color && *st_el->color
			    && fputs(GIT_COLOR_RESET, fp) < 0)
				return -1;
		}
		if (!p)
			return 0;
		if (fputs(newline, fp) < 0)
			return -1;
		count -= p + 1 - buf;
		buf = p + 1;
		print = 1;
	}
	return 0;
}

/*
 * '--color-words' algorithm can be described as:
 *
 *   1. collect a the minus/plus lines of a diff hunk, divided into
 *      minus-lines and plus-lines;
 *
 *   2. break both minus-lines and plus-lines into words and
 *      place them into two mmfile_t with one word for each line;
 *
 *   3. use xdiff to run diff on the two mmfile_t to get the words level diff;
 *
 * And for the common parts of the both file, we output the plus side text.
 * diff_words->current_plus is used to trace the current position of the plus file
 * which printed. diff_words->last_minus is used to trace the last minus word
 * printed.
 *
 * For '--graph' to work with '--color-words', we need to output the graph prefix
 * on each line of color words output. Generally, there are two conditions on
 * which we should output the prefix.
 *
 *   1. diff_words->last_minus == 0 &&
 *      diff_words->current_plus == diff_words->plus.text.ptr
 *
 *      that is: the plus text must start as a new line, and if there is no minus
 *      word printed, a graph prefix must be printed.
 *
 *   2. diff_words->current_plus > diff_words->plus.text.ptr &&
 *      *(diff_words->current_plus - 1) == '\n'
 *
 *      that is: a graph prefix must be printed following a '\n'
 */
static int color_words_output_graph_prefix(struct diff_words_data *diff_words)
{
	if ((diff_words->last_minus == 0 &&
		diff_words->current_plus == diff_words->plus.text.ptr) ||
		(diff_words->current_plus > diff_words->plus.text.ptr &&
		*(diff_words->current_plus - 1) == '\n')) {
		return 1;
	} else {
		return 0;
	}
}

	struct diff_words_style *style = diff_words->style;
	struct diff_options *opt = diff_words->opt;
	struct strbuf *msgbuf;
	char *line_prefix = "";
	assert(opt);
	if (opt->output_prefix) {
		msgbuf = opt->output_prefix(opt, opt->output_prefix_data);
		line_prefix = msgbuf->buf;
	}

	if (color_words_output_graph_prefix(diff_words)) {
		fputs(line_prefix, diff_words->opt->file);
	}
	if (diff_words->current_plus != plus_begin) {
		fn_out_diff_words_write_helper(diff_words->opt->file,
				&style->ctx, style->newline,
				plus_begin - diff_words->current_plus,
				diff_words->current_plus, line_prefix);
		if (*(plus_begin - 1) == '\n')
			fputs(line_prefix, diff_words->opt->file);
	}
	if (minus_begin != minus_end) {
		fn_out_diff_words_write_helper(diff_words->opt->file,
				&style->old, style->newline,
				minus_end - minus_begin, minus_begin,
				line_prefix);
	}
	if (plus_begin != plus_end) {
		fn_out_diff_words_write_helper(diff_words->opt->file,
				&style->new, style->newline,
				plus_end - plus_begin, plus_begin,
				line_prefix);
	}
	diff_words->last_minus = minus_first;
	struct diff_words_style *style = diff_words->style;

	struct diff_options *opt = diff_words->opt;
	struct strbuf *msgbuf;
	char *line_prefix = "";

	assert(opt);
	if (opt->output_prefix) {
		msgbuf = opt->output_prefix(opt, opt->output_prefix_data);
		line_prefix = msgbuf->buf;
	}
		fputs(line_prefix, diff_words->opt->file);
		fn_out_diff_words_write_helper(diff_words->opt->file,
			&style->old, style->newline,
			diff_words->minus.text.size,
			diff_words->minus.text.ptr, line_prefix);
	diff_words->last_minus = 0;
	xpp.flags = 0;
		      &xpp, &xecfg);
			diff_words->plus.text.size) {
		if (color_words_output_graph_prefix(diff_words))
			fputs(line_prefix, diff_words->opt->file);
		fn_out_diff_words_write_helper(diff_words->opt->file,
			&style->ctx, style->newline,
			- diff_words->current_plus, diff_words->current_plus,
			line_prefix);
	}
/* In "color-words" mode, show word-diff of words accumulated in the buffer */
static void diff_words_flush(struct emit_callback *ecbdata)
{
	if (ecbdata->diff_words->minus.text.size ||
	    ecbdata->diff_words->plus.text.size)
		diff_words_show(ecbdata->diff_words);
}
		diff_words_flush(ecbdata);
		if (ecbdata->diff_words->word_regex) {
			regfree(ecbdata->diff_words->word_regex);
			free(ecbdata->diff_words->word_regex);
		}
static void find_lno(const char *line, struct emit_callback *ecbdata)
{
	const char *p;
	ecbdata->lno_in_preimage = 0;
	ecbdata->lno_in_postimage = 0;
	p = strchr(line, '-');
	if (!p)
		return; /* cannot happen */
	ecbdata->lno_in_preimage = strtol(p + 1, NULL, 10);
	p = strchr(p, '+');
	if (!p)
		return; /* cannot happen */
	ecbdata->lno_in_postimage = strtol(p + 1, NULL, 10);
}

	struct diff_options *o = ecbdata->opt;
	char *line_prefix = "";
	struct strbuf *msgbuf;

	if (o && o->output_prefix) {
		msgbuf = o->output_prefix(o, o->output_prefix_data);
		line_prefix = msgbuf->buf;
	}
	if (ecbdata->header) {
		fprintf(ecbdata->opt->file, "%s", ecbdata->header->buf);
		strbuf_reset(ecbdata->header);
		ecbdata->header = NULL;
	}
		fprintf(ecbdata->opt->file, "%s%s--- %s%s%s\n",
			line_prefix, meta, ecbdata->label_path[0], reset, name_a_tab);
		fprintf(ecbdata->opt->file, "%s%s+++ %s%s%s\n",
			line_prefix, meta, ecbdata->label_path[1], reset, name_b_tab);
	if (line[0] == '@') {
		if (ecbdata->diff_words)
			diff_words_flush(ecbdata);
		find_lno(line, ecbdata);
		emit_hunk_header(ecbdata, line, len);
			putc('\n', ecbdata->opt->file);
	if (len < 1) {
		emit_line(ecbdata->opt, reset, reset, line, len);
		if (ecbdata->diff_words
		    && ecbdata->diff_words->type == DIFF_WORDS_PORCELAIN)
			fputs("~\n", ecbdata->opt->file);
		diff_words_flush(ecbdata);
		if (ecbdata->diff_words->type == DIFF_WORDS_PORCELAIN) {
			emit_line(ecbdata->opt, plain, reset, line, len);
			fputs("~\n", ecbdata->opt->file);
		} else {
			/* don't print the prefix character */
			emit_line(ecbdata->opt, plain, reset, line+1, len-1);
		}
	if (line[0] != '+') {
		const char *color =
			diff_get_color(ecbdata->color_diff,
				       line[0] == '-' ? DIFF_FILE_OLD : DIFF_PLAIN);
		ecbdata->lno_in_preimage++;
		if (line[0] == ' ')
			ecbdata->lno_in_postimage++;
		emit_line(ecbdata->opt, color, reset, line, len);
	} else {
		ecbdata->lno_in_postimage++;
		emit_add_line(reset, ecbdata, line + 1, len - 1);
		uintmax_t added, deleted;
	uintmax_t max_change = 0, max_len = 0;
	const char *line_prefix = "";
	struct strbuf *msg = NULL;
	if (options->output_prefix) {
		msg = options->output_prefix(options, options->output_prefix_data);
		line_prefix = msg->buf;
	}

		uintmax_t change = file->added + file->deleted;
		uintmax_t added = data->files[i]->added;
		uintmax_t deleted = data->files[i]->deleted;
			fprintf(options->file, "%s", line_prefix);
			fprintf(options->file, "%s%"PRIuMAX"%s",
				del_c, deleted, reset);
			fprintf(options->file, "%s%"PRIuMAX"%s",
				add_c, added, reset);
			fprintf(options->file, "%s", line_prefix);
		fprintf(options->file, "%s", line_prefix);
		fprintf(options->file, "%5"PRIuMAX"%s", added + deleted,
	fprintf(options->file, "%s", line_prefix);
static void show_shortstats(struct diffstat_t *data, struct diff_options *options)
	if (options->output_prefix) {
		struct strbuf *msg = NULL;
		msg = options->output_prefix(options,
				options->output_prefix_data);
		fprintf(options->file, "%s", msg->buf);
	}
		if (options->output_prefix) {
			struct strbuf *msg = NULL;
			msg = options->output_prefix(options,
					options->output_prefix_data);
			fprintf(options->file, "%s", msg->buf);
		}

				"%"PRIuMAX"\t%"PRIuMAX"\t",
				file->added, file->deleted);
static long gather_dirstat(struct diff_options *opt, struct dirstat_dir *dir,
		unsigned long changed, const char *base, int baselen)
	const char *line_prefix = "";
	struct strbuf *msg = NULL;

	if (opt->output_prefix) {
		msg = opt->output_prefix(opt, opt->output_prefix_data);
		line_prefix = msg->buf;
	}
			this = gather_dirstat(opt, dir, changed, f->name, newbaselen);
				fprintf(opt->file, "%s%4d.%01d%% %.*s\n", line_prefix,
					percent, permille % 10, baselen, base);
	gather_dirstat(options, &dir, changed, "", 0);
	int conflict_marker_size;
static int is_conflict_marker(const char *line, int marker_size, unsigned long len)
	if (len < marker_size + 1)
	case '=': case '>': case '<': case '|':
	for (cnt = 1; cnt < marker_size; cnt++)
	/* line[1] thru line[marker_size-1] are same as firstchar */
	if (len < marker_size + 1 || !isspace(line[marker_size]))
	int marker_size = data->conflict_marker_size;
	char *line_prefix = "";
	struct strbuf *msgbuf;

	assert(data->o);
	if (data->o->output_prefix) {
		msgbuf = data->o->output_prefix(data->o,
			data->o->output_prefix_data);
		line_prefix = msgbuf->buf;
	}
		if (is_conflict_marker(line + 1, marker_size, len - 1)) {
				"%s%s:%d: leftover conflict marker\n",
				line_prefix, data->filename, data->lineno);
		fprintf(data->o->file, "%s%s:%d: %s.\n",
			line_prefix, data->filename, data->lineno, err);
		emit_line(data->o, set, reset, line, 1);
static void emit_binary_diff_body(FILE *file, mmfile_t *one, mmfile_t *two, char *prefix)
		fprintf(file, "%sdelta %lu\n", prefix, orig_size);
		fprintf(file, "%sliteral %lu\n", prefix, two->size);
		fprintf(file, "%s", prefix);
	fprintf(file, "%s\n", prefix);
static void emit_binary_diff(FILE *file, mmfile_t *one, mmfile_t *two, char *prefix)
	fprintf(file, "%sGIT binary patch\n", prefix);
	emit_binary_diff_body(file, one, two, prefix);
	emit_binary_diff_body(file, two, one, prefix);
	/* Use already-loaded driver */
	if (one->driver)
		return;

	if (S_ISREG(one->mode))

	/* Fallback to default settings */
struct userdiff_driver *get_textconv(struct diff_filespec *one)

	if (!one->driver->textconv)
		return NULL;

	if (one->driver->textconv_want_cache && !one->driver->textconv_cache) {
		struct notes_cache *c = xmalloc(sizeof(*c));
		struct strbuf name = STRBUF_INIT;

		strbuf_addf(&name, "textconv/%s", one->driver->name);
		notes_cache_init(c, name.buf, one->driver->textconv);
		one->driver->textconv_cache = c;
	}

	return one->driver;
			 int must_show_header,
	struct userdiff_driver *textconv_one = NULL;
	struct userdiff_driver *textconv_two = NULL;
	struct strbuf header = STRBUF_INIT;
	struct strbuf *msgbuf;
	char *line_prefix = "";

	if (o->output_prefix) {
		msgbuf = o->output_prefix(o, o->output_prefix_data);
		line_prefix = msgbuf->buf;
	}

	if (DIFF_OPT_TST(o, SUBMODULE_LOG) &&
			(!one->mode || S_ISGITLINK(one->mode)) &&
			(!two->mode || S_ISGITLINK(two->mode))) {
		const char *del = diff_get_color_opt(o, DIFF_FILE_OLD);
		const char *add = diff_get_color_opt(o, DIFF_FILE_NEW);
		show_submodule_summary(o->file, one ? one->path : two->path,
				one->sha1, two->sha1, two->dirty_submodule,
				del, add, reset);
		return;
	}
	strbuf_addf(&header, "%s%sdiff --git %s %s%s\n", line_prefix, set, a_one, b_two, reset);
		strbuf_addf(&header, "%s%snew file mode %06o%s\n", line_prefix, set, two->mode, reset);
		if (xfrm_msg)
			strbuf_addstr(&header, xfrm_msg);
		must_show_header = 1;
		strbuf_addf(&header, "%s%sdeleted file mode %06o%s\n", line_prefix, set, one->mode, reset);
		if (xfrm_msg)
			strbuf_addstr(&header, xfrm_msg);
		must_show_header = 1;
			strbuf_addf(&header, "%s%sold mode %06o%s\n", line_prefix, set, one->mode, reset);
			strbuf_addf(&header, "%s%snew mode %06o%s\n", line_prefix, set, two->mode, reset);
			must_show_header = 1;
		if (xfrm_msg)
			strbuf_addstr(&header, xfrm_msg);

			fprintf(o->file, "%s", header.buf);
			strbuf_reset(&header);
	    ( (!textconv_one && diff_filespec_is_binary(one)) ||
	      (!textconv_two && diff_filespec_is_binary(two)) )) {
		if (fill_mmfile(&mf1, one) < 0 || fill_mmfile(&mf2, two) < 0)
			die("unable to read files to diff");
		    !memcmp(mf1.ptr, mf2.ptr, mf1.size)) {
			if (must_show_header)
				fprintf(o->file, "%s", header.buf);
		}
		fprintf(o->file, "%s", header.buf);
		strbuf_reset(&header);
			emit_binary_diff(o->file, &mf1, &mf2, line_prefix);
			fprintf(o->file, "%sBinary files %s and %s differ\n",
				line_prefix, lbl[0], lbl[1]);
		if (!DIFF_XDL_TST(o, WHITESPACE_FLAGS) || must_show_header) {
			fprintf(o->file, "%s", header.buf);
			strbuf_reset(&header);
		mf1.size = fill_textconv(textconv_one, one, &mf1.ptr);
		mf2.size = fill_textconv(textconv_two, two, &mf2.ptr);

		if (ecbdata.ws_rule & WS_BLANK_AT_EOF)
			check_blank_at_eof(&mf1, &mf2, &ecbdata);
		ecbdata.opt = o;
		ecbdata.header = header.len ? &header : NULL;
		xpp.flags = o->xdl_opts;
		if (o->word_diff) {
			int i;

			ecbdata.diff_words->type = o->word_diff;
			ecbdata.diff_words->opt = o;
			for (i = 0; i < ARRAY_SIZE(diff_words_styles); i++) {
				if (o->word_diff == diff_words_styles[i].type) {
					ecbdata.diff_words->style =
						&diff_words_styles[i];
					break;
				}
			}
			if (DIFF_OPT_TST(o, COLOR_DIFF)) {
				struct diff_words_style *st = ecbdata.diff_words->style;
				st->old.color = diff_get_color_opt(o, DIFF_FILE_OLD);
				st->new.color = diff_get_color_opt(o, DIFF_FILE_NEW);
				st->ctx.color = diff_get_color_opt(o, DIFF_PLAIN);
			}
			      &xpp, &xecfg);
		if (o->word_diff)
	strbuf_release(&header);
		xpp.flags = o->xdl_opts;
			      &xpp, &xecfg);
	data.conflict_marker_size = ll_merge_marker_size(attr_path);
		xpp.flags = 0;
			      &xpp, &xecfg);

		if (data.ws_rule & WS_BLANK_AT_EOF) {
			struct emit_callback ecbdata;
			int blank_at_eof;

			ecbdata.ws_rule = data.ws_rule;
			check_blank_at_eof(&mf1, &mf2, &ecbdata);
			blank_at_eof = ecbdata.blank_at_eof_in_postimage;

			if (blank_at_eof) {
				static char *err;
				if (!err)
					err = whitespace_error_string(WS_BLANK_AT_EOF);
				fprintf(o->file, "%s:%d: %s.\n",
					data.filename, blank_at_eof, err);
				data.status = 1; /* report errors */
			}
	if ((ce->ce_flags & CE_VALID) || ce_skip_worktree(ce))
	char *data = xmalloc(100), *dirty = "";

	/* Are we looking at the work tree? */
	if (s->dirty_submodule)
		dirty = "-dirty";

		       "Subproject commit %s%s\n", sha1_to_hex(s->sha1), dirty);
		if (size_only) {
			if (type < 0)
				die("unable to read %s", sha1_to_hex(s->sha1));
		} else {
			if (!s->data)
				die("unable to read %s", sha1_to_hex(s->sha1));
	retval = run_command_v_opt(spawn_arg, RUN_USING_SHELL);
			  struct diff_filepair *p,
			  int *must_show_header,
			  int use_color)
	const char *set = diff_get_color(use_color, DIFF_METAINFO);
	const char *reset = diff_get_color(use_color, DIFF_RESET);
	struct strbuf *msgbuf;
	char *line_prefix = "";

	*must_show_header = 1;
	if (o->output_prefix) {
		msgbuf = o->output_prefix(o, o->output_prefix_data);
		line_prefix = msgbuf->buf;
	}
		strbuf_addf(msg, "%s%ssimilarity index %d%%",
			    line_prefix, set, similarity_index(p));
		strbuf_addf(msg, "%s\n%s%scopy from ",
			    reset,  line_prefix, set);
		strbuf_addf(msg, "%s\n%s%scopy to ", reset, line_prefix, set);
		strbuf_addf(msg, "%s\n", reset);
		strbuf_addf(msg, "%s%ssimilarity index %d%%",
			    line_prefix, set, similarity_index(p));
		strbuf_addf(msg, "%s\n%s%srename from ",
			    reset, line_prefix, set);
		strbuf_addf(msg, "%s\n%s%srename to ",
			    reset, line_prefix, set);
		strbuf_addf(msg, "%s\n", reset);
			strbuf_addf(msg, "%s%sdissimilarity index %d%%%s\n",
				    line_prefix,
				    set, similarity_index(p), reset);
		*must_show_header = 0;
		strbuf_addf(msg, "%s%sindex %s..", line_prefix, set,
			    find_unique_abbrev(one->sha1, abbrev));
		strbuf_addstr(msg, find_unique_abbrev(two->sha1, abbrev));
		strbuf_addf(msg, "%s\n", reset);
	int must_show_header = 0;
	if (msg) {
		/*
		 * don't use colors when the header is intended for an
		 * external diff driver
		 */
		fill_metainfo(msg, name, other, one, two, o, p,
			      &must_show_header,
			      DIFF_OPT_TST(o, COLOR_DIFF) && !pgm);
		xfrm_msg = msg->len ? msg->buf : NULL;
	}

			     one, two, xfrm_msg, must_show_header,
			     o, complete_rewrite);
	if (*namep && **namep != '/') {
		if (**namep == '/')
			++*namep;
	}
	if (*otherp && **otherp != '/') {
		if (**otherp == '/')
			++*otherp;
	}
	memcpy(options, &default_diff_options, sizeof(*options));
	if (diff_no_prefix) {
		options->a_prefix = options->b_prefix = "";
	} else if (!diff_mnemonic_prefix) {
	/*
	 * Most of the time we can say "there are changes"
	 * only by checking if there are changed paths, but
	 * --ignore-whitespace* options force us to look
	 * inside contents.
	 */

	if (DIFF_XDL_TST(options, IGNORE_WHITESPACE) ||
	    DIFF_XDL_TST(options, IGNORE_WHITESPACE_CHANGE) ||
	    DIFF_XDL_TST(options, IGNORE_WHITESPACE_AT_EOL))
		DIFF_OPT_SET(options, DIFF_FROM_CONTENTS);
	else
		DIFF_OPT_CLR(options, DIFF_FROM_CONTENTS);

	/*
	 * When patches are generated, submodules diffed against the work tree
	 * must be checked for dirtiness too so it can be shown in the output
	 */
	if (options->output_format & DIFF_FORMAT_PATCH)
		DIFF_OPT_SET(options, DIRTY_SUBMODULES);
	if (DIFF_OPT_TST(options, QUICK)) {
static inline int short_opt(char opt, const char **argv,
			    const char **optarg)
{
	const char *arg = argv[0];
	if (arg[0] != '-' || arg[1] != opt)
		return 0;
	if (arg[2] != '\0') {
		*optarg = arg + 2;
		return 1;
	}
	if (!argv[1])
		die("Option '%c' requires a value", opt);
	*optarg = argv[1];
	return 2;
}

int parse_long_opt(const char *opt, const char **argv,
		   const char **optarg)
{
	const char *arg = argv[0];
	if (arg[0] != '-' || arg[1] != '-')
		return 0;
	arg += strlen("--");
	if (prefixcmp(arg, opt))
		return 0;
	arg += strlen(opt);
	if (*arg == '=') { /* sticked form: --option=value */
		*optarg = arg + 1;
		return 1;
	}
	if (*arg != '\0')
		return 0;
	/* separate form: --option value */
	if (!argv[1])
		die("Option '--%s' requires a value", opt);
	*optarg = argv[1];
	return 2;
}

static int stat_opt(struct diff_options *options, const char **av)
{
	const char *arg = av[0];
	char *end;
	int width = options->stat_width;
	int name_width = options->stat_name_width;
	int argcount = 1;

	arg += strlen("--stat");
	end = (char *)arg;

	switch (*arg) {
	case '-':
		if (!prefixcmp(arg, "-width")) {
			arg += strlen("-width");
			if (*arg == '=')
				width = strtoul(arg + 1, &end, 10);
			else if (!*arg && !av[1])
				die("Option '--stat-width' requires a value");
			else if (!*arg) {
				width = strtoul(av[1], &end, 10);
				argcount = 2;
			}
		} else if (!prefixcmp(arg, "-name-width")) {
			arg += strlen("-name-width");
			if (*arg == '=')
				name_width = strtoul(arg + 1, &end, 10);
			else if (!*arg && !av[1])
				die("Option '--stat-name-width' requires a value");
			else if (!*arg) {
				name_width = strtoul(av[1], &end, 10);
				argcount = 2;
			}
		}
		break;
	case '=':
		width = strtoul(arg+1, &end, 10);
		if (*end == ',')
			name_width = strtoul(end+1, &end, 10);
	}

	/* Important! This checks all the error cases! */
	if (*end)
		return 0;
	options->output_format |= DIFF_FORMAT_DIFFSTAT;
	options->stat_name_width = name_width;
	options->stat_width = width;
	return argcount;
}

	const char *optarg;
	int argcount;
	if (!strcmp(arg, "-p") || !strcmp(arg, "-u") || !strcmp(arg, "--patch"))
	else if (!prefixcmp(arg, "--stat"))
		/* --stat, --stat-width, or --stat-name-width */
		return stat_opt(options, av);
	else if (!prefixcmp(arg, "-B") || !prefixcmp(arg, "--break-rewrites=") ||
		 !strcmp(arg, "--break-rewrites")) {
			return error("invalid argument to -B: %s", arg+2);
	else if (!prefixcmp(arg, "-M") || !prefixcmp(arg, "--find-renames=") ||
		 !strcmp(arg, "--find-renames")) {
			return error("invalid argument to -M: %s", arg+2);
	else if (!prefixcmp(arg, "-C") || !prefixcmp(arg, "--find-copies=") ||
		 !strcmp(arg, "--find-copies")) {
			return error("invalid argument to -C: %s", arg+2);
	else if (!prefixcmp(arg, "--color=")) {
		int value = git_config_colorbool(NULL, arg+8, -1);
		if (value == 0)
			DIFF_OPT_CLR(options, COLOR_DIFF);
		else if (value > 0)
			DIFF_OPT_SET(options, COLOR_DIFF);
		else
			return error("option `color' expects \"always\", \"auto\", or \"never\"");
	}
		options->word_diff = DIFF_WORDS_COLOR;
		options->word_diff = DIFF_WORDS_COLOR;
	else if (!strcmp(arg, "--word-diff")) {
		if (options->word_diff == DIFF_WORDS_NONE)
			options->word_diff = DIFF_WORDS_PLAIN;
	}
	else if (!prefixcmp(arg, "--word-diff=")) {
		const char *type = arg + 12;
		if (!strcmp(type, "plain"))
			options->word_diff = DIFF_WORDS_PLAIN;
		else if (!strcmp(type, "color")) {
			DIFF_OPT_SET(options, COLOR_DIFF);
			options->word_diff = DIFF_WORDS_COLOR;
		}
		else if (!strcmp(type, "porcelain"))
			options->word_diff = DIFF_WORDS_PORCELAIN;
		else if (!strcmp(type, "none"))
			options->word_diff = DIFF_WORDS_NONE;
		else
			die("bad --word-diff argument: %s", type);
	}
	else if ((argcount = parse_long_opt("word-diff-regex", av, &optarg))) {
		if (options->word_diff == DIFF_WORDS_NONE)
			options->word_diff = DIFF_WORDS_PLAIN;
		options->word_regex = optarg;
		return argcount;
	}
		DIFF_OPT_SET(options, QUICK);
	else if (!strcmp(arg, "--ignore-submodules")) {
		DIFF_OPT_SET(options, OVERRIDE_SUBMODULE_CONFIG);
		handle_ignore_submodules_arg(options, "all");
	} else if (!prefixcmp(arg, "--ignore-submodules=")) {
		DIFF_OPT_SET(options, OVERRIDE_SUBMODULE_CONFIG);
		handle_ignore_submodules_arg(options, arg + 20);
	} else if (!strcmp(arg, "--submodule"))
		DIFF_OPT_SET(options, SUBMODULE_LOG);
	else if (!prefixcmp(arg, "--submodule=")) {
		if (!strcmp(arg + 12, "log"))
			DIFF_OPT_SET(options, SUBMODULE_LOG);
	}
	else if ((argcount = short_opt('l', av, &optarg))) {
		options->rename_limit = strtoul(optarg, NULL, 10);
		return argcount;
	}
	else if ((argcount = short_opt('S', av, &optarg))) {
		options->pickaxe = optarg;
		options->pickaxe_opts |= DIFF_PICKAXE_KIND_S;
		return argcount;
	} else if ((argcount = short_opt('G', av, &optarg))) {
		options->pickaxe = optarg;
		options->pickaxe_opts |= DIFF_PICKAXE_KIND_G;
		return argcount;
	}
		options->pickaxe_opts |= DIFF_PICKAXE_ALL;
		options->pickaxe_opts |= DIFF_PICKAXE_REGEX;
	else if ((argcount = short_opt('O', av, &optarg))) {
		options->orderfile = optarg;
		return argcount;
	}
	else if ((argcount = parse_long_opt("diff-filter", av, &optarg))) {
		options->filter = optarg;
		return argcount;
	}
	else if ((argcount = parse_long_opt("src-prefix", av, &optarg))) {
		options->a_prefix = optarg;
		return argcount;
	}
	else if ((argcount = parse_long_opt("dst-prefix", av, &optarg))) {
		options->b_prefix = optarg;
		return argcount;
	}
	else if ((argcount = parse_long_opt("output", av, &optarg))) {
		options->file = fopen(optarg, "w");
		if (!options->file)
			die_errno("Could not open '%s'", optarg);
		return argcount;
int parse_rename_score(const char **cp_p)
	for (;;) {
	if (cmd == '-') {
		/* convert the long-form arguments into short-form versions */
		if (!prefixcmp(opt, "break-rewrites")) {
			opt += strlen("break-rewrites");
			if (*opt == 0 || *opt++ == '=')
				cmd = 'B';
		} else if (!prefixcmp(opt, "find-copies")) {
			opt += strlen("find-copies");
			if (*opt == 0 || *opt++ == '=')
				cmd = 'C';
		} else if (!prefixcmp(opt, "find-renames")) {
			opt += strlen("find-renames");
			if (*opt == 0 || *opt++ == '=')
				cmd = 'M';
		}
	}
	opt1 = parse_rename_score(&opt);
			opt2 = parse_rename_score(&opt);
	if (opt->output_prefix) {
		struct strbuf *msg = NULL;
		msg = opt->output_prefix(opt, opt->output_prefix_data);
		fprintf(opt->file, "%s", msg->buf);
	}
	    !hashcmp(one->sha1, two->sha1) &&
	    !one->dirty_submodule && !two->dirty_submodule)
		return; /* no useful stat for tree diffs */
		return; /* nothing to check in tree diffs */
			 p->one->dirty_submodule ||
			 p->two->dirty_submodule ||
static void show_mode_change(FILE *file, struct diff_filepair *p, int show_name,
		const char *line_prefix)
		fprintf(file, "%s mode change %06o => %06o%c", line_prefix, p->one->mode,
			p->two->mode, show_name ? ' ' : '\n');
static void show_rename_copy(FILE *file, const char *renamecopy, struct diff_filepair *p,
			const char *line_prefix)
	show_mode_change(file, p, 0, line_prefix);
static void diff_summary(struct diff_options *opt, struct diff_filepair *p)
	FILE *file = opt->file;
	char *line_prefix = "";

	if (opt->output_prefix) {
		struct strbuf *buf = opt->output_prefix(opt, opt->output_prefix_data);
		line_prefix = buf->buf;
	}

		fputs(line_prefix, file);
		fputs(line_prefix, file);
		fputs(line_prefix, file);
		show_rename_copy(file, "copy", p, line_prefix);
		fputs(line_prefix, file);
		show_rename_copy(file, "rename", p, line_prefix);
			fprintf(file, "%s rewrite ", line_prefix);
		show_mode_change(file, p, !p->score, line_prefix);
		if (diff_filespec_is_binary(p->one) ||
		    diff_filespec_is_binary(p->two)) {
			git_SHA1_Update(&ctx, sha1_to_hex(p->one->sha1), 40);
			git_SHA1_Update(&ctx, sha1_to_hex(p->two->sha1), 40);
			continue;
		}

		xpp.flags = 0;
		xecfg.flags = 0;
			      &xpp, &xecfg);
	DIFF_QUEUE_CLEAR(q);
		for (i = 0; i < q->nr; i++) {
			diff_summary(options, q->queue[i]);
		}
	if (output_format & DIFF_FORMAT_NO_OUTPUT &&
	    DIFF_OPT_TST(options, EXIT_WITH_STATUS) &&
	    DIFF_OPT_TST(options, DIFF_FROM_CONTENTS)) {
		/*
		 * run diff_flush_patch for the exit status. setting
		 * options->file to /dev/null should be safe, becaue we
		 * aren't supposed to produce any output anyway.
		 */
		if (options->close_file)
			fclose(options->file);
		options->file = fopen("/dev/null", "w");
		if (!options->file)
			die_errno("Could not open /dev/null");
		options->close_file = 1;
		for (i = 0; i < q->nr; i++) {
			struct diff_filepair *p = q->queue[i];
			if (check_pair_status(p))
				diff_flush_patch(p, options);
			if (options->found_changes)
				break;
		}
	}

	DIFF_QUEUE_CLEAR(q);

	/*
	 * Report the content-level differences with HAS_CHANGES;
	 * diff_addremove/diff_change does not set the bit when
	 * DIFF_FROM_CONTENTS is in effect (e.g. with -w).
	 */
	if (DIFF_OPT_TST(options, DIFF_FROM_CONTENTS)) {
		if (options->found_changes)
			DIFF_OPT_SET(options, HAS_CHANGES);
		else
			DIFF_OPT_CLR(options, HAS_CHANGES);
	}
	DIFF_QUEUE_CLEAR(&outq);
	DIFF_QUEUE_CLEAR(&outq);
		 * 1. Entries that come from stat info dirtiness
static int diffnamecmp(const void *a_, const void *b_)
{
	const struct diff_filepair *a = *((const struct diff_filepair **)a_);
	const struct diff_filepair *b = *((const struct diff_filepair **)b_);
	const char *name_a, *name_b;

	name_a = a->one ? a->one->path : a->two->path;
	name_b = b->one ? b->one->path : b->two->path;
	return strcmp(name_a, name_b);
}

void diffcore_fix_diff_index(struct diff_options *options)
{
	struct diff_queue_struct *q = &diff_queued_diff;
	qsort(q->queue, q->nr, sizeof(q->queue[0]), diffnamecmp);
}

	if (!options->found_follow) {
		/* See try_to_follow_renames() in tree-diff.c */
		if (options->break_opt != -1)
			diffcore_break(options->break_opt);
		if (options->detect_rename)
			diffcore_rename(options);
		if (options->break_opt != -1)
			diffcore_merge_broken();
	}
		diffcore_pickaxe(options);
	if (!options->found_follow)
		/* See try_to_follow_renames() in tree-diff.c */
		diff_resolve_rename_copy();
	if (diff_queued_diff.nr && !DIFF_OPT_TST(options, DIFF_FROM_CONTENTS))

	options->found_follow = 0;
/*
 * Shall changes to this submodule be ignored?
 *
 * Submodule changes can be configured to be ignored separately for each path,
 * but that configuration can be overridden from the command line.
 */
static int is_submodule_ignored(const char *path, struct diff_options *options)
{
	int ignored = 0;
	unsigned orig_flags = options->flags;
	if (!DIFF_OPT_TST(options, OVERRIDE_SUBMODULE_CONFIG))
		set_diffopt_flags_from_submodule_config(options, path);
	if (DIFF_OPT_TST(options, IGNORE_SUBMODULES))
		ignored = 1;
	options->flags = orig_flags;
	return ignored;
}

		    const char *concatpath, unsigned dirty_submodule)
	if (S_ISGITLINK(mode) && is_submodule_ignored(concatpath, options))
	if (addremove != '-') {
		two->dirty_submodule = dirty_submodule;
	}
	if (!DIFF_OPT_TST(options, DIFF_FROM_CONTENTS))
		DIFF_OPT_SET(options, HAS_CHANGES);
		 const char *concatpath,
		 unsigned old_dirty_submodule, unsigned new_dirty_submodule)
	if (S_ISGITLINK(old_mode) && S_ISGITLINK(new_mode) &&
	    is_submodule_ignored(concatpath, options))
		tmp = old_dirty_submodule; old_dirty_submodule = new_dirty_submodule;
			new_dirty_submodule = tmp;
	one->dirty_submodule = old_dirty_submodule;
	two->dirty_submodule = new_dirty_submodule;
	if (!DIFF_OPT_TST(options, DIFF_FROM_CONTENTS))
		DIFF_OPT_SET(options, HAS_CHANGES);
	int err = 0;
	child.use_shell = 1;
	if (start_command(&child)) {

	if (strbuf_read(&buf, child.out, 0) < 0)
		err = error("error reading from textconv command '%s'", pgm);

	if (finish_command(&child) || err) {
		strbuf_release(&buf);
		remove_tempfile();
		return NULL;
	}

size_t fill_textconv(struct userdiff_driver *driver,
		     struct diff_filespec *df,
		     char **outbuf)
{
	size_t size;

	if (!driver || !driver->textconv) {
		if (!DIFF_FILE_VALID(df)) {
			*outbuf = "";
			return 0;
		}
		if (diff_populate_filespec(df, 0))
			die("unable to read files to diff");
		*outbuf = df->data;
		return df->size;
	}

	if (driver->textconv_cache && df->sha1_valid) {
		*outbuf = notes_cache_get(driver->textconv_cache, df->sha1,
					  &size);
		if (*outbuf)
			return size;
	}

	*outbuf = run_textconv(driver->textconv, df, &size);
	if (!*outbuf)
		die("unable to read files to diff");

	if (driver->textconv_cache && df->sha1_valid) {
		/* ignore errors, as we might be in a readonly repository */
		notes_cache_put(driver->textconv_cache, df->sha1, *outbuf,
				size);
		/*
		 * we could save up changes and flush them all at the end,
		 * but we would need an extra call after all diffing is done.
		 * Since generating a cache entry is the slow path anyway,
		 * this extra overhead probably isn't a big deal.
		 */
		notes_cache_write(driver->textconv_cache);
	}

	return size;
}