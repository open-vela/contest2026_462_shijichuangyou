/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_controller.h"
#include <string.h>

static uint32_t next_token(uint32_t value)
{
  return value == UINT32_MAX ? 1u : value + 1u;
}

static void navigate(struct ebadge_controller *c, int steps, uint32_t now)
{
  int index = (c->state.character + steps) % EBADGE_CHARACTER_COUNT;
  if (index < 0) index += EBADGE_CHARACTER_COUNT;
  c->state.character = (uint8_t)index;
  c->state.generation = next_token(c->state.generation);
  c->state.animation_token = next_token(c->state.animation_token);
  c->state.character_state = EBADGE_IDLE;
  c->state.last_user_activity_ms = now;
}

void ebadge_controller_init(struct ebadge_controller *c, uint32_t now)
{
  memset(c, 0, sizeof(*c));
  c->state.generation = 1;
  c->state.last_user_activity_ms = now;
}

bool ebadge_controller_post(struct ebadge_controller *c,
                           struct ebadge_event event)
{
  bool navigation = event.kind == EBADGE_NEXT || event.kind == EBADGE_PREV;
  if (c->pending_navigation || c->count == EBADGE_QUEUE_CAPACITY)
    {
      if (c->state.dropped_events != UINT32_MAX) c->state.dropped_events++;
      if (!navigation) return false;
      /* A reliable, bounded navigation mailbox follows the full queue.
       * Modulo preserves the final character, even for long swipe bursts.
       * The flag also invalidates old callbacks when the net offset is zero.
       */
      c->pending_steps = (c->pending_steps +
                         (event.kind == EBADGE_NEXT ? 1 : -1)) %
                        EBADGE_CHARACTER_COUNT;
      c->pending_navigation = true;
      return true;
    }

  c->queue[(c->head + c->count) % EBADGE_QUEUE_CAPACITY] = event;
  c->count++;
  return true;
}

void ebadge_controller_step(struct ebadge_controller *c, uint32_t now)
{
  while (c->count)
    {
      struct ebadge_event event = c->queue[c->head];
      c->head = (c->head + 1) % EBADGE_QUEUE_CAPACITY;
      c->count--;
      switch (event.kind)
        {
          case EBADGE_PREV:
          case EBADGE_NEXT:
            navigate(c, event.kind == EBADGE_NEXT ? 1 : -1, now);
            break;
          case EBADGE_TAP:
            c->state.last_user_activity_ms = now;
            if (c->state.character_state == EBADGE_IDLE)
              {
                c->state.character_state = EBADGE_REACTING;
                c->state.reaction_started_ms = now;
                c->state.animation_token = next_token(c->state.animation_token);
              }
            break;
          case EBADGE_ANIMATION_DONE:
            if (event.generation == c->state.generation &&
                event.animation_token == c->state.animation_token)
              c->state.character_state = EBADGE_IDLE;
            break;
          default:
            break;
        }
    }

  if (c->pending_navigation)
    {
      navigate(c, c->pending_steps, now);
      c->pending_steps = 0;
      c->pending_navigation = false;
    }

  /* Unsigned elapsed arithmetic tolerates the LVGL tick's uint32 wrap.
   * Real suspend/wake clock semantics must be handled by the power adapter.
   */
  if (c->state.character_state == EBADGE_REACTING &&
      (uint32_t)(now - c->state.reaction_started_ms) >= EBADGE_REACTION_MS)
    c->state.character_state = EBADGE_IDLE;
}
