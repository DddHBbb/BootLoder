#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"
#include "string.h"
#include "usart.h"
#include "delay.h"

void Show_chars(uint8_t x, uint8_t y, uint8_t xend, uint8_t yend, uint8_t chr);
const uint8_t gImage_12[504];
//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127
//[1]0 1 2 3 ... 127
//[2]0 1 2 3 ... 127
//[3]0 1 2 3 ... 127
//[4]0 1 2 3 ... 127
//[5]0 1 2 3 ... 127
//[6]0 1 2 3 ... 127
//[7]0 1 2 3 ... 127
u8 OLED_GRAM[128][8];
uint8_t BodyData[16];

//更新显存到LCD
void OLED_Refresh_Gram(void)
{
    u8 i, n;
	INTX_DISABLE();
    for (i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD); //设置页地址（0~7）
        OLED_WR_Byte(0x00, OLED_CMD);     //设置显示位置—列低地址
        OLED_WR_Byte(0x10, OLED_CMD);     //设置显示位置—列高地址
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(OLED_GRAM[n][i], OLED_DATA);
    }
		INTX_ENABLE();
}
//向SSD1306写入一个字节。
//dat:要写入的数据/命令
//cmd:数据/命令标志 0,表示命令;1,表示数据;
void OLED_WR_Byte(u8 dat, u8 cmd)
{
    u8 i;
    OLED_RS = cmd; //写命令
    OLED_CS = 0;
    for (i = 0; i < 8; i++)
    {
        OLED_SCLK = 0;
        if (dat & 0x80)
            OLED_SDIN = 1;
        else
            OLED_SDIN = 0;
        OLED_SCLK = 1;
        dat <<= 1;
    }
    OLED_CS = 1;
    OLED_RS = 1;
}

