/*
  Copyright (c) 2010-2011 Sascha Steinbiss <steinbiss@zbh.uni-hamburg.de>
  Copyright (c) 2010-2011 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2006-2008 Center for Bioinformatics, University of Hamburg

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include "core/assert_api.h"
#include "core/qsort_r.h"
#include "core/ma.h"
#include "core/str.h"
#include "core/timer.h"
#include "core/mathsupport.h"
#include "core/unused_api.h"
#include "core/radix-intsort.h"
#include "tools/gt_qsortbench.h"

typedef struct {
  GtStr *impl;
  unsigned long num_values,
                maxvalue;
  bool use_aqsort,
       use_permute,
       verbose;
} QSortBenchArguments;

static void* gt_qsortbench_arguments_new(void)
{
  QSortBenchArguments *arguments = gt_calloc((size_t) 1, sizeof *arguments);
  arguments->impl = gt_str_new();
  return arguments;
}

static void gt_qsortbench_arguments_delete(void *tool_arguments)
{
  QSortBenchArguments *arguments = tool_arguments;
  if (!arguments) return;
  gt_str_delete(arguments->impl);
  gt_free(arguments);
}

static const char *gt_qsort_implementation_names[]
    = {"thomas","system","inlinedptr","inlinedarr","direct",
       "radixlin","radixlin2","radixrec","radixdiv",NULL};

static GtOptionParser* gt_qsortbench_option_parser_new(void *tool_arguments)
{
  QSortBenchArguments *arguments = tool_arguments;
  GtOptionParser *op;
  GtOption *option;

  gt_assert(arguments);

  /* init */
  op = gt_option_parser_new("[option ...]", /* XXX */
                            "Benchmarks quicksort implementations."); /* XXX */

  option = gt_option_new_choice("impl", "implementation\nchoose from "
                                "thomas|system|inlinedptr|inlinedarr|"
                                "direct|radixlin|radixlin2|radixrec|radixdiv",
                                arguments->impl,
                                gt_qsort_implementation_names[0],
                                gt_qsort_implementation_names);
  gt_option_parser_add_option(op, option);

  option = gt_option_new_ulong("size", "number of integers to sort",
                               &arguments->num_values, 1000000UL);
  gt_option_parser_add_option(op, option);

  option = gt_option_new_ulong("maxval", "maximal integer to sort",
                               &arguments->maxvalue, 10000UL);
  gt_option_parser_add_option(op, option);

  option = gt_option_new_bool("aqsort", "prepare bad input array using the "
                                        "'aqsort' anti-quicksort algorithm",
                               &arguments->use_aqsort, false);
  gt_option_parser_add_option(op, option);

  option = gt_option_new_bool("permute", "prepare bad input array by "
                                         "permutation of unique items",
                               &arguments->use_permute, false);
  gt_option_parser_add_option(op, option);

  option = gt_option_new_verbose(&arguments->verbose);
  gt_option_parser_add_option(op, option);
  return op;
}

static int gt_qsortbench_arguments_check(GT_UNUSED int rest_argc,
                                       GT_UNUSED void *tool_arguments,
                                       GT_UNUSED GtError *err)
{
  GT_UNUSED QSortBenchArguments *arguments = tool_arguments;
  int had_err = 0;
  gt_error_check(err);
  gt_assert(arguments);

  return had_err;
}

static void gt_qbenchsort_permute_ulong_array (unsigned long *arr,
                                               unsigned long size)
{
  unsigned long i, j, c;

  for (i = 0; i < size; ++i)
  {
    j = gt_rand_max(size-1);
    c = arr[i];
    arr[i] = arr[j];
    arr[j] = c;
  }
}

unsigned long *val;         /* array, solidified on the fly */
unsigned long ncmp;         /* number of comparisons */
unsigned long nsolid;       /* number of solid items */
unsigned long candidate;    /* pivot candidate */
unsigned long gas;          /* gas value = highest sorted value */
#define freeze(x) val[x] = nsolid++

