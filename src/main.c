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

  #define MAC_W 20
  #define NAME_W 20
  #define ADDR_W 30
  #define PORT_W 6
  #define PAIR_W 5

  printf("%-*s %-*s %-*s %-*s %-*s\n\n",
         MAC_W, "MAC",
         NAME_W, "NAME",
         ADDR_W, "ADDR",
         PORT_W, "PORT",
         PAIR_W, "PAIR?");

  for (size_t i = 0; i < all->count; i++)
  {
    accessory_t *acc = all->items[i];

    printf("%-*s %-*s %-*s %-*hu %-*s\n",
           MAC_W, acc->id,
           NAME_W, acc->name,
           ADDR_W, acc->address,
           PORT_W, acc->port,
           PAIR_W, (acc->status - 1) ? "YES" : "NO");
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
