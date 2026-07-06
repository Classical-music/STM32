#ifndef __WWDG_H__
#define __WWDG_H__

#include "type.h"

typedef enum {
  WWDG_WDGTB_1 = 0,
  WWDG_WDGTB_2 = 0x80,
  WWDG_WDGTB_4 = 0x100,
  WWDG_WDGTB_8 = 0x180,
} wwdg_wdgtb_t;

void wwdg_init(uint8_t tr, uint8_t wr, wwdg_wdgtb_t wdgtb);

#endif
