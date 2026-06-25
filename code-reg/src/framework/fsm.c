#include "fsm.h"

void fsm_init(fsm_t *fsm, state_t state, const transition_t *table,
              size_t tbl_size) {
  if (!fsm || !table || !tbl_size)
    return;
  fsm->current_state = state;
  fsm->table = table;
  fsm->table_size = tbl_size;
}

void fsm_dispatch(fsm_t *fsm, const event_t *evt) {
  for (size_t i = 0; i < fsm->table_size; i++) {
    const transition_t *t = &fsm->table[i];
    if (t->current_state == fsm->current_state && t->event_type == evt->type) {
      // 执行动作
      state_t next = (t->action) ? t->action(fsm, evt) : t->next_state;
      fsm->current_state = next;
    }
  }
}
