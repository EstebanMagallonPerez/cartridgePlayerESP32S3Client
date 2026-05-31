#ifndef _EPD_GUI_H_
#define _EPD_GUI_H_

#include "EPD_Init.h"

#define Rotation 180

void Paint_NewImage(uint8_t *image,uint16_t Width,uint16_t Height,uint16_t Rotate,uint16_t Color);
void Paint_SetPixel(uint16_t Xpoint,uint16_t Ypoint,uint16_t Color);
void Paint_Clear(uint8_t Color);

#endif