static int gt_qsortbench_cmp(const void *px, const void *py)
{
  const unsigned long x = *(const unsigned long*)px;
  const unsigned long y = *(const unsigned long*)py;
  ncmp++;
  if (val[x]==gas && val[y]==gas) {
    if (x == candidate) {
      freeze(x);
    } else {
      freeze(y);
    }
  }
  if (val[x] == gas)
    candidate = x;
  else if (val[y] == gas)
    candidate = y;
  if (val[x] < val[y])
  {
    return -1;
  }
  if (val[x] > val[y])
  {
    return 1;
  }
  return 0;
}

static void gt_qsortbench_aqsort(unsigned long n, unsigned long *a)
{
  unsigned long i;
  unsigned long *ptr = gt_malloc(sizeof (*ptr) * n);

  val = a;
  gas = n-1;
  nsolid = ncmp = candidate = 0;
  for (i=0; i<n; i++) {
    ptr[i] = i;
    val[i] = gas;
  }
  qsort(ptr, (size_t) n, sizeof (*ptr), gt_qsortbench_cmp);
  for (i=1UL;i<n;i++)
    gt_assert(val[ptr[i]]==val[ptr[i-1]]+1);
  gt_free(ptr);
}

typedef unsigned long Sorttype;

static unsigned long cmpcount = 0;

static int qsortcmp(const Sorttype *a,const Sorttype *b,
                    const GT_UNUSED void *data)
{
  cmpcount++;
  if ((*a) < (*b))
  {
    return -1;
  }
  if ((*a) > (*b))
  {
    return 1;
  }
  return 0;
}

#ifdef  QSORTNAME
#undef  QSORTNAME
#endif

#define QSORTNAME(NAME) qsortbench_##NAME

#define qsortbench_ARRAY_GET(ARR,RELIDX) ARR[RELIDX]

#define qsortbench_ARRAY_SET(ARR,RELIDX,VALUE) ARR[RELIDX] = VALUE

typedef void * QSORTNAME(Datatype);

typedef unsigned long QSORTNAME(Sorttype);

static int QSORTNAME(qsortcmparr)(const QSORTNAME(Sorttype) *arr,
                                  unsigned long a,
                                  unsigned long b,
                                  const GT_UNUSED void *data)
{
  cmpcount++;
  if (arr[a] < arr[b])
  {
    return -1;
  }
  if (arr[a] > arr[b])
  {
    return 1;
  }
  return 0;
}

#include "match/qsort-array.gen"

#include "match/qsort-direct.gen"

static void check_inlinedarr_qsort(unsigned long *a, unsigned long n)
{
  QSORTNAME(gt_inlinedarr_qsort_r) (6UL, false, a, n, NULL);
}

static void check_direct_qsort(unsigned long *a, unsigned long n)
{
  QSORTNAME(gt_direct_qsort) (6UL, false, a, n);
}

static int qsortcmpwithdata(const void *a,const void *b, GT_UNUSED void *data)
{
  cmpcount++;
  if (*(Sorttype*)a < *((Sorttype*)b))
  {
    return -1;
  }
  if (*(Sorttype*)a > *(Sorttype*)b)
  {
    return 1;
  }
  return 0;
}

#include "match/qsort-inplace.gen"

static void check_thomas_qsort(unsigned long *a, unsigned long n)
{
  gt_qsort_r(a,(size_t) n,sizeof (Sorttype),NULL, qsortcmpwithdata);
}

static int qsortcmpnodata(const void *a,const void *b)
{
  cmpcount++;
  if (*(Sorttype*)a < *((Sorttype*)b))
  {
    return -1;
  }
  if (*(Sorttype*)a > *(Sorttype*)b)
  {
    return 1;
  }
  return 0;
}

static void check_gnu_qsort(unsigned long *a, unsigned long n)
{
  qsort(a,(size_t) n, sizeof (Sorttype), qsortcmpnodata);
}

static void check_inlinedptr_qsort(unsigned long *a, unsigned long n)
{
  gt_inlined_qsort_r(a, n, NULL);
}

static void check_radixsort_GtUlong_linear(unsigned long *source,
                                           unsigned long len)
{
  unsigned long *temp = gt_malloc((size_t) len * sizeof (*temp));

  gt_radixsort_GtUlong_linear(source, temp, len);
  gt_free(temp);
}

