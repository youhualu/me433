#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro

#include <stdio.h>

#include "i2c_master_noint.h"
#include "mpu6050.h"

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = FRCPLL // use fast frc oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = OFF // primary osc disabled
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576 // use largest wdt value
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz fast rc internal oscillator
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

#define PIC32_SYS_FREQ 48000000ul // 48 million Hz
#define PIC32_DESIRED_BAUD 230400 // Baudrate for RS232

void UART1_Startup(void);
void ReadUART1(char * string, int maxLength);
void WriteUART1(const char * string);

void blink();

int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0;
    TRISBbits.TRISB4 = 1;
    LATAbits.LATA4 = 0;
    LATBbits.LATB4 = 0;
    
    U1RXRbits.U1RXR = 0b0001; // U1RX is B6
    RPB7Rbits.RPB7R = 0b0001; // U1TX is B7
    
    UART1_Startup();
    
    __builtin_enable_interrupts();
    
    // init the imu
    init_mpu6050();
    
    char m_in[100]; // uart transfer in data
    char m_out[200]; // uart send out data
    int i;
    #define NUM_OF_DATA_POINTS 300 // # of data points collect at 100Hz
    float ax[NUM_OF_DATA_POINTS], ay[NUM_OF_DATA_POINTS], az[NUM_OF_DATA_POINTS], gx[NUM_OF_DATA_POINTS], gy[NUM_OF_DATA_POINTS], gz[NUM_OF_DATA_POINTS], temp[NUM_OF_DATA_POINTS];
    

    char who = whoami(); 
    if (who != 0x68){
        while(1){
            LATAbits.LATA4 = 1;
        }
    }
    
    char IMU_buf[IMU_ARRAY_LEN]; // raw 8 bit array for imu data

    while (1) {
        blink();
        
        ReadUART1(m_in,100); 
        for (i=0; i<NUM_OF_DATA_POINTS; i++){
            _CP0_SET_COUNT(0);
            burst_read_mpu6050(IMU_buf);
            ax[i] = conv_xXL(IMU_buf);
            ay[i] = conv_yXL(IMU_buf);
            az[i] = conv_zXL(IMU_buf);
            gx[i] = conv_xG(IMU_buf);
            gy[i] = conv_yG(IMU_buf);
            gz[i] = conv_zG(IMU_buf);
            temp[i] = conv_temp(IMU_buf);
            
            while(_CP0_GET_COUNT()<24000000/2/100){}
        }
        
        // print data
        for (i=0; i<NUM_OF_DATA_POINTS; i++){
            sprintf(m_out,"%d %f %f %f %f %f %f %f\r\n",NUM_OF_DATA_POINTS-i,ax[i],ay[i],az[i],gx[i],gy[i],gz[i],temp[i]);
            WriteUART1(m_out);
        }
        
    }
}

// Read from UART1
// block other functions until you get a '\r' or '\n'
// send the pointer to your char array and the number of elements in the array
void ReadUART1(char * message, int maxLength) {
  char data = 0;
  int complete = 0, num_bytes = 0;
  while (!complete) {
    if (U1STAbits.URXDA) { 
      data = U1RXREG;      
      if ((data == '\n') || (data == '\r')) {
        complete = 1;
      } else {
        message[num_bytes] = data;
        ++num_bytes;
        if (num_bytes >= maxLength) {
          num_bytes = 0;
        }
      }
    }
  }
  message[num_bytes] = '\0';
}

// Write a character array using UART1
void WriteUART1(const char * string) {
  while (*string != '\0') {
    while (U1STAbits.UTXBF) {
      ; // wait until tx buffer isn't full
    }
    U1TXREG = *string;
    ++string;
  }
}

void UART1_Startup() {
  // disable interrupts
  __builtin_disable_interrupts();

  // turn on UART1 without an interrupt
  U1MODEbits.BRGH = 0; // set baud to PIC32_DESIRED_BAUD
  U1BRG = ((PIC32_SYS_FREQ / PIC32_DESIRED_BAUD) / 16) - 1;

  // 8 bit, no parity bit, and 1 stop bit (8N1 setup)
  U1MODEbits.PDSEL = 0;
  U1MODEbits.STSEL = 0;

  // configure TX & RX pins as output & input pins
  U1STAbits.UTXEN = 1;
  U1STAbits.URXEN = 1;

  // enable the uart
  U1MODEbits.ON = 1;

  __builtin_enable_interrupts();
}

void blink(){
    LATAbits.LATA4 = 1;
    _CP0_SET_COUNT(0);
    while(_CP0_GET_COUNT()<600000){}
    LATAbits.LATA4 = 0;
    _CP0_SET_COUNT(0);
    while(_CP0_GET_COUNT()<600000){}
}