static void diff_filespec_load_driver(struct diff_filespec *one);
static char *run_textconv(const char *, struct diff_filespec *, size_t *);

	die("bad config variable '%s'", var);
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

static void copy_file_with_prefix(FILE *file,
				  int prefix, const char *data, int size,
				  const char *set, const char *reset)
	int ch, nl_just_seen = 1;
	while (0 < size--) {
		ch = *data++;
		if (nl_just_seen) {
			fputs(set, file);
			putc(prefix, file);
		if (ch == '\n') {
			nl_just_seen = 1;
			fputs(reset, file);
		} else
			nl_just_seen = 0;
		putc(ch, file);
	if (!nl_just_seen)
		fprintf(file, "%s\n\\ No newline at end of file\n", reset);
			      const char *textconv_one,
			      const char *textconv_two,
	const char *old = diff_get_color(color_diff, DIFF_FILE_OLD);
	const char *new = diff_get_color(color_diff, DIFF_FILE_NEW);
	const char *data_one, *data_two;
	diff_populate_filespec(one, 0);
	diff_populate_filespec(two, 0);
	if (textconv_one) {
		data_one = run_textconv(textconv_one, one, &size_one);
		if (!data_one)
			die("unable to read files to diff");
	}
	else {
		data_one = one->data;
		size_one = one->size;
	}
	if (textconv_two) {
		data_two = run_textconv(textconv_two, two, &size_two);
		if (!data_two)
			die("unable to read files to diff");
	}
	else {
		data_two = two->data;
		size_two = two->size;
		"%s--- %s%s%s\n%s+++ %s%s%s\n%s@@ -",
		metainfo, a_name.buf, name_a_tab, reset,
		metainfo, b_name.buf, name_b_tab, reset, fraginfo);
		copy_file_with_prefix(o->file, '-', data_one, size_one, old, reset);
		copy_file_with_prefix(o->file, '+', data_two, size_two, new, reset);
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
	FILE *file;
	if (diff_words->current_plus != plus_begin)
		fwrite(diff_words->current_plus,
				plus_begin - diff_words->current_plus, 1,
				diff_words->file);
	if (minus_begin != minus_end)
		color_fwrite_lines(diff_words->file,
				diff_get_color(1, DIFF_FILE_OLD),
				minus_end - minus_begin, minus_begin);
	if (plus_begin != plus_end)
		color_fwrite_lines(diff_words->file,
				diff_get_color(1, DIFF_FILE_NEW),
				plus_end - plus_begin, plus_begin);
	xdemitcb_t ecb;
		color_fwrite_lines(diff_words->file,
			diff_get_color(1, DIFF_FILE_OLD),
			diff_words->minus.text.size, diff_words->minus.text.ptr);
	xpp.flags = XDF_NEED_MINIMAL;
		      &xpp, &xecfg, &ecb);
			diff_words->plus.text.size)
		fwrite(diff_words->current_plus,
			- diff_words->current_plus, 1,
			diff_words->file);
typedef unsigned long (*sane_truncate_fn)(char *line, unsigned long len);

struct emit_callback {
	int nparents, color_diff;
	unsigned ws_rule;
	sane_truncate_fn truncate;
	const char **label_path;
	struct diff_words_data *diff_words;
	int *found_changesp;
	FILE *file;
};
		/* flush buffers */
		if (ecbdata->diff_words->minus.text.size ||
				ecbdata->diff_words->plus.text.size)
			diff_words_show(ecbdata->diff_words);

		free(ecbdata->diff_words->word_regex);
static void emit_line(FILE *file, const char *set, const char *reset, const char *line, int len)
{
	int has_trailing_newline, has_trailing_carriage_return;

	has_trailing_newline = (len > 0 && line[len-1] == '\n');
	if (has_trailing_newline)
		len--;
	has_trailing_carriage_return = (len > 0 && line[len-1] == '\r');
	if (has_trailing_carriage_return)
		len--;

	fputs(set, file);
	fwrite(line, len, 1, file);
	fputs(reset, file);
	if (has_trailing_carriage_return)
		fputc('\r', file);
	if (has_trailing_newline)
		fputc('\n', file);
}

