// vim: set ts=2 sw=2 ai et:
// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Madeleine Choi

#include <stdio.h>
#include <stdlib.h>

#include "discovery.h"

int
main(void)
{
  all_services_t *all = all_services_discover();

  for (size_t i = 0; i < all->count; i++)
  {
    accessory_t *accessory = all->items[i];
    printf("%s\n", accessory->name);
  }

  free(all);

  return EXIT_SUCCESS;
}
