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
#include <stdlib.h>
#include "oled.h"

uint8_t  AppData[1024*150] __attribute__ ((at(0X20001000)));
uint8_t *p1=AppData;

int main(void)
{
	u32 tt;
 	u32 total;
	uint32_t FileSize;
	__IO uint32_t FlashAddr;

	float Progress;
	
	HAL_Init();                     //��ʼ��HAL��   
	Stm32_Clock_Init(360,25,2,8);   //����ʱ��,180Mhz
	delay_init(180);                //��ʼ����ʱ����
	uart_init(115200);              //��ʼ��USART
	KEY_Init();                     //��ʼ������
	OLED_Init();
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET);
//	delay_ms(20);
//	*(p1+1024*150) = 27;

 	while(SD_Init())//��ⲻ��SD��
	{
	}
 	exfuns_init();							//Ϊfatfs��ر��������ڴ�				 
	if(f_mount(fs[0],"0:",1)!=FR_OK )			//����SD�� 
	{
		printf("����ʧ��=%d\r\n",f_mount(fs[0],"0:",1));
	}
	 if(f_open(file,(TCHAR*)"0:/Stethoscope.bin",FA_READ) !=  FR_OK)/* ���ļ� */
	{
		printf("δ�����¹̼�=%d\r\n",f_open(file,(TCHAR*)"0:/Stethoscope.bin",FA_READ));
		/* �ر��ļ� */
		f_close(file);
		//free((void *)p1);
		/* ж���ļ�ϵͳ */
		f_mount(NULL,"0:", 0);
	}	   
	else
	{
		FileSize = f_size(file);
		printf("size=%d\n\r",FileSize);
		printf("��ʼ���¹̼�...\r\n\r\n");
		Show_String(16, 32, (uint8_t *)"Updating FirmWare");
		OLED_Refresh_Gram();
		printf("������...\r\n\r\n");
		delay_ms(100);	
		while(1)
		{
			if(f_read(file, (void *)p1, 2048, &bw) != FR_OK)
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
		printf("\r\n�̼�������ɣ���\r\n");
		f_close(file);
		f_unlink("0:/Stethoscope.bin");							/* ɾ���̼��ļ� */
		OLED_Clear();
		__set_FAULTMASK(1);												/* �ر������ж� */
		NVIC_SystemReset();												/* �����λ */
	}
	tt = ((*(vu32*)(FLASH_APP1_ADDR+4))&0xFF000000);
	printf("tt=%x\n\r",tt);
	printf("&tt=%p\n\r",&tt);
	OLED_Clear();
//	INTX_DISABLE();
	if(((*(__IO uint32_t*) (FLASH_APP1_ADDR + 4)) & 0xFF000000 ) == 0x8000000)//�ж��Ƿ�Ϊ0X08XXXXXX.
	{	 
		iap_load_app(FLASH_APP1_ADDR);//ִ��FLASH APP����
	}else 
	{
		printf("��FLASHӦ�ó���,�޷�ִ��!\r\n");
	}				 
	
	while(1)
	{
	} 
}
