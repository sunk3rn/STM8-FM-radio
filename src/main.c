#include "stm8s.h"
#include "milis.h"
#include "stm8_hd44780.h"
#include "stdio.h"
#include "swi2c.h"

char text[]="STM8 - FM Radio";
char text2[16];
#define SLVADR (0b1100000 <<1)
uint16_t frequency = 1034; //Main frequency variable
uint16_t converted_frequency;
uint8_t frequencyLSB;
uint8_t frequencyMSB;
uint8_t message[5] = {0b00110001,0b01101001,0b00010000,0b00010010,0b01000000}; //The variable that sends the frequency to the FM module
uint8_t mute_flag = 0;
uint16_t favorite_frequency = 0;

void initialization (void){
	GPIO_Init(GPIOC,GPIO_PIN_1,GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOC,GPIO_PIN_4,GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOC,GPIO_PIN_2,GPIO_MODE_IN_PU_NO_IT); //GREEN
	GPIO_Init(GPIOG,GPIO_PIN_0,GPIO_MODE_IN_PU_NO_IT); //RED
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	init_milis();
	
	lcd_init();	
	
	swi2c_init();
}

uint16_t calculate_frequency ( uint16_t wanted_frequency)  
{
	uint32_t result;
	result = (4 * ((wanted_frequency + 2) * 100000 + 225 * 1000)) / 32768;
	if (result <= 16384 && result >= 0) {
		return resutl;
	}
	else {
		return 0;
	}
}

void set_frequency(void){
	converted_frequency = calculate_frequency(frequency);

	//Prohodil jsem MSB a LSB, nevim co je spravne
	frequencyMSB = converted_frequency >> 8;
	frequencyLSB = converted_frequency & 0xFF;
	if (mute_flag == 1){
		frequencyMSB & 0xC0;
		frequencyMSB += 0b10000000;
	} 
		
	message[0] = frequencyMSB;	//First byte of the message
	message[1] = frequencyLSB; //Second byte of the message
	
	swi2c_block_write(SLVADR,message,5);	
}

void refresh_lcd(void){
	uint16_t output_frequency1 = frequency / 10;
	uint16_t output_frequency2 = frequency % 10;
	set_frequency();
	lcd_clear();
	lcd_gotoxy(0,0);
	lcd_puts(text);
	lcd_gotoxy(0,1);
	if (mute_flag == 1){sprintf(text2,"Muted %d.%d MHz ",output_frequency1,output_frequency2);}
	else{sprintf(text2,"     %d.%d MHz ",output_frequency1,output_frequency2);}
	lcd_puts(text2);
}

#define SCAN_PERIOD 15 //Scanning period for buttons

void scan_buttons(void);

 INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13) {
     /*Binec, ingorovat
		*/
 }

void main(void) {
	inicializace();	
	set_frequency();
	favorite_frequency = frequency;
	refresh_lcd();
	
while(1) {
	scan_buttons();
	}
}

// po uplynutí každých SCAN_PERIOD skenuje stav tlačítka (tlačítek) a reaguje na stisk
void scan_buttons(void) {
	static uint16_t last_time=0; // kdy naposled jsme stav tlačítek skenovali
	static bool last_button_status=1; // minuly stav tlačítka (0=stisk, 1=uvolněno)
	if((milis() - last_time) > SCAN_PERIOD) { // pokud uplynul požadovaný čas, tak skenujeme stav tlačítek
        last_time = milis(); // zapamatuj si kdy jsi skenoval stav tlačítek
		// bylo tlačítko minule uvolněné a teď je stisknuté ?
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_1)==RESET && last_button_status==1) { 
			last_button_status = 0; // zapamatuj si že teď je tlačítko stisknuté
			// sem davej kod pro vykonani po zmacknuti:
			frequency++;
			if (frequency > 1080){
			frequency = 875;}
			refresh_lcd();
			set_frequency();
			
		}
		// pokud je tlačítko uvolněné, zapamatuj si to 
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_1)!=RESET){
		last_button_status=1;
		delay_ms(75);
		}
		
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_4)==RESET && last_button_status==1){ 
			last_button_status = 0; // zapamatuj si že teď je tlačítko stisknuté
			// sem davej kod pro vykonani po zmacknuti:
			frequency--;
			if (frequency < 875){
			frequency = 1080;}
			refresh_lcd();
			set_frequency();
			
		}
		// pokud je tlačítko uvolněné, zapamatuj si to 
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_4)!=RESET) {
			last_button_status=1;
			delay_ms(75);
		}
		
		if(GPIO_ReadInputPin(GPIOG,GPIO_PIN_0)==RESET && last_button_status==1) { 
			last_button_status = 0; // zapamatuj si že teď je tlačítko stisknuté
			// sem davej kod pro vykonani po zmacknuti:
			if (mute_flag){mute_flag = 0;}
			else{mute_flag = 1;}
			refresh_lcd();
			set_frequency();
			delay_ms(50);
			
		}
		// pokud je tlačítko uvolněné, zapamatuj si to 
		if(GPIO_ReadInputPin(GPIOG,GPIO_PIN_0)!=RESET) {
		last_button_status=1;
		delay_ms(75);
		}
		
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_2)==RESET && last_button_status==1) { 
			last_button_status = 0; // zapamatuj si že teď je tlačítko stisknuté
			// sem davej kod pro vykonani po zmacknuti:
			if (mute_flag == 1) {
				favorite_frequency = frequency;
			}
			else {
				frequency = oblibena_frequency;
			}
			refresh_lcd();
			set_frequency();
			
		}
		// pokud je tlačítko uvolněné, zapamatuj si to 
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_2)!=RESET) {
		last_button_status=1;
		delay_ms(75);
		}
	}
}

// pod tímto komentářem nic neměňte 
#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
