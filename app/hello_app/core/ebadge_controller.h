/* SPDX-License-Identifier: Apache-2.0 */
#ifndef EBADGE_CONTROLLER_H
#define EBADGE_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#define EBADGE_CHARACTER_COUNT 3
#define EBADGE_QUEUE_CAPACITY 32
#define EBADGE_REACTION_MS 1200u

enum ebadge_event_kind { EBADGE_TAP, EBADGE_PREV, EBADGE_NEXT,
                         EBADGE_ANIMATION_DONE };
enum ebadge_character_state { EBADGE_IDLE, EBADGE_REACTING };

struct ebadge_event
{
  enum ebadge_event_kind kind;
  uint32_t generation;
  uint32_t animation_token;
};

struct ebadge_state
{
  uint8_t character;
  enum ebadge_character_state character_state;
  uint32_t generation;
  uint32_t animation_token;
  uint32_t reaction_started_ms;
  uint32_t last_user_activity_ms;
  uint32_t dropped_events;
};

struct ebadge_controller
{
  struct ebadge_state state;
  struct ebadge_event queue[EBADGE_QUEUE_CAPACITY];
  uint8_t head;
  uint8_t count;
  int8_t pending_steps;
  bool pending_navigation;
};

/* All calls belong to the UI owner task. NOT an ISR/multithread queue.
 * Future sensor/Agent producers require a synchronized ingress adapter.
 * Display/AI state are deliberately not conflated with character state.
 */
void ebadge_controller_init(struct ebadge_controller *c, uint32_t now);
bool ebadge_controller_post(struct ebadge_controller *c,
                           struct ebadge_event event);
void ebadge_controller_step(struct ebadge_controller *c, uint32_t now);

#endif
