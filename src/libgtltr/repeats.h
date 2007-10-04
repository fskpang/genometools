/*
  Copyright (c) 2007 David Ellinghaus <dellinghaus@zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg

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

#ifndef REPEATS_H
#define REPEATS_H

#include "libgtmatch/sarr-def.h"
#include "repeattypes.h"
#include "ltrharvest-opt.h"

void showrepeats (RepeatInfo * repeatinfo,
                  unsigned long seedminlength);

int simpleexactselfmatchoutput(/*@unused@*/ void *info,
    Seqpos len, Seqpos pos1, Seqpos pos2);

int simpleexactselfmatchstore(LTRharvestoptions *lo, Seqpos len,
                              Seqpos pos1, Seqpos pos2);

int subsimpleexactselfmatchstore(SubRepeatInfo *info, unsigned long len,
  Seqpos dbstart, unsigned long querystart);

int subshowrepeats (SubRepeatInfo *info, unsigned long len, Seqpos dbstart,
                    unsigned long querystart);
#endif
