#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "key.h"
#include "string.h"
#include "ff.h"
#include "exfuns.h"
#include "string.h"
#include "sdio_sdcard.h"
#include "IAP.h"
uint8_t  AppData[1024*150]  __attribute__ ((at(0X20001000)));
uint8_t *p1=AppData;

int main(void)
{
	u32 tt;
 	u32 total,free;
	uint32_t FileSize;
	__IO uint32_t FlashAddr;

	float Progress;
	
	HAL_Init();                     //初始化HAL库   
	Stm32_Clock_Init(360,25,2,8);   //设置时钟,180Mhz
	delay_init(180);                //初始化延时函数
	uart_init(115200);              //初始化USART
	KEY_Init();                     //初始化按键
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET);

 	while(SD_Init())//检测不到SD卡
	{
	}
 	exfuns_init();							//为fatfs相关变量申请内存				 
	if(f_mount(fs[0],"0:",1)!=FR_OK )			//挂载SD卡 
	{
		printf("挂载失败=%d\r\n",f_mount(fs[0],"0:",1));
	}
	 if(f_open(file,(TCHAR*)"0:/Stethoscope.bin",FA_READ) !=  FR_OK)/* 打开文件 */
	{
		printf("未发现新固件=%d\r\n",f_open(file,(TCHAR*)"0:/Stethoscope.bin",FA_READ));
		/* 关闭文件 */
		f_close(file);
		/* 卸载文件系统 */
		f_mount(NULL,"0:", 0);
	}	  
	else
	{
		FileSize = f_size(file);
		printf("size=%d\n\r",FileSize);
		printf("开始更新固件...\r\n\r\n");
		printf("擦除中...\r\n\r\n");
		delay_ms(100);	
		while(1)
		{
			if(f_read(file, p1, 2048, &bw) != FR_OK)
			{
					printf("fail");
			}
			else
			{
				p1+=bw;
				if(bw<2048) 
					break;
			}
		}
		iap_write_appbin(FLASH_APP1_ADDR, (uint8_t*)AppData, FileSize);
		HAL_FLASH_Lock(); 
		printf("\r\n固件更新完成！！\r\n");
		f_close(file);
		f_unlink("0:/Stethoscope.bin");							/* 删除固件文件 */
		__set_FAULTMASK(1);												/* 关闭所有中断 */
		NVIC_SystemReset();												/* 软件复位 */
	}
	tt = ((*(vu32*)(FLASH_APP1_ADDR+4))&0xFF000000);
	printf("tt=%x\n\r",tt);
	printf("&tt=%p\n\r",&tt);
	if(((*(__IO uint32_t*) (FLASH_APP1_ADDR + 4)) & 0xFF000000 ) == 0x8000000)//判断是否为0X08XXXXXX.
	{	 
		iap_load_app(FLASH_APP1_ADDR);//执行FLASH APP代码
	}else 
	{
		printf("非FLASH应用程序,无法执行!\r\n");
	}				 
	
	while(1)
	{
	} 
}
