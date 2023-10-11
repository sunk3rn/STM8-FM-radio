#include "stm8s.h"
#include "milis.h"
#include "stm8_hd44780.h"
#include "stdio.h"
#include "swi2c.h"

char text[]="STM8 - FM Radio";
char text2[16];
#define SLVADR (0b1100000 <<1)
uint16_t frekvence = 1034; //Hlavni promena pro frekvenci
uint16_t prevedena_frekvence;
uint8_t frekvenceLSB;
uint8_t frekvenceMSB;
//uint8_t pritomnost; // vyjadruje stav FM modulu, aa - busy(neni),00 - success
uint8_t zprava[5] = {0b00110001,0b01101001,0b00010000,0b00010010,0b01000000};//Promena co posila zpravu do FM modulu
uint8_t mute_flag = 0;
uint16_t oblibena_frekvence = 0;

void inicializace (void){
	GPIO_Init(GPIOC,GPIO_PIN_1,GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOC,GPIO_PIN_4,GPIO_MODE_IN_PU_NO_IT);
	GPIO_Init(GPIOC,GPIO_PIN_2,GPIO_MODE_IN_PU_NO_IT); //GREEN
	GPIO_Init(GPIOG,GPIO_PIN_0,GPIO_MODE_IN_PU_NO_IT); //RED
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	init_milis();
	
	lcd_init();	
	
	swi2c_init();
}

uint16_t frekvence_vypocitat ( uint16_t zad_frekvence)//Zadavat frekvenci v MHz /10 napr: 87.9 MHz = 879  
{
	uint32_t vysledek;
	vysledek = (4 * ((zad_frekvence + 2) * 100000 + 225 * 1000)) / 32768;
	if (vysledek <= 16384 && vysledek >= 0)
	{
		return vysledek;
	}
	else
	{
		return 0;
	}
}

void nastavit_frekvenci(void){
	prevedena_frekvence = frekvence_vypocitat(frekvence);

	//Prohodil jsem MSB a LSB, nevim co je spravne
	frekvenceMSB = prevedena_frekvence >> 8;
	frekvenceLSB = prevedena_frekvence & 0xFF;
	if (mute_flag == 1){
		frekvenceMSB & 0xC0;
		frekvenceMSB += 0b10000000;
	} 
		
	zprava[0] = frekvenceMSB;	//Prvni byte zpravy
	zprava[1] = frekvenceLSB; //Druhy byte zpravy
	
	swi2c_block_write(SLVADR,zprava,5);	
	//pritomnost = swi2c_test_slave(SLVADR);
}

void obnovit_displej(void){
	uint16_t vyst_frek1 = frekvence / 10;
	uint16_t vyst_frek2 = frekvence % 10;
	nastavit_frekvenci();
	lcd_clear();
	lcd_gotoxy(0,0);
	lcd_puts(text);
	lcd_gotoxy(0,1);
	if (mute_flag == 1){sprintf(text2,"Muted %d.%d MHz ",vyst_frek1,vyst_frek2);}
	else{sprintf(text2,"     %d.%d MHz ",vyst_frek1,vyst_frek2);}
	lcd_puts(text2);
}

//Kod pro lepsi tlacitka
#define SCAN_PERIOD 15 //Čas mezi skenováním

void scan_buttons(void);

 INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13)
 {
     /*Binec, ingorovat
		*/
 }

void main(void)
{
	inicializace();
	
	nastavit_frekvenci();
	oblibena_frekvence = frekvence;
	
	//TIM2_TimeBaseInit(TIM2_Prescaler_TypeDef TIM2_Prescaler, uint16_t TIM2_Period)
	//TIM2_TimeBaseInit(TIM2_PRESCALER_1,40000);
	//TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
	//TIM2_Cmd(ENABLE);
	
	obnovit_displej();
	
while(1){
	scan_buttons();
	

	}
}

// po uplynutí každých SCAN_PERIOD skenuje stav tlačítka (tlačítek) a reaguje na stisk
void scan_buttons(void){
	static uint16_t last_time=0; // kdy naposled jsme stav tlačítek skenovali
	static bool last_button_status=1; // minuly stav tlačítka (0=stisk, 1=uvolněno)
	if((milis() - last_time) > SCAN_PERIOD){ // pokud uplynul požadovaný čas, tak skenujeme stav tlačítek
        last_time = milis(); // zapamatuj si kdy jsi skenoval stav tlačítek
		// bylo tlačítko minule uvolněné a teď je stisknuté ?
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_1)==RESET && last_button_status==1){ 
			last_button_status = 0; // zapamatuj si že teď je tlačítko stisknuté
			// sem davej kod pro vykonani po zmacknuti:
			frekvence++;
			if (frekvence > 1080){
			frekvence = 875;}
			obnovit_displej();
			nastavit_frekvenci();
			
		}
		// pokud je tlačítko uvolněné, zapamatuj si to 
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_1)!=RESET){
		last_button_status=1;
		delay_ms(75);
		}
		
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_4)==RESET && last_button_status==1){ 
			last_button_status = 0; // zapamatuj si že teď je tlačítko stisknuté
			// sem davej kod pro vykonani po zmacknuti:
			frekvence--;
			if (frekvence < 875){
			frekvence = 1080;}
			obnovit_displej();
			nastavit_frekvenci();
			
		}
		// pokud je tlačítko uvolněné, zapamatuj si to 
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_4)!=RESET){
		last_button_status=1;
		delay_ms(75);
		}
		
		if(GPIO_ReadInputPin(GPIOG,GPIO_PIN_0)==RESET && last_button_status==1){ 
			last_button_status = 0; // zapamatuj si že teď je tlačítko stisknuté
			// sem davej kod pro vykonani po zmacknuti:
			if (mute_flag){mute_flag = 0;}
			else{mute_flag = 1;}
			obnovit_displej();
			nastavit_frekvenci();
			delay_ms(50);
			
		}
		// pokud je tlačítko uvolněné, zapamatuj si to 
		if(GPIO_ReadInputPin(GPIOG,GPIO_PIN_0)!=RESET){
		last_button_status=1;
		delay_ms(75);
		}
		
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_2)==RESET && last_button_status==1){ 
			last_button_status = 0; // zapamatuj si že teď je tlačítko stisknuté
			// sem davej kod pro vykonani po zmacknuti:
			if (mute_flag == 1){
				oblibena_frekvence = frekvence;
			}
			else{
				frekvence = oblibena_frekvence;
			}
			obnovit_displej();
			nastavit_frekvenci();
			
		}
		// pokud je tlačítko uvolněné, zapamatuj si to 
		if(GPIO_ReadInputPin(GPIOC,GPIO_PIN_2)!=RESET){
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
