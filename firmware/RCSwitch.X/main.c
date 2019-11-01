/*
 * File:   main.c
 * Author: bemcg
 *
 * Created on October 30, 2019, 2:06 PM
 */

// PIC16F18313 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FEXTOSC = OFF    // FEXTOSC External Oscillator mode Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with 2x PLL (32MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; I/O or oscillator function on OSC2)
#pragma config CSWEN = OFF      // Clock Switch Enable bit (The NOSC and NDIV bits cannot be changed by user software)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config MCLRE = OFF      // Master Clear Enable bit (MCLR/VPP pin function is digital input; MCLR internally disabled; Weak pull-up under control of port pin's WPU control bit.)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config WDTE = OFF       // Watchdog Timer Enable bits (WDT disabled; SWDTEN is ignored)
#pragma config LPBOREN = OFF    // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out Reset disabled)
#pragma config BORV = LOW       // Brown-out Reset Voltage selection bit (Brown-out voltage (Vbor) set to 2.45V)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (The PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will not cause a Reset)
#pragma config DEBUG = OFF      // Debugger enable bit (Background debugger disabled)

// CONFIG3
#pragma config WRT = OFF        // User NVM self-write protection bits (Write protection off)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (HV on MCLR/VPP must be used for programming.)

// CONFIG4
#pragma config CP = OFF         // User NVM Program Memory Code Protection bit (User NVM code protection disabled)
#pragma config CPD = OFF        // Data NVM Memory Code Protection bit (Data NVM code protection disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

#define _XTAL_FREQ   32000000

void configPins(void);
void configTimer1(void);
void configInterrupts(void);

void main(void) {
    configPins();
    configTimer1();
    configInterrupts();
    T1CONbits.TMR1ON = 1;
    while (1) {
        TMR1L = 0;
        TMR1H = 0;
        T1GCONbits.T1GGO_nDONE = 1;
        while (T1GCONbits.T1GGO_nDONE == 1);
        if (TMR1 < 1400) {
            LATAbits.LATA4 = 0;
            LATAbits.LATA5 = 0;
        }
        if (TMR1 > 1600) {
            LATAbits.LATA4 = 1;
            LATAbits.LATA5 = 1;
        }
    }
}

void configPins(void) {
    ANSELA = 0;
    LATA = 0b00000000;
    TRISA = 0b00000100;
    T1GPPS = 0b00010;
    PPSLOCK = 0x55;
    PPSLOCK = 0xaa;
    PPSLOCKbits.PPSLOCKED = 1;
}
void configTimer1(void) {
    T1CONbits.TMR1CS = 0b00;  // Fosc/4
    T1CONbits.T1CKPS = 0b11;  // 1:8 prescale
    T1GCONbits.TMR1GE = 1;
    T1GCONbits.T1GPOL = 1;
    T1GCONbits.T1GSPM = 1;
    T1GCONbits.T1GSS = 0b00;
    
}

void configInterrupts(void) {
    
}

void __interrupt() isr(void) {
    
}