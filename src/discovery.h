// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Madeleine Choi

#pragma once

#include <stdint.h>

enum accessory_status_t
{
  ACCESSORY_STATUS_PAIRED,
  ACCESSORY_STATUS_UNPAIRED,
};

typedef enum accessory_status_t accessory_status_t;

struct accessory_t
{
  const char *id;
  const char *name;
  const char *address;
  accessory_status_t status;
  uint16_t port;
};

typedef struct accessory_t accessory_t;

struct all_services_t
{
  accessory_t **items;
  size_t count;
  size_t capacity;
};

typedef struct all_services_t all_services_t;

all_services_t *
all_services_discover();
