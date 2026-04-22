// vim: set ts=2 sw=2 ai et:
// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Madeleine Choi

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "discovery.h"

[[noreturn]] void
usage()
{
  fprintf(stderr, "Copyright (C) 2026 Madeleine Choi\n"
                  "\n"
                  "usage: openhome list\n");
  exit(EXIT_FAILURE);
}

int
cmd_list(void)
{
  all_services_t *all = all_services_discover();

  if (!all)
    return EXIT_FAILURE;

  for (size_t i = 0; i < all->count; i++)
  {
    accessory_t *accessory = all->items[i];
    printf("%s\n", accessory->name);
  }

  free(all);

  return EXIT_SUCCESS;
}

int
main(int argc, const char **argv)
{
  if (argc < 2)
    usage();

  int ret = EXIT_SUCCESS;

  if (strcmp(argv[1], "list") == 0)
    cmd_list();
  else
    usage();

  return ret;
}
