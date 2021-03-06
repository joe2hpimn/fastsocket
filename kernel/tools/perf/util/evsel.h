#ifndef __PERF_EVSEL_H
#define __PERF_EVSEL_H 1

#include <linux/list.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../../include/linux/perf_event.h"
#include "types.h"
#include "xyarray.h"
#include "cgroup.h"
#include "hist.h"
 
struct perf_counts_values {
	union {
		struct {
			u64 val;
			u64 ena;
			u64 run;
		};
		u64 values[3];
	};
};

struct perf_counts {
	s8		   	  scaled;
	struct perf_counts_values aggr;
	struct perf_counts_values cpu[];
};

struct perf_evsel;

/*
 * Per fd, to map back from PERF_SAMPLE_ID to evsel, only used when there are
 * more than one entry in the evlist.
 */
struct perf_sample_id {
	struct hlist_node 	node;
	u64		 	id;
	struct perf_evsel	*evsel;
};

/** struct perf_evsel - event selector
 *
 * @name - Can be set to retain the original event name passed by the user,
 *         so that when showing results in tools such as 'perf stat', we
 *         show the name used, not some alias.
 */
struct perf_evsel {
	struct list_head	node;
	struct perf_event_attr	attr;
	char			*filter;
	struct xyarray		*fd;
	struct xyarray		*sample_id;
	u64			*id;
	struct perf_counts	*counts;
	int			idx;
	u32			ids;
	struct hists		hists;
	char			*name;
	struct event_format	*tp_format;
	union {
		void		*priv;
		off_t		id_offset;
	};
	struct cgroup_sel	*cgrp;
	struct {
		void		*func;
		void		*data;
	} handler;
	struct cpu_map		*cpus;
	unsigned int		sample_size;
	bool 			supported;
	bool 			needs_swap;
	/* parse modifier helper */
	int			exclude_GH;
	struct perf_evsel	*leader;
	char			*group_name;
};

struct cpu_map;
struct thread_map;
struct perf_evlist;
struct perf_record_opts;

struct perf_evsel *perf_evsel__new(struct perf_event_attr *attr, int idx);
struct perf_evsel *perf_evsel__newtp(const char *sys, const char *name, int idx);

struct event_format *event_format__new(const char *sys, const char *name);

void perf_evsel__init(struct perf_evsel *evsel,
		      struct perf_event_attr *attr, int idx);
void perf_evsel__exit(struct perf_evsel *evsel);
void perf_evsel__delete(struct perf_evsel *evsel);

void perf_evsel__config(struct perf_evsel *evsel,
			struct perf_record_opts *opts);

bool perf_evsel__is_cache_op_valid(u8 type, u8 op);

#define PERF_EVSEL__MAX_ALIASES 8

extern const char *perf_evsel__hw_cache[PERF_COUNT_HW_CACHE_MAX]
				       [PERF_EVSEL__MAX_ALIASES];
extern const char *perf_evsel__hw_cache_op[PERF_COUNT_HW_CACHE_OP_MAX]
					  [PERF_EVSEL__MAX_ALIASES];
extern const char *perf_evsel__hw_cache_result[PERF_COUNT_HW_CACHE_RESULT_MAX]
					      [PERF_EVSEL__MAX_ALIASES];
extern const char *perf_evsel__hw_names[PERF_COUNT_HW_MAX];
extern const char *perf_evsel__sw_names[PERF_COUNT_SW_MAX];
int __perf_evsel__hw_cache_type_op_res_name(u8 type, u8 op, u8 result,
					    char *bf, size_t size);
const char *perf_evsel__name(struct perf_evsel *evsel);

int perf_evsel__alloc_fd(struct perf_evsel *evsel, int ncpus, int nthreads);
int perf_evsel__alloc_id(struct perf_evsel *evsel, int ncpus, int nthreads);
int perf_evsel__alloc_counts(struct perf_evsel *evsel, int ncpus);
void perf_evsel__free_fd(struct perf_evsel *evsel);
void perf_evsel__free_id(struct perf_evsel *evsel);
void perf_evsel__close_fd(struct perf_evsel *evsel, int ncpus, int nthreads);

