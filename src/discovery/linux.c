// vim: set ts=2 sw=2 ai et:
// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Madeleine Choi

#include <assert.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/strlst.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../discovery.h"

#define SERVICES_CAPACITY 8

struct _discovery_state_t
{
  all_services_t *all;
  AvahiSimplePoll *simple_poll;
  AvahiClient *client;
};

typedef struct _discovery_state_t _discovery_state_t;

static const char *
_get_txt(AvahiStringList *txt, const char *key)
{
  AvahiStringList *n = avahi_string_list_find(txt, key);
  if (!n) return NULL;
  const char *s = (const char *) avahi_string_list_get_text(n);
  return strchr(s, '=') + 1;
}

accessory_t *
_accessory_new(AvahiStringList *txt, uint16_t port, const char *address)
{
  const char *id = _get_txt(txt, "id");
  const char *name = _get_txt(txt, "md");
  const char *sf = _get_txt(txt, "sf");

  if (!id || !name || !sf)
    return NULL;

  accessory_t *a = malloc(sizeof(*a));
  if (!a) return NULL;

  a->id = strdup(id);
  a->name = strdup(name);
  a->address = address;
  a->status = (accessory_status_t) (sf[0] - '0');
  a->port = port;

  return a;
}

void
_all_services_push(all_services_t *self, accessory_t *acc)
{
  if (self->count == self->capacity)
  {
    self->capacity *= 2;
    self->items = realloc(self->items, self->capacity * sizeof(accessory_t *));
  }

  self->items[self->count++] = acc;
}

void
client_callback(AvahiClient *c, AvahiClientState state, void *userdata)
{
  assert(c);

  _discovery_state_t *discovery_state = userdata;
  assert(discovery_state);

  if (state == AVAHI_CLIENT_FAILURE)
  {
    fprintf(stderr, "server connection failure: %s\n",
            avahi_strerror(avahi_client_errno(c)));
    avahi_simple_poll_quit(discovery_state->simple_poll);
  }
}

void
resolve_callback(AvahiServiceResolver *r,
                 AVAHI_GCC_UNUSED AvahiIfIndex interface,
                 AVAHI_GCC_UNUSED AvahiProtocol protocol,
                 AvahiResolverEvent event, const char *name, const char *type,
                 const char *domain, AVAHI_GCC_UNUSED const char *host_name,
                 const AvahiAddress *address, uint16_t port,
                 AvahiStringList *txt,
                 AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void *userdata)
{
  assert(r);

  _discovery_state_t *state = userdata;
  assert(state);

  switch (event)
  {
    case AVAHI_RESOLVER_FAILURE:
      fprintf(stderr,
              "(resolver) failed to resolve service %s of type %s in domain "
              "%s: %s\n",
              name, type, domain,
              avahi_strerror(
                  avahi_client_errno(avahi_service_resolver_get_client(r))));
      break;

    case AVAHI_RESOLVER_FOUND:
      char a[AVAHI_ADDRESS_STR_MAX];
      avahi_address_snprint(a, sizeof(a), address);
      accessory_t *acc = _accessory_new(txt, port, strdup(a));
      _all_services_push(state->all, acc);
      break;
  }

  avahi_service_resolver_free(r);
}

void
browse_callback(AvahiServiceBrowser *b, AvahiIfIndex interface,
                AvahiProtocol protocol, AvahiBrowserEvent event,
                const char *name, const char *type, const char *domain,
                AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void *userdata)
{
  _discovery_state_t *state = userdata;
  assert(state);

  switch (event)
  {
    case AVAHI_BROWSER_FAILURE:
      fprintf(stderr, "(browser) %s\n",
              avahi_strerror(
                  avahi_client_errno(avahi_service_browser_get_client(b))));
      avahi_simple_poll_quit(state->simple_poll);
      return;

    case AVAHI_BROWSER_NEW:
      if (!(avahi_service_resolver_new(state->client, interface, protocol, name,
                                       type, domain, AVAHI_PROTO_UNSPEC, 0,
                                       resolve_callback, state)))
        fprintf(stderr, "(browser) failed to resolve service %s: %s\n", name,
                avahi_strerror(avahi_client_errno(state->client)));
      break;

    case AVAHI_BROWSER_ALL_FOR_NOW:
      avahi_simple_poll_quit(state->simple_poll);
      return;

    default:
      break;
  }
}

all_services_t *
all_services_discover()
{
  /* Avahi gives us an event loop to work with, but
   * we only want to discover everything once, so we
   * grow a vector of accessory_t and exit on
   * AVAHI_ALL_FOR_NOW.
   */

  all_services_t *all = calloc(1, sizeof(*all));
  all->capacity = SERVICES_CAPACITY;
  all->items = malloc(all->capacity * sizeof(accessory_t *));

  AvahiServiceBrowser *sb = NULL;
  int error = 0;

  _discovery_state_t *state = malloc(sizeof(*state));
  state->all = all;
  state->client = NULL;
  state->simple_poll = NULL;

  if (!(state->simple_poll = avahi_simple_poll_new()))
  {
    fprintf(stderr, "failed to create poll: %s\n", avahi_strerror(error));
    goto fail;
  }

  state->client = avahi_client_new(avahi_simple_poll_get(state->simple_poll), 0,
                                   client_callback, state, &error);

  if (!state->client)
  {
    fprintf(stderr, "failed to create client: %s\n", avahi_strerror(error));
    goto fail;
  }

  sb = avahi_service_browser_new(state->client, AVAHI_IF_UNSPEC,
                                 AVAHI_PROTO_UNSPEC, "_hap._tcp", NULL, 0,
                                 browse_callback, state);

  if (!sb)
  {
    fprintf(stderr, "failed to create service browser: %s\n",
            avahi_strerror(error));
    goto fail;
  }

  avahi_simple_poll_loop(state->simple_poll);

  goto cleanup;

fail:

  state->all->items = NULL;

cleanup:

  if (sb)
    avahi_service_browser_free(sb);

  if (state->client)
    avahi_client_free(state->client);

  if (state->simple_poll)
    avahi_simple_poll_free(state->simple_poll);

  free(state);
  
  return all;
}