static void check_radixsort_GtUlong_linear2(unsigned long *source,
                                            unsigned long len)
{
  unsigned long *temp = gt_malloc((size_t) len * sizeof (*temp));

  gt_radixsort_GtUlong_linear2(source, temp, len);
  gt_free(temp);
}

static void check_radixsort_GtUlong_recursive(unsigned long *source,
                                              unsigned long len)
{
  unsigned long *temp = gt_malloc((size_t) len * sizeof (*temp));

  gt_radixsort_GtUlong_recursive(source, temp, len);
  gt_free(temp);
}

static void check_radixsort_GtUlong_divide(unsigned long *source,
                                           unsigned long len)
{
  unsigned long *temp = gt_malloc((size_t) len * sizeof (*temp));

  gt_radixsort_GtUlong_divide(source, temp, len);
  gt_free(temp);
}

typedef void (*GtQsortimplementationfunc)(unsigned long *,unsigned long);

static GtQsortimplementationfunc gt_qsort_implementation_funcs[] =
{
  check_thomas_qsort,
  check_gnu_qsort,
  check_inlinedptr_qsort,
  check_inlinedarr_qsort,
  check_direct_qsort,
  check_radixsort_GtUlong_linear,
  check_radixsort_GtUlong_linear2,
  check_radixsort_GtUlong_recursive,
  check_radixsort_GtUlong_divide
};

#define GT_NUM_OF_QSORT_IMPLEMENTATIONS\
        (sizeof (gt_qsort_implementation_funcs)/\
         sizeof (gt_qsort_implementation_funcs[0]))

static int gt_qsortbench_runner(GT_UNUSED int argc, GT_UNUSED const char **argv,
                                GT_UNUSED int parsed_args,
                                void *tool_arguments, GT_UNUSED GtError *err)
{
  QSortBenchArguments *arguments = tool_arguments;
  int had_err = 0;
  size_t method;
  GtTimer *timer;
  unsigned long *array, idx;

  gt_error_check(err);
  gt_assert(arguments);
  if (arguments->verbose) {
    printf("# number of items = %lu\n", arguments->num_values);
    printf("# max value items = %lu\n", arguments->maxvalue);
    printf("# implementation = %s\n", gt_str_get(arguments->impl));
  }

  /* prepare benchmark input data */
  array = gt_malloc(sizeof (*array) * arguments->num_values );
  if (arguments->use_aqsort) {
    if (arguments->verbose)
      printf("# using aqsort\n");
    gt_qsortbench_aqsort(arguments->num_values, array);
  } else if (arguments->use_permute) {
    if (arguments->verbose)
      printf("# using permuted array\n");
    for (idx = 0; idx < arguments->num_values; idx++) {
      array[idx] = idx;
    }
    gt_qbenchsort_permute_ulong_array(array, arguments->num_values);
  } else {
    if (arguments->verbose)
      printf("# using simple array of random numbers\n");
    /* use seed initialization to make array deterministic */
    srand48(366292341);
    for (idx = 0; idx < arguments->num_values; idx++) {
      array[idx] = drand48() * arguments->maxvalue;
    }
  }

  timer = gt_timer_new();
  gt_timer_start(timer);
  /* run implementation */
  for (method = 0; method < GT_NUM_OF_QSORT_IMPLEMENTATIONS; method++)
  {
    if (strcmp(gt_str_get(arguments->impl),
               gt_qsort_implementation_names[method]) == 0)
    {
      gt_qsort_implementation_funcs[method](array, arguments->num_values);
      break;
    }
  }
  gt_timer_show(timer, stdout);
  gt_timer_delete(timer);
  for (idx = 1UL; idx < arguments->num_values; idx++) {
    gt_assert(array[idx-1] <= array[idx]);
  }
  gt_free(array);
  if (cmpcount > 0)
  {
    printf("cmpcount = %lu\n",cmpcount);
  }
  return had_err;
}

GtTool* gt_qsortbench(void)
{
  return gt_tool_new(gt_qsortbench_arguments_new,
                  gt_qsortbench_arguments_delete,
                  gt_qsortbench_option_parser_new,
                  gt_qsortbench_arguments_check,
                  gt_qsortbench_runner);
}