static void emit_add_line(const char *reset, struct emit_callback *ecbdata, const char *line, int len)
{
	const char *ws = diff_get_color(ecbdata->color_diff, DIFF_WHITESPACE);
	const char *set = diff_get_color(ecbdata->color_diff, DIFF_FILE_NEW);

	if (!*ws)
		emit_line(ecbdata->file, set, reset, line, len);
	else {
		/* Emit just the prefix, then the rest. */
		emit_line(ecbdata->file, set, reset, line, ecbdata->nparents);
		ws_check_emit(line + ecbdata->nparents,
			      len - ecbdata->nparents, ecbdata->ws_rule,
			      ecbdata->file, set, reset, ws);
	}
}

	int i;
	int color;
		fprintf(ecbdata->file, "%s--- %s%s%s\n",
			meta, ecbdata->label_path[0], reset, name_a_tab);
		fprintf(ecbdata->file, "%s+++ %s%s%s\n",
			meta, ecbdata->label_path[1], reset, name_b_tab);
	/* This is not really necessary for now because
	 * this codepath only deals with two-way diffs.
	 */
	for (i = 0; i < len && line[i] == '@'; i++)
		;
	if (2 <= i && i < len && line[i] == ' ') {
		ecbdata->nparents = i - 1;
		emit_line(ecbdata->file,
			  diff_get_color(ecbdata->color_diff, DIFF_FRAGINFO),
			  reset, line, len);
			putc('\n', ecbdata->file);
	if (len < ecbdata->nparents) {
		emit_line(ecbdata->file, reset, reset, line, len);
	color = DIFF_PLAIN;
	if (ecbdata->diff_words && ecbdata->nparents != 1)
		/* fall back to normal diff */
		free_diff_words_data(ecbdata);
		if (ecbdata->diff_words->minus.text.size ||
		    ecbdata->diff_words->plus.text.size)
			diff_words_show(ecbdata->diff_words);
		line++;
		len--;
		emit_line(ecbdata->file, plain, reset, line, len);
	for (i = 0; i < ecbdata->nparents && len; i++) {
		if (line[i] == '-')
			color = DIFF_FILE_OLD;
		else if (line[i] == '+')
			color = DIFF_FILE_NEW;
	}
	if (color != DIFF_FILE_NEW) {
		emit_line(ecbdata->file,
			  diff_get_color(ecbdata->color_diff, color),
			  reset, line, len);
		return;
	emit_add_line(reset, ecbdata, line, len);
		unsigned int added, deleted;
	int max_change = 0, max_len = 0;
		int change = file->added + file->deleted;
		int added = data->files[i]->added;
		int deleted = data->files[i]->deleted;
			fprintf(options->file, "%s%d%s", del_c, deleted, reset);
			fprintf(options->file, "%s%d%s", add_c, added, reset);
		fprintf(options->file, "%5d%s", added + deleted,
static void show_shortstats(struct diffstat_t* data, struct diff_options *options)
				"%d\t%d\t", file->added, file->deleted);
static long gather_dirstat(FILE *file, struct dirstat_dir *dir, unsigned long changed, const char *base, int baselen)
			this = gather_dirstat(file, dir, changed, f->name, newbaselen);
				fprintf(file, "%4d.%01d%% %.*s\n", percent, permille % 10, baselen, base);
	gather_dirstat(options->file, &dir, changed, "", 0);
	int trailing_blanks_start;
static int is_conflict_marker(const char *line, unsigned long len)
	if (len < 8)
	case '=': case '>': case '<':
	for (cnt = 1; cnt < 7; cnt++)
	/* line[0] thru line[6] are same as firstchar */
	if (firstchar == '=') {
		/* divider between ours and theirs? */
		if (len != 8 || line[7] != '\n')
			return 0;
	} else if (len < 8 || !isspace(line[7])) {
		/* not divider before ours nor after theirs */
	}
		if (!ws_blank_line(line + 1, len - 1, data->ws_rule))
			data->trailing_blanks_start = 0;
		else if (!data->trailing_blanks_start)
			data->trailing_blanks_start = data->lineno;
		if (is_conflict_marker(line + 1, len - 1)) {
				"%s:%d: leftover conflict marker\n",
				data->filename, data->lineno);
		fprintf(data->o->file, "%s:%d: %s.\n",
			data->filename, data->lineno, err);
		emit_line(data->o->file, set, reset, line, 1);
		data->trailing_blanks_start = 0;
		data->trailing_blanks_start = 0;
static void emit_binary_diff_body(FILE *file, mmfile_t *one, mmfile_t *two)
		fprintf(file, "delta %lu\n", orig_size);
		fprintf(file, "literal %lu\n", two->size);
	fprintf(file, "\n");
static void emit_binary_diff(FILE *file, mmfile_t *one, mmfile_t *two)
	fprintf(file, "GIT binary patch\n");
	emit_binary_diff_body(file, one, two);
	emit_binary_diff_body(file, two, one);
	if (!one->driver)
static const char *get_textconv(struct diff_filespec *one)
	if (!S_ISREG(one->mode))
	diff_filespec_load_driver(one);
	return one->driver->textconv;
	const char *textconv_one = NULL, *textconv_two = NULL;
	fprintf(o->file, "%sdiff --git %s %s%s\n", set, a_one, b_two, reset);
		fprintf(o->file, "%snew file mode %06o%s\n", set, two->mode, reset);
		if (xfrm_msg && xfrm_msg[0])
			fprintf(o->file, "%s%s%s\n", set, xfrm_msg, reset);
		fprintf(o->file, "%sdeleted file mode %06o%s\n", set, one->mode, reset);
		if (xfrm_msg && xfrm_msg[0])
			fprintf(o->file, "%s%s%s\n", set, xfrm_msg, reset);
			fprintf(o->file, "%sold mode %06o%s\n", set, one->mode, reset);
			fprintf(o->file, "%snew mode %06o%s\n", set, two->mode, reset);
		if (xfrm_msg && xfrm_msg[0])
			fprintf(o->file, "%s%s%s\n", set, xfrm_msg, reset);
	if (fill_mmfile(&mf1, one) < 0 || fill_mmfile(&mf2, two) < 0)
		die("unable to read files to diff");

	    ( (diff_filespec_is_binary(one) && !textconv_one) ||
	      (diff_filespec_is_binary(two) && !textconv_two) )) {
		    !memcmp(mf1.ptr, mf2.ptr, mf1.size))
			emit_binary_diff(o->file, &mf1, &mf2);
			fprintf(o->file, "Binary files %s and %s differ\n",
				lbl[0], lbl[1]);
		xdemitcb_t ecb;
		if (textconv_one) {
			size_t size;
			mf1.ptr = run_textconv(textconv_one, one, &size);
			if (!mf1.ptr)
				die("unable to read files to diff");
			mf1.size = size;
		}
		if (textconv_two) {
			size_t size;
			mf2.ptr = run_textconv(textconv_two, two, &size);
			if (!mf2.ptr)
				die("unable to read files to diff");
			mf2.size = size;
		ecbdata.file = o->file;
		xpp.flags = XDF_NEED_MINIMAL | o->xdl_opts;
		if (DIFF_OPT_TST(o, COLOR_DIFF_WORDS)) {
			ecbdata.diff_words->file = o->file;
			      &xpp, &xecfg, &ecb);
		if (DIFF_OPT_TST(o, COLOR_DIFF_WORDS))
		xdemitcb_t ecb;
		xpp.flags = XDF_NEED_MINIMAL | o->xdl_opts;
			      &xpp, &xecfg, &ecb);
		xdemitcb_t ecb;
		xpp.flags = XDF_NEED_MINIMAL;
			      &xpp, &xecfg, &ecb);

		if ((data.ws_rule & WS_TRAILING_SPACE) &&
		    data.trailing_blanks_start) {
			fprintf(o->file, "%s:%d: ends with blank lines.\n",
				data.filename, data.trailing_blanks_start);
			data.status = 1; /* report errors */
	if (ce->ce_flags & CE_VALID)
	char *data = xmalloc(100);
		"Subproject commit %s\n", sha1_to_hex(s->sha1));
		if (size_only)
		else {
	retval = run_command_v_opt(spawn_arg, 0);
			  struct diff_filepair *p)
		strbuf_addf(msg, "similarity index %d%%", similarity_index(p));
		strbuf_addstr(msg, "\ncopy from ");
		strbuf_addstr(msg, "\ncopy to ");
		strbuf_addch(msg, '\n');
		strbuf_addf(msg, "similarity index %d%%", similarity_index(p));
		strbuf_addstr(msg, "\nrename from ");
		strbuf_addstr(msg, "\nrename to ");
		strbuf_addch(msg, '\n');
			strbuf_addf(msg, "dissimilarity index %d%%\n",
				    similarity_index(p));
		/* nothing */
		;
		strbuf_addf(msg, "index %.*s..%.*s",
			    abbrev, sha1_to_hex(one->sha1),
			    abbrev, sha1_to_hex(two->sha1));
		strbuf_addch(msg, '\n');
	if (msg->len)
		strbuf_setlen(msg, msg->len - 1);

	if (msg) {
		fill_metainfo(msg, name, other, one, two, o, p);
		xfrm_msg = msg->len ? msg->buf : NULL;
	}
			     one, two, xfrm_msg, o, complete_rewrite);
	if (*namep && **namep != '/')
	if (*otherp && **otherp != '/')
	memset(options, 0, sizeof(*options));
	if (!diff_mnemonic_prefix) {
	if (DIFF_OPT_TST(options, QUIET)) {
	if (!strcmp(arg, "-p") || !strcmp(arg, "-u"))
	else if (!prefixcmp(arg, "--stat")) {
		char *end;
		int width = options->stat_width;
		int name_width = options->stat_name_width;
		arg += 6;
		end = (char *)arg;

		switch (*arg) {
		case '-':
			if (!prefixcmp(arg, "-width="))
				width = strtoul(arg + 7, &end, 10);
			else if (!prefixcmp(arg, "-name-width="))
				name_width = strtoul(arg + 12, &end, 10);
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
	}
	else if (!prefixcmp(arg, "-B")) {
			return -1;
	else if (!prefixcmp(arg, "-M")) {
			return -1;
	else if (!prefixcmp(arg, "-C")) {
			return -1;
		DIFF_OPT_SET(options, COLOR_DIFF_WORDS);
		DIFF_OPT_SET(options, COLOR_DIFF_WORDS);
		DIFF_OPT_SET(options, QUIET);
	else if (!strcmp(arg, "--ignore-submodules"))
		DIFF_OPT_SET(options, IGNORE_SUBMODULES);
	else if (!prefixcmp(arg, "-l"))
		options->rename_limit = strtoul(arg+2, NULL, 10);
	else if (!prefixcmp(arg, "-S"))
		options->pickaxe = arg + 2;
		options->pickaxe_opts = DIFF_PICKAXE_ALL;
		options->pickaxe_opts = DIFF_PICKAXE_REGEX;
	else if (!prefixcmp(arg, "-O"))
		options->orderfile = arg + 2;
	else if (!prefixcmp(arg, "--diff-filter="))
		options->filter = arg + 14;
	else if (!prefixcmp(arg, "--src-prefix="))
		options->a_prefix = arg + 13;
	else if (!prefixcmp(arg, "--dst-prefix="))
		options->b_prefix = arg + 13;
	else if (!prefixcmp(arg, "--output=")) {
		options->file = fopen(arg + strlen("--output="), "w");
static int parse_num(const char **cp_p)
	for(;;) {
	opt1 = parse_num(&opt);
			opt2 = parse_num(&opt);
	    !hashcmp(one->sha1, two->sha1))
		return; /* no tree diffs in patch format */
		return; /* no tree diffs in patch format */
static void show_mode_change(FILE *file, struct diff_filepair *p, int show_name)
		fprintf(file, " mode change %06o => %06o%c", p->one->mode, p->two->mode,
			show_name ? ' ' : '\n');
static void show_rename_copy(FILE *file, const char *renamecopy, struct diff_filepair *p)
	show_mode_change(file, p, 0);
static void diff_summary(FILE *file, struct diff_filepair *p)
		show_rename_copy(file, "copy", p);
		show_rename_copy(file, "rename", p);
			fputs(" rewrite ", file);
		show_mode_change(file, p, !p->score);
		xdemitcb_t ecb;
		xpp.flags = XDF_NEED_MINIMAL;
		xecfg.flags = XDL_EMIT_FUNCNAMES;
			      &xpp, &xecfg, &ecb);
	q->queue = NULL;
	q->nr = q->alloc = 0;
		for (i = 0; i < q->nr; i++)
			diff_summary(options->file, q->queue[i]);
	q->queue = NULL;
	q->nr = q->alloc = 0;
	outq.queue = NULL;
	outq.nr = outq.alloc = 0;
	outq.queue = NULL;
	outq.nr = outq.alloc = 0;
		 * 1. Entries that come from stat info dirtyness
	if (options->break_opt != -1)
		diffcore_break(options->break_opt);
	if (options->detect_rename)
		diffcore_rename(options);
	if (options->break_opt != -1)
		diffcore_merge_broken();
		diffcore_pickaxe(options->pickaxe, options->pickaxe_opts);
	diff_resolve_rename_copy();
	if (diff_queued_diff.nr)
		    const char *concatpath)
	if (DIFF_OPT_TST(options, IGNORE_SUBMODULES) && S_ISGITLINK(mode))
	if (addremove != '-')
	DIFF_OPT_SET(options, HAS_CHANGES);
		 const char *concatpath)
	if (DIFF_OPT_TST(options, IGNORE_SUBMODULES) && S_ISGITLINK(old_mode)
			&& S_ISGITLINK(new_mode))
	DIFF_OPT_SET(options, HAS_CHANGES);
	if (start_command(&child) != 0 ||
	    strbuf_read(&buf, child.out, 0) < 0 ||
	    finish_command(&child) != 0) {
		close(child.out);
		error("error running textconv command '%s'", pgm);
	close(child.out);