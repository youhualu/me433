#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include <stdio.h>  
#include"i2c_master_noint.h"
#include"ssd1306.h"
#include"font.h"

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
#pragma config USERID = 00000000 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = ON // allow multiple reconfigurations
#pragma config IOL1WAY = ON // allow multiple reconfigurations

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
    LATAbits.LATA4 = 0;
    TRISBbits.TRISB4 = 1;
    
    __builtin_enable_interrupts();
    
    i2c_master_setup();
    ssd1306_setup();
    ssd1306_clear();
    
    while (1) {
        // use _CP0_SET_COUNT(0) and _CP0_GET_COUNT() to test the PIC timing
        // remember the core timer runs at half the sysclk
        _CP0_SET_COUNT(0);
        while (_CP0_GET_COUNT() < 12e6){}
        LATAbits.LATA4 = 1-LATAbits.LATA4;
        
   
  // Print message and fps on the screen
        double time_0 = _CP0_GET_COUNT();
        char s[20] = {0};
        sprintf(s,"Good Morning"); //print message
        drawString(s, 0, 0);
        ssd1306_update();
        double time = _CP0_GET_COUNT();
        char framerate[50]; 
        sprintf(framerate, "fps = %2.0f", (double) ((24000000/(time-time_0)))); //calculate fps
        drawString(framerate, 65,25); 
        
    }
}

void drawChar(char m, char x, char y){
    for (char j = 0; j<=4; j++){
        char column = ASCII[m-' '][j];
        for (char k = 0; k <=7; k++){
            if (column & 0x01){
                ssd1306_drawPixel(x+j, y+k, 1);
            }else{
                ssd1306_drawPixel(x+j, y+k, 0);
            }
            column = column >> 1;
        }
    }
}

void drawString(const char* string, char x, char y) {
    while (*string != '\0') {
        drawChar(*string, x, y);
        string++;
        x = x+6;
    }
}
