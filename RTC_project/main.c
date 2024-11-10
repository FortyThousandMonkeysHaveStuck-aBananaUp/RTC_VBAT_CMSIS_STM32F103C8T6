#define STM32F103xB
#include "stm32f1xx.h"

void System_Clock_Config(void);
void GPIO_PB12_and_PB15_Config(void);//PB12 as output and PB15 as input
void RTC_Config(void);
void delay_whithout_interruption(unsigned long wait);

//Interrupts RTC->CNTL; //0x view
void RTC_IRQHandler(void)
{
    if(RTC->CRL&RTC_CRL_SECF)
    {
        RTC->CRL&=~RTC_CRL_SECF;
        
        if(GPIOB->IDR&GPIO_IDR_IDR12)
          {GPIOB->BSRR|=GPIO_BSRR_BR12;}
        else 
          {GPIOB->BSRR|=GPIO_BSRR_BS12;}

    }
    
    if(RTC->CRL&RTC_CRL_ALRF)
    {
        RTC->CRH&=~RTC_CRH_ALRIE;
        RTC->CRL&=~RTC_CRL_ALRF;
        //do something
        delay_whithout_interruption(1);
    }
}



int main(void)
{
  System_Clock_Config();
  GPIO_PB12_and_PB15_Config();

  while(222)
  {
      if(GPIOB->IDR&GPIO_IDR_IDR15)
      {
        delay_whithout_interruption(50);
        if(GPIOB->IDR&GPIO_IDR_IDR15)//if it was not noise
        {
            RTC_Config();
            while(GPIOB->IDR&GPIO_IDR_IDR15);
            delay_whithout_interruption(250);
        }
      }
  }
  
  return 0;
}

void System_Clock_Config(void)
{
//Steps
//1 Latency
//2 Enable HSE and wait for
//3 PLL multiplication factor
//4 PLL entry clock source
//5 Enable PLL and wait for
//6 Set the prescalers
//HPRE
//PPRE1 is divided by 2
//PPRE2
//7 SW and wait for

    //1 Latency
    FLASH->ACR|=FLASH_ACR_LATENCY_1;

    //2 Enable HSE and wait for
    RCC->CR|=RCC_CR_HSEON;
    while(!(RCC->CR&RCC_CR_HSERDY));

    //3 PLL multiplication factor
    RCC->CFGR|=RCC_CFGR_PLLMULL9;

    //4 PLL entry clock source
    RCC->CFGR|=RCC_CFGR_PLLSRC;

    //5 Enable PLL and wait for
    RCC->CR|=RCC_CR_PLLON;
    while(!(RCC->CR&RCC_CR_PLLRDY));

    //6 Set the prescalers
    //HPRE
    RCC->CFGR&=~RCC_CFGR_HPRE;

    //PPRE1 is divided by 2
    RCC->CFGR|=RCC_CFGR_PPRE1_DIV2;

    //PPRE2
    RCC->CFGR&=~RCC_CFGR_PPRE2;

    //7 SW and wait for
    RCC->CFGR|=RCC_CFGR_SW_PLL;
    while(!(RCC->CFGR&RCC_CFGR_SWS_PLL));
}

void GPIO_PB12_and_PB15_Config(void)//PB12 as output and PB15 as input
{
//Steps
//1 Clock
//2 Reset
//3 Mode
//4 Configurate

    //1 Clock
    RCC->APB2ENR|=RCC_APB2ENR_IOPBEN;
    
    //2 Reset
    GPIOB->BRR=0xffff;

    //3 Mode
    GPIOB->CRH|=GPIO_CRH_MODE12;//11 for output mode

    GPIOB->CRH&=~GPIO_CRH_MODE15;//00 for input mode

    //4 Configurate
    GPIOB->CRH&=~GPIO_CRH_CNF12;

    GPIOB->CRH&=~GPIO_CRH_CNF15;
    GPIOB->CRH|=GPIO_CRH_CNF15_1;//10: Input with pull-up / pull-down
}