//开启OLED显示
void OLED_Display_On(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); //SET DCDC命令
    OLED_WR_Byte(0X14, OLED_CMD); //DCDC ON
    OLED_WR_Byte(0XAF, OLED_CMD); //DISPLAY ON
}
//关闭OLED显示
void OLED_Display_Off(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); //SET DCDC命令
    OLED_WR_Byte(0X10, OLED_CMD); //DCDC OFF
    OLED_WR_Byte(0XAE, OLED_CMD); //DISPLAY OFF
}
//清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!
void OLED_Clear(void)
{
    u8 i, n;
    for (i = 0; i < 8; i++)
        for (n = 0; n < 128; n++)
            OLED_GRAM[n][i] = 0X00;
    OLED_Refresh_Gram(); //更新显示
}
//画点
//x:0~127
//y:0~63
//t:1 填充 0,清空
void OLED_DrawPoint(u8 x, u8 y, u8 t)
{
    u8 pos, bx, temp = 0;
    if (x > 127 || y > 63)
        return; //超出范围了.
    pos = 7 - y / 8;
    bx = y % 8;
    temp = 1 << (7 - bx);
    if (t)
        OLED_GRAM[x][pos] |= temp;
    else
        OLED_GRAM[x][pos] &= ~temp;
}
//x1,y1,x2,y2 填充区域的对角坐标
//确保x1<=x2;y1<=y2 0<=x1<=127 0<=y1<=63
//dot:0,清空;1,填充
void OLED_Fill(u8 x1, u8 y1, u8 x2, u8 y2, u8 dot)
{
    u8 x, y;
    for (x = x1; x <= x2; x++)
    {
        for (y = y1; y <= y2; y++)
            OLED_DrawPoint(x, y, dot);
    }
}
//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示
//size:选择字体 12/16/24
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 size, u8 mode)
{
    u8 temp, t, t1;
    u8 y0 = y;
    u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); //得到字体一个字符对应点阵集所占的字节数
    chr = chr - ' ';                                           //得到偏移后的值
    for (t = 0; t < csize; t++)
    {
        if (size == 12)
            temp = asc2_1206[chr][t]; //调用1206字体
        else if (size == 16)
            temp = asc2_1608[chr][t]; //调用1608字体
        else if (size == 24)
            temp = asc2_2412[chr][t]; //调用2412字体
        else
            return; //没有的字库
        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
                OLED_DrawPoint(x, y, mode);
            else
                OLED_DrawPoint(x, y, !mode);
            temp <<= 1;
            y++;
            if ((y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}
//m^n函数
u32 mypow(u8 m, u8 n)
{
    u32 result = 1;
    while (n--)
        result *= m;
    return result;
}
//显示2个数字
//x,y :起点坐标
//len :数字的位数
//size:字体大小
//mode:模式	0,填充模式;1,叠加模式
//num:数值(0~4294967295);
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size, u8 mode)
{
    u8 t, temp;
    u8 enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / mypow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + (size / 2) * t, y, ' ', size, mode);
                continue;
            }
            else
                enshow = 1;
        }
        OLED_ShowChar(x + (size / 2) * t, y, temp + '0', size, mode);
    }
}
//显示字符串
//x,y:起点坐标
//size:字体大小
//*p:字符串起始地址
void OLED_ShowString(u8 x, u8 y, const u8 *p, u8 size)
{
    while ((*p <= '~') && (*p >= ' ')) //判断是不是非法字符!
    {
        if (x > (128 - (size / 2)))
        {
            x = 0;
            y += size;
        }
        if (y > (64 - size))
        {
            y = x = 0;
            OLED_Clear();
        }
        OLED_ShowChar(x, y, *p, size, 1);
        x += size / 2;
        p++;
    }
}
//初始化SSD1306
void OLED_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;

    __HAL_RCC_GPIOA_CLK_ENABLE(); //使能GPIOA时钟
    __HAL_RCC_GPIOD_CLK_ENABLE(); //使能GPIOD时钟

    //GPIO初始化设置
    GPIO_Initure.Pin = GPIO_PIN_15;          //PA15
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP; //推挽输出
    GPIO_Initure.Pull = GPIO_PULLUP;         //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FAST;    //高速
    HAL_GPIO_Init(GPIOA, &GPIO_Initure);     //初始化

    //PB4,7
    GPIO_Initure.Pin = GPIO_PIN_4 | GPIO_PIN_0;
    HAL_GPIO_Init(GPIOD, &GPIO_Initure); //初始化

    //PC6,7
    GPIO_Initure.Pin = GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOD, &GPIO_Initure); //初始化

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET);

    OLED_SDIN = 1;
    OLED_SCLK = 1;

    OLED_CS = 1;
    OLED_RS = 1;

    OLED_RST = 0;
    delay_ms(100);
    OLED_RST = 1;

    OLED_WR_Byte(0xAE, OLED_CMD); //关闭显示
    OLED_WR_Byte(0xD5, OLED_CMD); //设置时钟分频因子,震荡频率
    OLED_WR_Byte(80, OLED_CMD);   //[3:0],分频因子;[7:4],震荡频率
    OLED_WR_Byte(0xA8, OLED_CMD); //设置驱动路数
    OLED_WR_Byte(0X3F, OLED_CMD); //默认0X3F(1/64)
    OLED_WR_Byte(0xD3, OLED_CMD); //设置显示偏移
    OLED_WR_Byte(0X00, OLED_CMD); //默认为0
    OLED_WR_Byte(0x40, OLED_CMD); //设置显示开始行 [5:0],行数.
    OLED_WR_Byte(0x8D, OLED_CMD); //电荷泵设置
    OLED_WR_Byte(0x14, OLED_CMD); //bit2，开启/关闭
    OLED_WR_Byte(0x20, OLED_CMD); //设置内存地址模式
    OLED_WR_Byte(0x02, OLED_CMD); //[1:0],00，列地址模式;01，行地址模式;10,页地址模式;默认10;
    OLED_WR_Byte(0xA1, OLED_CMD); //段重定义设置,bit0:0,0->0;1,0->127;
    OLED_WR_Byte(0xC0, OLED_CMD); //设置COM扫描方向;bit3:0,普通模式;1,重定义模式 COM[N-1]->COM0;N:驱动路数
    OLED_WR_Byte(0xDA, OLED_CMD); //设置COM硬件引脚配置
    OLED_WR_Byte(0x12, OLED_CMD); //[5:4]配置
    OLED_WR_Byte(0x81, OLED_CMD); //对比度设置
    OLED_WR_Byte(0xEF, OLED_CMD); //1~255;默认0X7F (亮度设置,越大越亮)
    OLED_WR_Byte(0xD9, OLED_CMD); //设置预充电周期
    OLED_WR_Byte(0xf1, OLED_CMD); //[3:0],PHASE 1;[7:4],PHASE 2;
    OLED_WR_Byte(0xDB, OLED_CMD); //设置VCOMH 电压倍率
    OLED_WR_Byte(0x30, OLED_CMD); //[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;
    OLED_WR_Byte(0xA4, OLED_CMD); //全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏)
    OLED_WR_Byte(0xA6, OLED_CMD); //设置显示方式;bit0:1,反相显示;0,正常显示
    OLED_WR_Byte(0xAF, OLED_CMD); //开启显示
                                  //OLED_Clear();
}

