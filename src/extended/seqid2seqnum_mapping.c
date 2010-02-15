/*
  Copyright (c) 2010 Gordon Gremme <gremme@zbh.uni-hamburg.de>

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

#include <string.h>
#include "core/cstr_api.h"
#include "core/hashmap.h"
#include "core/ma_api.h"
#include "core/parseutils.h"
#include "core/undef.h"
#include "extended/seqid2seqnum_mapping.h"

struct GtSeqid2SeqnumMapping {
  GtHashmap *map;
  unsigned long *seqnums,
                *offsets,
                cached_seqnum,
                cached_offset;
  const char *cached_seqid;
};

static int fill_mapping(GtSeqid2SeqnumMapping *mapping, GtBioseq *bioseq,
                        GtError *err)
{
  unsigned long i, j, offset;
  gt_error_check(err);
  gt_assert(mapping && bioseq);
  for (i = 0; i < gt_bioseq_number_of_sequences(bioseq); i++) {
    const char *desc = gt_bioseq_get_description(bioseq, i);
    mapping->seqnums[i] = i;
    if ((offset = gt_parse_description_range(desc)) == GT_UNDEF_ULONG) {
      /* no offset could be parsed -> store description as sequence id */
      gt_hashmap_add(mapping->map, gt_cstr_dup(desc), mapping->seqnums + i);
      mapping->offsets[i] = 1;
    }
    else {
      char *dup;
      /* offset could be parsed -> store description up to ':' as sequence id */
      j = 0;
      while (desc[j] != ':')
        j++;
      dup = gt_malloc((j + 1) * sizeof *dup);
      strncpy(dup, desc, j);
      dup[j] = '\0';
      gt_hashmap_add(mapping->map, dup, mapping->seqnums + i);
      mapping->offsets[i] = offset;
    }
  }
  return 0;
}

GtSeqid2SeqnumMapping* gt_seqid2seqnum_mapping_new(GtBioseq *bioseq,
                                                   GtError *err)
{
  unsigned long num_of_seqs;
  GtSeqid2SeqnumMapping *mapping;
  gt_error_check(err);
  gt_assert(bioseq);
  mapping = gt_malloc(sizeof *mapping);
  mapping->map = gt_hashmap_new(GT_HASH_STRING, gt_free_func, NULL);
  num_of_seqs = gt_bioseq_number_of_sequences(bioseq);
  mapping->seqnums = gt_malloc(num_of_seqs * sizeof *mapping->seqnums);
  mapping->offsets = gt_malloc(num_of_seqs * sizeof *mapping->offsets);
  if (fill_mapping(mapping, bioseq, err)) {
    gt_seqid2seqnum_mapping_delete(mapping);
    return NULL;
  }
  mapping->cached_seqid = NULL;
  return mapping;
}

void gt_seqid2seqnum_mapping_delete(GtSeqid2SeqnumMapping *mapping)
{
  if (!mapping) return;
  gt_free(mapping->offsets);
  gt_free(mapping->seqnums);
  gt_hashmap_delete(mapping->map);
  gt_free(mapping);
}

#include "core/unused_api.h"

int gt_seqid2seqnum_mapping_map(GtSeqid2SeqnumMapping *mapping,
                                const char *seqid,
                                GT_UNUSED const GtRange *range,
                                unsigned long *seqnum, unsigned long *offset,
                                GtError *err)
{
  unsigned long *seqval;
  gt_error_check(err);
  gt_assert(mapping && seqid && seqnum && offset);
  /* try to answer request from cache */
  if (mapping->cached_seqid && !strcmp(seqid, mapping->cached_seqid)) {
    *seqnum = mapping->cached_seqnum;
    *offset = mapping->cached_offset;
    return 0;
  }
  /* cache miss -> regular mapping */
  if (!(seqval = gt_hashmap_get(mapping->map, seqid))) {
    gt_error_set(err, "sequence ID to sequence number mapping does not contain "
                      "a sequence with ID \"%s\"", seqid);
    return -1;
  }
  *seqnum = *seqval;
  *offset = mapping->offsets[*seqval];
  /* store result in cache */
  mapping->cached_seqid = gt_hashmap_get_key(mapping->map, seqid);
  gt_assert(mapping->cached_seqid);
  mapping->cached_seqnum = *seqval;
  mapping->cached_offset = mapping->offsets[*seqval];
  return 0;
}
