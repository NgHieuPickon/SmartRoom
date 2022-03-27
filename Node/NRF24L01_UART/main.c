#include "stm8s.h"
#include "stdio.h"
#include "stm8s_it.h"
#include "stm8s_gpio.h"
#include "stm8s_clk.h"
#include "stm8s_conf.h"
#include "stm8s_uart1.h"

#include "nRF24L01.h"
#include "RF24.h"

#define PUTCHAR_PROTOTYPE int putchar (int c)
#define GETCHAR_PROTOTYPE int getchar (void)

// global variables
unsigned long elpstime;   // elapse timer (mS)
unsigned long elpstend;   // elapse timer check end
//
RF24 radio(0);         // nRF24L01 module instance

// delay wait (dly mS)
void delay(unsigned long dly){
  elpstend = elpstime + dly;
  while(elpstend > elpstime);
}

// report system clock (mS)
unsigned long millis(void){
  return elpstime;
}

//--------------------------------------------------------------------------------
//
//  Setup the system clock to run at 16MHz using the internal oscillator.
//
void InitialiseSystemClock()
{   
  CLK_DeInit();
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
  CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, DISABLE, CLK_CURRENTCLOCKSTATE_DISABLE);

}

//  Setup Timer 2 to generate an interrupt every 1mS based on a 16MHz clock.
void SetupTimer2()
{
  TIM2_DeInit();
  TIM2_TimeBaseInit(TIM2_PRESCALER_32, 500);
  TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
  TIM2_Cmd(ENABLE);
}

PUTCHAR_PROTOTYPE
{
  /* Write a character to the UART1 */
  UART1_SendData8(c);
  /* Loop until the end of transmission */
  while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);

  return (c);
}

GETCHAR_PROTOTYPE
{
  int c = 0;
  /* Loop until the Read data register flag is SET */
  while (UART1_GetFlagStatus(UART1_FLAG_RXNE) == RESET);
    c = UART1_ReceiveData8();
  return (c);
}

//--------------------------------------------------------------------------------
void SetupOutputPorts()
{
    GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_FAST);
}

//--------------------------------------------------------------------------------
//  set up SPI ports
//
// SPI control port assign
#define CEOUT  GPIO_PIN_1    // |= GPIO_PIN_1;
#define CSNOUT GPIO_PIN_4    // |= GPIO_PIN_4;

// Assign output ce & csn port for SPI port control
void SPICtrlPortInitial(void){
    GPIO_Init(GPIOD, CEOUT, GPIO_MODE_OUT_PP_HIGH_FAST);
    GPIO_WriteHigh(GPIOD, CEOUT);

    GPIO_Init(GPIOD, CSNOUT, GPIO_MODE_OUT_PP_HIGH_FAST);
    GPIO_WriteHigh(GPIOD, CSNOUT);
}

// Set SPI ce control port
void CEportSet(unsigned char val){ // val 0:Low level, !=0:High level
    if(val)
    {
      GPIO_WriteHigh(GPIOD, CEOUT);
    } else {
      GPIO_WriteLow(GPIOD, CEOUT);
    }
}

// Set SPI csn control port
void CSNportSet(unsigned char val){ // val 0:Low level, !=0:High level
    if(val)
    {
      GPIO_WriteHigh(GPIOD, CSNOUT);
    } else {
      GPIO_WriteLow(GPIOD, CSNOUT);
    }
}

// TX one byte
unsigned char TxSPIbyte(unsigned char data)
{
    SPI_Cmd(ENABLE);
    SPI_SendData(data);
    while(!(SPI_GetFlagStatus(SPI_FLAG_TXE)));
    SPI_Cmd(DISABLE);
    while(!(SPI_GetFlagStatus(SPI_FLAG_RXNE)));
    return SPI_ReceiveData();
}

//  Initialize SPI setting
void SetupSPImode()
{
  SPI_Cmd(DISABLE);
  SPI_DeInit();
  //  fmaster / 4 (4M baud)  16MHz/value  value 0:2, 1:4, 2:8, 3:16 ...
  //  Master device.
  SPI_CalculateCRCCmd(DISABLE);
  SPI_Init(SPI_FIRSTBIT_MSB, SPI_BAUDRATEPRESCALER_4, SPI_MODE_MASTER, SPI_CLOCKPOLARITY_LOW, SPI_CLOCKPHASE_1EDGE, SPI_DATADIRECTION_2LINES_FULLDUPLEX, SPI_NSS_HARD, SPI_CRCPR_RESET_VALUE);
}

void print(char* data, int size){
  for(int i = 0; i < size; i++){
    printf("%d", *data);
    data++;
  }
}

void println(char* data, int size){
  for(int i = 0; i < size; i++){
    printf("%d", *data);
    data++;
  }
  printf("\r\n");
}

//--------------------------------------------------------------------------------
//
//  Main program loop.
//
void main()
{
  // variables
    const char addresses[][6] = {"1Node","2Node"};
    unsigned char rxdevtime = 0;

    GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_PP_HIGH_FAST);
    GPIO_WriteHigh(GPIOB, GPIO_PIN_5);
  //
  //  Initialise the system.
  //
  InitialiseSystemClock();
  SetupTimer2();
  SetupOutputPorts();
  SetupSPImode();                     // set SPI mode
  SPICtrlPortInitial();               // set SPI control ports
  __enable_interrupt();
  delay(500);                         // wait module unit power up
  
  //Setup UART
  UART1_DeInit();
  UART1_Init((uint32_t)115200, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
              UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  printf("\n\rUART1 Example :retarget the C library printf()/getchar() functions to the UART\n\r");

  // set nRF214L01 initial condition
  radio.begin();                      // Start up the radio
  radio.setAutoAck(1);                // Ensure autoACK is enabled
  radio.setRetries(1, 1);           // Max delay between retries & number of retries
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(10);
  radio.openWritingPipe((const unsigned char *)addresses[0]);
  radio.openReadingPipe(1, (const unsigned char *)addresses[1]);
  radio.startListening();             // Start listening
  // loop process
  char tx[8];
  while (1)
  {
    rxdevtime = 0;
    for(char i = 0 ; i < 7 ; i++)
    {
      tx[i] = i;            
    }          
    // send data to nRF24L01 module
    // radio.stopListening();        // First, stop listening so we can talk
    // radio.write(tx, sizeof(tx));
    // printf("Data send: ");
    // println(tx, sizeof(tx));
    
    radio.startListening();
    //check data received
    while(radio.available()){
      radio.read(&rxdevtime, sizeof(unsigned long));
        printf("Data receive: %d\r\n", rxdevtime);\
        delay(1000);
        if(rxdevtime == 4){
          GPIO_WriteReverse(GPIOB, GPIO_PIN_5);
          // send data to nRF24L01 module
            radio.stopListening();        // First, stop listening so we can talk
            radio.write(tx, sizeof(tx));
            printf("Data send: ");
            println(tx, sizeof(tx));
        }
    }
  }   
}


void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}