//在指定地址开始显示一个ASCII字符
//x,y:显示开始坐标.
//xend,yend:x,y 方向的终点坐标
//chr:字符
void Show_chars(uint8_t x, uint8_t y, uint8_t xend, uint8_t yend, uint8_t chr)
{
    uint8_t temp;
    uint8_t pos, t;
    uint16_t y0 = y;
    uint8_t color = 1;
    uint8_t size = 12;

    if (chr > ' ')
        chr = chr - ' '; //得到偏移后的值
    else
        chr = 0; //小于空格的一律用空格代替,比如TAB键(键值为9)
    for (pos = 0; pos < size; pos++)
    {
        if (size == 12)
            temp = asc2_1206[chr][pos]; //调用1206字体
        else
            temp = asc2_1608[chr][pos]; //调用1608字体
        if (x > xend)
            return; //超区域了
        for (t = 0; t < 8; t++)
        {
            if (temp & 0x80)
                OLED_DrawPoint(x, y, color);
            else
                OLED_DrawPoint(x, y, !color);
            temp <<= 1;
            y++;
            if ((y - y0) == size || (y > yend))
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

//在指定位置开始,显示指定长度范围的字符串.
//x,y:显示开始坐标.
//xend:x方向的终止坐标
//str:字符串
void Show_String(uint8_t x, uint8_t y, uint8_t *str)
{
    u8 bHz = 0; //字符或者中文
    uint8_t size = 12;
    uint8_t xend = 0, yend = 0;

    xend = (strlen((const char *)str) * 14);
    yend = y + 12;
    while (*str != 0) //数据未结束
    {
        if (!bHz)
        {
            if (*str > 0x80)
                bHz = 1; //中文
            else         //字符
            {
                Show_chars(x, y, xend, yend, *str); //显示一个字符
                if ((xend - x) > size / 2)
                    x += size / 2; //字符,为全字的一半
                else
                    x += xend - x; //未完全显示
                if (x >= xend)
                    return; //超过了,退出
                str++;
            }
        }
    }
}
//画线
//x1,y1:起点坐标
//x2,y2:终点坐标
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1; //计算坐标增量
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;
    if (delta_x > 0)
        incx = 1; //设置单步方向
    else if (delta_x == 0)
        incx = 0; //垂直线
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0; //水平线
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)
        distance = delta_x; //选取基本增量坐标轴
    else
        distance = delta_y;
    for (t = 0; t <= distance + 1; t++) //画线输出
    {
        OLED_DrawPoint(uRow, uCol, dot); //画点
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}
//画矩形
//(x1,y1),(x2,y2):矩形的对角坐标
void OLED_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    OLED_DrawLine(x1, y1, x2, y1, 1);
    OLED_DrawLine(x1, y1, x1, y2, 1);
    OLED_DrawLine(x1, y2, x2, y2, 1);
    OLED_DrawLine(x2, y1, x2, y2, 1);
}


//这两个参数定义动画每帧的长宽。
#define MOVIE_XSIZE_1 64
#define MOVIE_YSIZE_1 63
#define FRAME_SIZE_1 MOVIE_XSIZE_1 *MOVIE_YSIZE_1 / 8

void Movie_Show_Img(uint8_t x, uint8_t y, uint32_t picx)
{
    uint16_t temp, t, t1;
    uint16_t y0 = y;
    picx *= FRAME_SIZE_1;
    for (t = 0; t < FRAME_SIZE_1; t++)
    {
        temp = gImage_12[picx + t]; //得到图片的一个字节数据
        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
                OLED_DrawPoint(x, y, 1);
            else
                OLED_DrawPoint(x, y, 0);
            temp <<= 1;
            y++;
            if ((y - y0) == MOVIE_YSIZE_1)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
    OLED_Refresh_Gram(); //更新显示到OLED
}
/*动态显示当前位置*/
void DispCurrentPosition(const uint8_t *pbuf)
{
    const uint8_t *buf = pbuf;
    uint16_t len = strlen((const char *)buf) * 6; /*12号字体*/
    static uint8_t old_data;
    if (old_data != BodyData[0]) /*是否和上次是同一个位置*/
    {
        old_data = BodyData[0];
    }
    if (len > 127) /*显示内容超过单屏长度*/
    {
        Show_String(48, 36, (uint8_t *)buf);
    }
    else /*显示未超过单屏长度*/
    {
        Show_String(48, 36, (uint8_t *)buf);
    }
}
void BluetoothDisp(u8 t)
{
    uint8_t xpos = 95, ypos = 0;
    OLED_DrawLine(xpos, ypos + 3, xpos + 6, ypos + 9, t);
    OLED_DrawLine(xpos, ypos + 9, xpos + 6, ypos + 3, t);
    OLED_DrawLine(xpos + 3, ypos, xpos + 3, ypos + 12, t);
    OLED_DrawLine(xpos + 3, ypos, xpos + 6, ypos + 3, t);
    OLED_DrawLine(xpos + 3, ypos + 12, xpos + 6, ypos + 9, t);
    OLED_DrawLine(xpos, ypos + 2, xpos + 4, ypos + 6, t);
    OLED_Refresh_Gram();
}
extern UART_HandleTypeDef UART3_Handler;


void VolumeShow(uint8_t x, uint8_t y, uint8_t xsize, uint8_t ysize, uint32_t picx, const uint8_t str[])
{
    uint16_t temp, t, t1;
    uint16_t y0 = y;
    uint16_t frame_size = xsize * ysize / 8;
    picx *= frame_size;
    for (t = 0; t < frame_size; t++)
    {
        temp = str[picx + t];
        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
                OLED_DrawPoint(x, y, 1);
            else
                OLED_DrawPoint(x, y, 0);
            temp <<= 1;
            y++;
            if ((y - y0) == ysize)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}


const uint8_t gImage_12[504] = { /* 0X01,0X01,0X3F,0X00,0X40,0X00, */
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X3F,0XC0,0X00,0X00,0X00,0X00,0X00,0X07,0XFF,0XFE,0X00,0X00,0X00,
0X00,0X00,0X3F,0XFF,0XFF,0X80,0X00,0X00,0X00,0X00,0XFF,0XFF,0XFF,0XE0,0X00,0X00,
0X00,0X03,0XFF,0XFF,0XFF,0XF8,0X00,0X00,0X00,0X07,0XFF,0XFF,0XFF,0XFC,0X00,0X00,
0X00,0X0F,0XFF,0XFF,0XFF,0XF8,0X04,0X00,0X00,0X3F,0XFF,0XFF,0XFF,0XF8,0X04,0X00,
0X00,0X7F,0XFF,0XFF,0XFF,0XF0,0X07,0XFC,0X00,0XFF,0XFF,0XFF,0XFF,0XE0,0X07,0XFC,
0X01,0XFF,0XFF,0XFF,0XFF,0XE0,0X04,0X00,0X01,0XFF,0XFF,0XFF,0XFF,0XC0,0X04,0X00,
0X03,0XFF,0XFF,0XFF,0XFF,0XC0,0X00,0X00,0X07,0XFF,0XFF,0XFF,0XFF,0X80,0X07,0XFC,
0X07,0XFF,0XFF,0XC1,0XFF,0X80,0X06,0XCC,0X0F,0XFF,0XFE,0X00,0X7F,0X80,0X04,0XCC,
0X0F,0XFF,0XF8,0XFA,0X3F,0X00,0X04,0XCC,0X1F,0XFF,0XF1,0XFE,0X0F,0X00,0X04,0XCC,
0X1F,0XFF,0XE7,0XD6,0X07,0X00,0X00,0X00,0X3F,0XFF,0XCF,0XD4,0X22,0X00,0X00,0X00,
0X3F,0XFF,0XCE,0XB4,0X32,0X00,0X07,0XFC,0X3F,0XFF,0X96,0XA4,0X30,0X00,0X00,0X0C,
0X7F,0XFF,0XB5,0XA4,0XB8,0X00,0X00,0X0C,0X7F,0XFF,0X35,0XA4,0XB8,0X00,0X00,0X0C,
0X7F,0XFF,0X25,0X26,0XB8,0X00,0X00,0X0C,0X7F,0XFF,0X2D,0X36,0X9C,0X00,0X00,0X00,
0X7F,0XFF,0X6D,0XB6,0XDC,0X00,0X07,0XFC,0X7F,0XFF,0X6D,0XB3,0XDC,0X00,0X00,0X0C,
0X7F,0XFF,0X6D,0X9B,0XDC,0X00,0X00,0X0C,0X7F,0XFF,0X2C,0XDB,0XDC,0X00,0X04,0X0C,
0X7F,0XFF,0X26,0XDB,0XD8,0X00,0X07,0X00,0X7F,0XFF,0X26,0XD9,0XD8,0X00,0X03,0XC0,
0X7F,0XFF,0XB6,0XC9,0X98,0X00,0X01,0XFC,0X3F,0XFF,0X96,0X4B,0XB0,0X00,0X03,0XC0,
0X3F,0XFF,0XC2,0X4A,0XB2,0X00,0X07,0X00,0X3F,0XFF,0XC2,0X5A,0XE6,0X00,0X04,0X00,
0X1F,0XFF,0XE2,0X5A,0XC7,0X00,0X00,0X00,0X1F,0XFF,0XF0,0X57,0X0F,0X00,0X07,0XFC,
0X0F,0XFF,0XF8,0X17,0X3F,0X00,0X04,0XCC,0X0F,0XFF,0XFE,0X00,0X7F,0X80,0X04,0XCC,
0X07,0XFF,0XFF,0XC1,0XFF,0X80,0X04,0XCC,0X07,0XFF,0XFF,0XFF,0XFF,0X80,0X04,0XCC,
0X03,0XFF,0XFF,0XFF,0XFF,0XC0,0X00,0X00,0X01,0XFF,0XFF,0XFF,0XFF,0XC0,0X07,0X98,
0X01,0XFF,0XFF,0XFF,0XFF,0XE0,0X07,0XDC,0X00,0XFF,0XFF,0XFF,0XFF,0XE0,0X0C,0XCC,
0X00,0X7F,0XFF,0XFF,0XFF,0XF0,0X06,0XCC,0X00,0X3F,0XFF,0XFF,0XFF,0XF8,0X06,0X7C,
0X00,0X0F,0XFF,0XFF,0XFF,0XF8,0X00,0X30,0X00,0X07,0XFF,0XFF,0XFF,0XFC,0X00,0X00,
0X00,0X01,0XFF,0XFF,0XFF,0XF8,0X00,0X00,0X00,0X00,0XFF,0XFF,0XFF,0XE0,0X00,0X00,
0X00,0X00,0X3F,0XFF,0XFF,0X80,0X00,0X00,0X00,0X00,0X07,0XFF,0XFE,0X00,0X00,0X00,
0X00,0X00,0X00,0X3F,0XC0,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,};