void RTC_Config(void)
{
//Steps
//1 RCC->APB1ENR PWREN=1 //1: Power interface clock enable
//2 PWR->CR DBP=1 //1: Access to RTC and Backup registers enabled
//3: Resets the entire Backup domain!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! The time reset in this place?
//4 RCC->BDCR //RTCSEL = 1  //01: LSE oscillator clock used as RTC clock
//5 LSEON = 1 and wait for the LSERDY bit to set//1: External 32 kHz oscillator ON
//6 RTCEN = 1 //1: RTC clock enabled //And the clock starts counting
//7 Configuration procedure:
//8 Interrupts

    //1 RCC->APB1ENR PWREN=1 //1: Power interface clock enable
    RCC->APB1ENR|=RCC_APB1ENR_PWREN;

    //2 PWR->CR DBP=1 //1: Access to RTC and Backup registers enabled
    PWR->CR|=PWR_CR_DBP;

    //3: Resets the entire Backup domain!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! The time reset in this place? 
    //The RTC_PRL, RTC_ALR, RTC_CNT, and RTC_DIV registers are reset only by a Backup Domain reset. Refer to Section 6.1.3 on page 75.
    RCC->BDCR|=RCC_BDCR_BDRST;
    RCC->BDCR&=~RCC_BDCR_BDRST;

    //4 RCC->BDCR //RTCSEL = 1  //01: LSE oscillator clock used as RTC clock
    RCC->BDCR&=~RCC_BDCR_RTCSEL;
    RCC->BDCR|=(0x1<<8);

    //5 LSEON = 1 and wait for the LSERDY bit to set//1: External 32 kHz oscillator ON
    RCC->BDCR|=RCC_BDCR_LSEON;
    while(!(RCC->BDCR&RCC_BDCR_LSERDY));

    //6 RTCEN = 1 //1: RTC clock enabled //And the clock starts counting
    RCC->BDCR|=RCC_BDCR_RTCEN;


    //7 Configuration procedure:
    //1. Poll RTOFF, wait until its value goes to Ñ1Ò
    //2.  Set the CNF bit to enter configuration mode
    //3.  Write to one or more RTC registers
    //4.  Clear the CNF bit to exit configuration mode
    //5.  Poll RTOFF, wait until its value goes to Ñ1Ò to check the end of the write operation.
    //The write operation only executes when the CNF bit is cleared; it takes at least three 
    //RTCCLK cycles to complete.

        //Configuration procedure: //1. Poll RTOFF, wait until its value goes to Ñ1Ò
        while(!(RTC->CRL&RTC_CRL_RTOFF));//RTC_PRLH / RTC_PRLL a write operation is allowed if the RTOFF value is Ñ1Ò.
        
        //Configuration procedure: //2.  Set the CNF bit to enter configuration mode
        RTC->CRL|=RTC_CRL_CNF;

        //Configuration procedure://3.  Write to one or more RTC registers
        //Prescaler 0x7FFF
        //32768
        //37740
        //0.99966 for 0x7FF9
        //0.99984 for 0x7FFF 0.00016
        //1.00015 for 0x8009 0.00015
        RTC->PRLH=0;
        RTC->PRLL=0x8009;//If the input clock frequency (fRTCCLK) is 32.768 kHz, write 7FFFh in this register (PRL[19:0]) to get a signal period of 1 second. 
        //365 days in year
        //24 hours in day
        //3600 seconds in minute
        //3600*24*365=31536000 seconds in year
        //if the 1 second = 0.999997 second
        //31536000 seconds * 0.999997 second = 31535905,392
        //delta=-94,608 seconds per year
        //~8 seconds per month

        //Alarm do nothing

        //Configuration procedure://4.  Clear the CNF bit to exit configuration mode
        RTC->CRL&=~RTC_CRL_CNF;
        
        //Configuration procedure://5.  Poll RTOFF, wait until its value goes to Ñ1Ò to check the end of the write operation.
        while(!(RTC->CRL&RTC_CRL_RTOFF));

    //8 Interrupts
    RTC->CRL&=~RTC_CRL_SECF;
    RTC->CRL&=~RTC_CRL_ALRF;
    RTC->CRH|=RTC_CRH_SECIE;
    RTC->CRH|=RTC_CRH_ALRIE;
    NVIC_SetPriority(RTC_IRQn, 1);
    NVIC_EnableIRQ(RTC_IRQn);
}

void delay_whithout_interruption(unsigned long wait)
{
    wait*=10300;//what is a limit? //1~1mS  if HCLK to core = 75 MHz  x must be 
    while(--wait);
}