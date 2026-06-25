#include "board.h"
#include "wwdg.h"

void board_init(void) {
  systick_init(1000);
  wwdg_init(0x7F, 0x5F, WWDG_WDGTB_8); // 初始化看门狗
}
