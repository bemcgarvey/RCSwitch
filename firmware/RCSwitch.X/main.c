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
#pragma config RSTOSC = HFINT1  // Power-up default value for COSC bits (HFINTOSC (1MHz))
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
#include <stdbool.h>

typedef enum {
    FAIL_OFF, FAIL_ON, FAIL_HOLD
} FailsafeMode;

typedef enum {
    OUTPUT_OFF, OUTPUT_ON
} OutputMode;

//Set failsafe mode to desired value before compiling
volatile FailsafeMode failsafeMode = FAIL_OFF;

volatile OutputMode output = OUTPUT_OFF;
volatile char failsafeEngaged = false;
volatile unsigned int lastSignal = 0;

#define _XTAL_FREQ   1000000U

//define rx pulse length in microseconds required to turn on output switch.
#define ON_US       1600U
//define rx pulse length in microseconds required to turn off output switch.
#define OFF_US      1400U
//define failsafe (no pulse detected) time in milliseconds
#define FAILSAFE_TIMEOUT    250

//calculation of Timer1 values
#define ON_PULSE    (ON_US / 4)
#define OFF_PULSE   (OFF_US / 4)

void configPins(void);
void configTimer1(void);
void startTimer1(void);
void configTimer2(void);
void startTimer2(void);
void configInterrupts(void);
void configPMD(void);

void main(void) {
    configPins();
    LATAbits.LATA4 = 1;
    __delay_ms(500);
    LATAbits.LATA4 = 0;
    configPMD();
    configTimer1();
    configTimer2();
    configInterrupts();
    startTimer1();
    startTimer2();
    while (1) {
        if (!failsafeEngaged) {
            if (output == OUTPUT_ON) {
                LATAbits.LATA4 = 1;
                LATAbits.LATA5 = 1;
            } else {
                LATAbits.LATA4 = 0;
                LATAbits.LATA5 = 0;
            }
        } else {  //failsafeEngaged
            if (failsafeMode == FAIL_ON) {
                LATAbits.LATA4 = 1;
                LATAbits.LATA5 = 1;
            } else if (failsafeMode == FAIL_OFF) {
                LATAbits.LATA4 = 0;
                LATAbits.LATA5 = 0;
            }
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
    T1CONbits.TMR1CS = 0b00; // Fosc/4
    T1CONbits.T1CKPS = 0b00; // 1:1 prescale
    T1GCONbits.TMR1GE = 1;
    T1GCONbits.T1GPOL = 1;
    T1GCONbits.T1GSPM = 1;
    T1GCONbits.T1GSS = 0b00;
    TMR1L = 0;
    TMR1H = 0;
}

void startTimer1(void) {
    T1CONbits.TMR1ON = 1;
    T1GCONbits.T1GGO_nDONE = 1;
}

void configTimer2(void) {
    TMR2 = 0;
    PR2 = 250; 
}

void startTimer2(void) {
    T2CONbits.TMR2ON = 1;
}

void configInterrupts(void) {
    PIR1bits.TMR1GIF = 0;
    PIE1bits.TMR1GIE = 1;
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
}

void configPMD(void) {
    PMD0 = 0b01000111;
    PMD1 = 0b10000001;
    PMD2 = 0b01100110;
    PMD3 = 0b01110011;
    PMD4 = 0b00100010;
    PMD5 = 0b00000111;
}

void __interrupt() isr(void) {
    if (PIR1bits.TMR2IF == 1) {
        if (lastSignal < FAILSAFE_TIMEOUT) {
            ++lastSignal;
        } else {
            failsafeEngaged = true;
        }
        PIR1bits.TMR2IF = 0;
    }
    if (PIR1bits.TMR1GIF == 1) {
        if (TMR1 <= OFF_PULSE) {
            output = OUTPUT_OFF;
        }
        if (TMR1 >= ON_PULSE) {
            output = OUTPUT_ON;
        }
        TMR1L = 0;
        TMR1H = 0;
        lastSignal = 0;
        failsafeEngaged = false;
        T1GCONbits.T1GGO_nDONE = 1;
        PIR1bits.TMR1GIF = 0;
    }
}