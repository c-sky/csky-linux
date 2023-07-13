/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PERF_PARSE_EVENTS_H
#define __PERF_PARSE_EVENTS_H
/*
 * Parse symbolic events/counts passed in as options:
 */

#include <linux/list.h>
#include <stdbool.h>
#include <linux/types.h>
#include <linux/perf_event.h>
#include <string.h>

struct evsel;
struct evlist;
struct parse_events_error;

struct option;
struct perf_pmu;

const char *event_type(int type);

/* Arguments encoded in opt->value. */
struct parse_events_option_args {
	struct evlist **evlistp;
	const char *pmu_filter;
};
int parse_events_option(const struct option *opt, const char *str, int unset);
int parse_events_option_new_evlist(const struct option *opt, const char *str, int unset);
__attribute__((nonnull(1, 2, 4)))
int __parse_events(struct evlist *evlist, const char *str, const char *pmu_filter,
		   struct parse_events_error *error, struct perf_pmu *fake_pmu,
		   bool warn_if_reordered);

__attribute__((nonnull(1, 2, 3)))
static inline int parse_events(struct evlist *evlist, const char *str,
			       struct parse_events_error *err)
{
	return __parse_events(evlist, str, /*pmu_filter=*/NULL, err, /*fake_pmu=*/NULL,
			      /*warn_if_reordered=*/true);
}

int parse_event(struct evlist *evlist, const char *str);

int parse_events_terms(struct list_head *terms, const char *str);
int parse_filter(const struct option *opt, const char *str, int unset);
int exclude_perf(const struct option *opt, const char *arg, int unset);

enum {
	PARSE_EVENTS__TERM_TYPE_NUM,
	PARSE_EVENTS__TERM_TYPE_STR,
};

enum {
	PARSE_EVENTS__TERM_TYPE_USER,
	PARSE_EVENTS__TERM_TYPE_CONFIG,
	PARSE_EVENTS__TERM_TYPE_CONFIG1,
	PARSE_EVENTS__TERM_TYPE_CONFIG2,
	PARSE_EVENTS__TERM_TYPE_CONFIG3,
	PARSE_EVENTS__TERM_TYPE_NAME,
	PARSE_EVENTS__TERM_TYPE_SAMPLE_PERIOD,
	PARSE_EVENTS__TERM_TYPE_SAMPLE_FREQ,
	PARSE_EVENTS__TERM_TYPE_BRANCH_SAMPLE_TYPE,
	PARSE_EVENTS__TERM_TYPE_TIME,
	PARSE_EVENTS__TERM_TYPE_CALLGRAPH,
	PARSE_EVENTS__TERM_TYPE_STACKSIZE,
	PARSE_EVENTS__TERM_TYPE_NOINHERIT,
	PARSE_EVENTS__TERM_TYPE_INHERIT,
	PARSE_EVENTS__TERM_TYPE_MAX_STACK,
	PARSE_EVENTS__TERM_TYPE_MAX_EVENTS,
	PARSE_EVENTS__TERM_TYPE_NOOVERWRITE,
	PARSE_EVENTS__TERM_TYPE_OVERWRITE,
	PARSE_EVENTS__TERM_TYPE_DRV_CFG,
	PARSE_EVENTS__TERM_TYPE_PERCORE,
	PARSE_EVENTS__TERM_TYPE_AUX_OUTPUT,
	PARSE_EVENTS__TERM_TYPE_AUX_SAMPLE_SIZE,
	PARSE_EVENTS__TERM_TYPE_METRIC_ID,
	PARSE_EVENTS__TERM_TYPE_RAW,
	PARSE_EVENTS__TERM_TYPE_LEGACY_CACHE,
	PARSE_EVENTS__TERM_TYPE_HARDWARE,
	__PARSE_EVENTS__TERM_TYPE_NR,
};

struct parse_events_array {
	size_t nr_ranges;
	struct {
		unsigned int start;
		size_t length;
	} *ranges;
};

struct parse_events_term {
	char *config;
	struct parse_events_array array;
	union {
		char *str;
		u64  num;
	} val;
	int type_val;
	int type_term;
	struct list_head list;
	bool used;
	bool no_value;

	/* error string indexes for within parsed string */
	int err_term;
	int err_val;

	/* Coming from implicit alias */
	bool weak;
};

struct parse_events_error {
	int   num_errors;       /* number of errors encountered */
	int   idx;	/* index in the parsed string */
	char *str;      /* string to display at the index */
	char *help;	/* optional help string */
	int   first_idx;/* as above, but for the first encountered error */
	char *first_str;
	char *first_help;
};

