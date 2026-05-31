#include "EPD.h"

typedef struct {
    uint8_t *Image;
    uint16_t width;
    uint16_t height;
    uint16_t widthMemory;
    uint16_t heightMemory;
    uint16_t color;
    uint16_t rotate;
    uint16_t widthByte;
    uint16_t heightByte;
} PAINT;

static PAINT Paint;


/*******************************************************************
    函数说明：创建图片缓存数组
    接口说明：*image  要传入的图片数组
               Width  图片宽度
               Heighe 图片长度
               Rotate 屏幕显示方向
               Color  显示颜色
    返回值：  无
*******************************************************************/
void Paint_NewImage(uint8_t *image,uint16_t Width,uint16_t Height,uint16_t Rotate,uint16_t Color)
{
	Paint.Image = 0x00;
	Paint.Image = image;
	Paint.color = Color;  
	Paint.widthMemory = Width;
	Paint.heightMemory = Height;  
	Paint.widthByte = (Width % 8 == 0)? (Width / 8 ): (Width / 8 + 1);
	Paint.heightByte = Height;     
	Paint.rotate = Rotate;
	if(Rotate==0||Rotate==180) 
	{
		Paint.width=Height;
		Paint.height=Width;
	} 
	else 
	{
		Paint.width = Width;
		Paint.height = Height;
	}
}				 

/*******************************************************************
    函数说明：清空缓冲区 
    接口说明：Color  像素点颜色参数
    返回值：  无
*******************************************************************/
void Paint_Clear(uint8_t Color)
{
	uint16_t X,Y;
	uint32_t Addr;
  for(Y=0;Y<Paint.heightByte;Y++) 
	{
    for(X=0;X<Paint.widthByte;X++) 
		{   
      Addr=X+Y*Paint.widthByte;//8 pixel =  1 byte
      Paint.Image[Addr]=Color;
    }
  }
}


/*******************************************************************
    函数说明：点亮一个像素点
    接口说明：Xpoint 像素点x坐标参数
              Ypoint 像素点Y坐标参数
              Color  像素点颜色参数
    返回值：  无
*******************************************************************/
void Paint_SetPixel(uint16_t Xpoint,uint16_t Ypoint,uint16_t Color)
{
	uint16_t X, Y;
	uint32_t Addr;
	uint8_t Rdata;		
    switch(Paint.rotate) 
		{
				case 0:
					if(Xpoint>=396)
					{
						Xpoint+=8;
					}
					X=Xpoint;
					Y=Ypoint;
					break;
			case 90:
					if(Ypoint>=396)
					{
						Ypoint+=8;
					}
					X=Paint.widthMemory-Ypoint-1;
					Y=Xpoint;
					break;
			case 180:
				  if(Xpoint>=396)
					{
						Xpoint+=8;
					}
					X=Paint.widthMemory-Xpoint-1;
					Y=Paint.heightMemory-Ypoint-1;
					break;

			case 270:
					if(Ypoint>=396)
					{
						Ypoint+=8;
					}
					X=Ypoint;
					Y=Paint.heightMemory-Xpoint-1;
					break;
				default:
						return;
    }
		Addr=X/8+Y*Paint.widthByte;
    Rdata=Paint.Image[Addr];
    if(Color==BLACK)
    {    
			Paint.Image[Addr]=Rdata&~(0x80>>(X % 8)); //将对应数据位置0
		}
    else
		{
      Paint.Image[Addr]=Rdata|(0x80>>(X % 8));   //将对应数据位置1  
		}
}

