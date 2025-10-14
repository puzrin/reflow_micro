#pragma once

#if defined(RFL_SUPERMINI_BOARD_DRIVERS)
    #define LCD_OFF             {0}
    #define LCD_OK_COLOR        {10}
    #define LCD_WARM_COLOR      {100}
    #define LCD_HOT_COLOR       {200}
    #define LCD_VERY_HOT_COLOR  {255}
    #define LCD_BLE_COLOR       {255}
#else
    #define LCD_OFF             {0, 0, 0}
    #define LCD_OK_COLOR        {0, 128, 0}
    #define LCD_WARM_COLOR      {180, 180, 0}
    #define LCD_HOT_COLOR       {255, 0, 0}
    #define LCD_VERY_HOT_COLOR  {255, 255, 255}
    #define LCD_BLE_COLOR       {0, 0, 255}
#endif