struct parse_events_state {
	struct list_head	   list;
	int			   idx;
	struct parse_events_error *error;
	struct evlist		  *evlist;
	struct list_head	  *terms;
	int			   stoken;
	struct perf_pmu		  *fake_pmu;
	/* If non-null, when wildcard matching only match the given PMU. */
	const char		  *pmu_filter;
	/* Should PE_LEGACY_NAME tokens be generated for config terms? */
	bool			   match_legacy_cache_terms;
	bool			   wild_card_pmus;
};

bool parse_events__filter_pmu(const struct parse_events_state *parse_state,
			      const struct perf_pmu *pmu);
void parse_events__shrink_config_terms(void);
int parse_events__is_hardcoded_term(struct parse_events_term *term);
int parse_events_term__num(struct parse_events_term **term,
			   int type_term, char *config, u64 num,
			   bool novalue,
			   void *loc_term, void *loc_val);
int parse_events_term__str(struct parse_events_term **term,
			   int type_term, char *config, char *str,
			   void *loc_term, void *loc_val);
int parse_events_term__term(struct parse_events_term **term,
			    int term_lhs, int term_rhs,
			    void *loc_term, void *loc_val);
int parse_events_term__clone(struct parse_events_term **new,
			     struct parse_events_term *term);
void parse_events_term__delete(struct parse_events_term *term);
void parse_events_terms__delete(struct list_head *terms);
void parse_events_terms__purge(struct list_head *terms);
void parse_events__clear_array(struct parse_events_array *a);
int parse_events__modifier_event(struct list_head *list, char *str, bool add);
int parse_events__modifier_group(struct list_head *list, char *event_mod);
int parse_events_name(struct list_head *list, const char *name);
int parse_events_add_tracepoint(struct list_head *list, int *idx,
				const char *sys, const char *event,
				struct parse_events_error *error,
				struct list_head *head_config);
int parse_events_load_bpf(struct parse_events_state *parse_state,
			  struct list_head *list,
			  char *bpf_file_name,
			  bool source,
			  struct list_head *head_config);
/* Provide this function for perf test */
struct bpf_object;
int parse_events_load_bpf_obj(struct parse_events_state *parse_state,
			      struct list_head *list,
			      struct bpf_object *obj,
			      struct list_head *head_config);
int parse_events_add_numeric(struct parse_events_state *parse_state,
			     struct list_head *list,
			     u32 type, u64 config,
			     struct list_head *head_config,
			     bool wildcard);
int parse_events_add_tool(struct parse_events_state *parse_state,
			  struct list_head *list,
			  int tool_event);
int parse_events_add_cache(struct list_head *list, int *idx, const char *name,
			   struct parse_events_state *parse_state,
			   struct list_head *head_config);
int parse_events__decode_legacy_cache(const char *name, int pmu_type, __u64 *config);
int parse_events_add_breakpoint(struct parse_events_state *parse_state,
				struct list_head *list,
				u64 addr, char *type, u64 len,
				struct list_head *head_config);
int parse_events_add_pmu(struct parse_events_state *parse_state,
			 struct list_head *list, char *name,
			 struct list_head *head_config,
			 bool auto_merge_stats);

struct evsel *parse_events__add_event(int idx, struct perf_event_attr *attr,
				      const char *name, const char *metric_id,
				      struct perf_pmu *pmu);

int parse_events_multi_pmu_add(struct parse_events_state *parse_state,
			       char *str,
			       struct list_head *head_config,
			       struct list_head **listp);

int parse_events_copy_term_list(struct list_head *old,
				 struct list_head **new);

void parse_events__set_leader(char *name, struct list_head *list);
void parse_events_update_lists(struct list_head *list_event,
			       struct list_head *list_all);
void parse_events_evlist_error(struct parse_events_state *parse_state,
			       int idx, const char *str);

struct event_symbol {
	const char	*symbol;
	const char	*alias;
};
extern struct event_symbol event_symbols_hw[];
extern struct event_symbol event_symbols_sw[];

char *parse_events_formats_error_string(char *additional_terms);

void parse_events_error__init(struct parse_events_error *err);
void parse_events_error__exit(struct parse_events_error *err);
void parse_events_error__handle(struct parse_events_error *err, int idx,
				char *str, char *help);
void parse_events_error__print(struct parse_events_error *err,
			       const char *event);

#ifdef HAVE_LIBELF_SUPPORT
/*
 * If the probe point starts with '%',
 * or starts with "sdt_" and has a ':' but no '=',
 * then it should be a SDT/cached probe point.
 */
static inline bool is_sdt_event(char *str)
{
	return (str[0] == '%' ||
		(!strncmp(str, "sdt_", 4) &&
		 !!strchr(str, ':') && !strchr(str, '=')));
}
#else
static inline bool is_sdt_event(char *str __maybe_unused)
{
	return false;
}
#endif /* HAVE_LIBELF_SUPPORT */

#endif /* __PERF_PARSE_EVENTS_H */