int perf_evsel__set_filter(struct perf_evsel *evsel, int ncpus, int nthreads,
			   const char *filter);

int perf_evsel__open_per_cpu(struct perf_evsel *evsel,
			     struct cpu_map *cpus);
int perf_evsel__open_per_thread(struct perf_evsel *evsel,
				struct thread_map *threads);
int perf_evsel__open(struct perf_evsel *evsel, struct cpu_map *cpus,
		     struct thread_map *threads);
void perf_evsel__close(struct perf_evsel *evsel, int ncpus, int nthreads);

struct perf_sample;

void *perf_evsel__rawptr(struct perf_evsel *evsel, struct perf_sample *sample,
			 const char *name);
u64 perf_evsel__intval(struct perf_evsel *evsel, struct perf_sample *sample,
		       const char *name);

static inline char *perf_evsel__strval(struct perf_evsel *evsel,
				       struct perf_sample *sample,
				       const char *name)
{
	return perf_evsel__rawptr(evsel, sample, name);
}

struct format_field;

struct format_field *perf_evsel__field(struct perf_evsel *evsel, const char *name);

#define perf_evsel__match(evsel, t, c)		\
	(evsel->attr.type == PERF_TYPE_##t &&	\
	 evsel->attr.config == PERF_COUNT_##c)

static inline bool perf_evsel__match2(struct perf_evsel *e1,
				      struct perf_evsel *e2)
{
	return (e1->attr.type == e2->attr.type) &&
	       (e1->attr.config == e2->attr.config);
}

int __perf_evsel__read_on_cpu(struct perf_evsel *evsel,
			      int cpu, int thread, bool scale);

/**
 * perf_evsel__read_on_cpu - Read out the results on a CPU and thread
 *
 * @evsel - event selector to read value
 * @cpu - CPU of interest
 * @thread - thread of interest
 */
static inline int perf_evsel__read_on_cpu(struct perf_evsel *evsel,
					  int cpu, int thread)
{
	return __perf_evsel__read_on_cpu(evsel, cpu, thread, false);
}

/**
 * perf_evsel__read_on_cpu_scaled - Read out the results on a CPU and thread, scaled
 *
 * @evsel - event selector to read value
 * @cpu - CPU of interest
 * @thread - thread of interest
 */
static inline int perf_evsel__read_on_cpu_scaled(struct perf_evsel *evsel,
						 int cpu, int thread)
{
	return __perf_evsel__read_on_cpu(evsel, cpu, thread, true);
}

int __perf_evsel__read(struct perf_evsel *evsel, int ncpus, int nthreads,
		       bool scale);

/**
 * perf_evsel__read - Read the aggregate results on all CPUs
 *
 * @evsel - event selector to read value
 * @ncpus - Number of cpus affected, from zero
 * @nthreads - Number of threads affected, from zero
 */
static inline int perf_evsel__read(struct perf_evsel *evsel,
				    int ncpus, int nthreads)
{
	return __perf_evsel__read(evsel, ncpus, nthreads, false);
}

/**
 * perf_evsel__read_scaled - Read the aggregate results on all CPUs, scaled
 *
 * @evsel - event selector to read value
 * @ncpus - Number of cpus affected, from zero
 * @nthreads - Number of threads affected, from zero
 */
static inline int perf_evsel__read_scaled(struct perf_evsel *evsel,
					  int ncpus, int nthreads)
{
	return __perf_evsel__read(evsel, ncpus, nthreads, true);
}

void hists__init(struct hists *hists);

int perf_evsel__parse_sample(struct perf_evsel *evsel, union perf_event *event,
			     struct perf_sample *sample);

static inline struct perf_evsel *perf_evsel__next(struct perf_evsel *evsel)
{
	return list_entry(evsel->node.next, struct perf_evsel, node);
}

static inline bool perf_evsel__is_group_member(const struct perf_evsel *evsel)
{
	return evsel->leader != NULL;
}
#endif /* __PERF_EVSEL_H */
