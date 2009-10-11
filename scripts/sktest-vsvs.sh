#!/bin/sh
#
# Copyright (c) 2009 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
# Copyright (c) 2009 Center for Bioinformatics, University of Hamburg
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

set -e -x

cd testsuite

num=2
while test ${num} -lt 10 
do
  ../scripts/iterrunmerge.sh ${num}
  num=`expr ${num} + 1`
done

if test ! "X${GTTESTDATA}" = "X"
then
  AT=../testdata/at1MB
  U8=../testdata/U89959_genomic.fas
  ../scripts/rununique.sh ../bin/gt 10 20 ${U8} ${AT}
fi

cd ..
