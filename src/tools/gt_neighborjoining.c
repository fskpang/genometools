/*
  Copyright (c) 2003-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2003-2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#include "gt.h"

static OPrval parse_options(int *parsed_args, int argc, char **argv, Error *err)
{
  OptionParser *op;
  OPrval oprval;
  error_check(err);
  op = option_parser_new("sequence_file|example",
                         "Compute and show Neighbor-Joining tree for the "
                         "sequences in sequence file (using\nthe unit cost "
                         "edit distance as distance function). If 'example' is "
                         "given as\nsequence_file, a builtin example is used.");
  oprval = option_parser_parse_min_max_args(op, parsed_args, argc, argv,
                                            versionfunc, 1, 1, err);
  option_parser_free(op);
  return oprval;
}

static double distfunc(unsigned long i, unsigned long j, void *data)
{
  Bioseq *bioseq= (Bioseq*) data;
  return linearedist(bioseq_get_sequence(bioseq, i),
                     bioseq_get_sequence_length(bioseq, i),
                     bioseq_get_sequence(bioseq, j),
                     bioseq_get_sequence_length(bioseq, j));
}

static double exampledistfunc(unsigned long i, unsigned long j, void *data)
{
  static const double exampledistances[5][5] =
    { {0.0   , 0.1715, 0.2147, 0.3091, 0.2326},
      {0.1715, 0.0   , 0.2991, 0.3399, 0.2058},
      {0.2147, 0.2991, 0.0   , 0.2795, 0.3943},
      {0.3091, 0.3399, 0.2795, 0.0   , 0.4289},
      {0.2326, 0.2058, 0.3943, 0.4289, 0.0   } };
  return exampledistances[i][j];
}

int gt_neighborjoining(int argc, char *argv[], Error *err)
{
  bool use_hard_coded_example = false;
  Bioseq *bioseq = NULL;
  NeighborJoining *nj;
  int parsed_args;
  error_check(err);

  /* option parsing */
  switch (parse_options(&parsed_args, argc, argv, err)) {
    case OPTIONPARSER_OK: break;
    case OPTIONPARSER_ERROR: return -1;
    case OPTIONPARSER_REQUESTS_EXIT: return 0;
  }

  if (!strcmp(argv[1], "example"))
    use_hard_coded_example = true;

  if (use_hard_coded_example)
    nj = neighborjoining_new(5, NULL, exampledistfunc);
  else {
    bioseq = bioseq_new(argv[1]);
    nj = neighborjoining_new(bioseq_number_of_sequences(bioseq), bioseq,
                             distfunc);
  }

  neighborjoining_show_tree(nj, stdout);

  bioseq_free(bioseq);
  neighborjoining_free(nj);

  return 0;
}
