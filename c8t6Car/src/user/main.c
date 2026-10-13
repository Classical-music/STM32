#include "board.h"
#include "seg/test_seg.h"
#include "beep/test_beep.h"

int main(void)
{
    Board_Init();

    while (1)
    {
        // Seg_Test();
        Beep_Test();
    }
}
