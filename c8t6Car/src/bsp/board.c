#include "board.h"

void Board_Init(void)
{
    TICK_Init();
    LED_Init();
    SEG_Init();
    BEEP_Init();
}
