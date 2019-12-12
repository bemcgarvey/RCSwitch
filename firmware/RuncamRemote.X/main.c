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

#define _XTAL_FREQ   1000000U

//define rx pulse length in microseconds required to turn on output.
#define ON_US       1600U
//define rx pulse length in microseconds required to turn off output.
#define OFF_US      1400U
//define double toggle time in ms
#define DOUBLE_TOGGLE_TIME      500

//calculation of Timer1 values
#define ON_PULSE    (ON_US / 4)
#define OFF_PULSE   (OFF_US / 4)

void configPins(void);
void configTimer1(void);
void startTimer1(void);
void configTimer2(void);
void configInterrupts(void);
void configPMD(void);
void pulse(char pulses);

typedef enum {INPUT_UNKNOWN, INPUT_ON, INPUT_OFF} InputState;
volatile InputState inputState = INPUT_UNKNOWN;
volatile bool toggled = false;
typedef enum {PULSE_LOW, PULSE_HIGH} PulseState;
volatile PulseState pulseState = PULSE_LOW;
volatile char pulseCount;

void main(void) {
    configPins();
    configPMD();
    configTimer1();
    configTimer2();
    configInterrupts();
    startTimer1();
    while (1) {
        if (toggled) {
            toggled = false;
            __delay_ms(DOUBLE_TOGGLE_TIME);
            if (toggled) {
                pulse(1);
            } else {
                pulse(2);
            }
            toggled = false;
        }
    }
}

void pulse(char pulses) {
    pulseCount = (pulses == 1) ? 1 : 3;
    pulseState = PULSE_HIGH;
    LATAbits.LATA2 = 1;
    TMR2 = 0;
    T2CONbits.TMR2ON = 1;
}

void configPins(void) {
    ANSELA = 0;
    LATA = 0b00000000;
    TRISA = 0b00100000;
    T1GPPS = 0b00101;
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
    //Set Timer2 for 80ms period.
    PR2 = 125;
    T2CONbits.T2CKPS = 0b10; //1:16 prescale
    T2CONbits.T2OUTPS = 0b1001; //1:10 postscale
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
    //Turn off everything except Timer1 and Timer2 and PCLCK
    PMD0 = 0b01000111;
    PMD1 = 0b10000001;
    PMD2 = 0b01100110;
    PMD3 = 0b01110011;
    PMD4 = 0b00100010;
    PMD5 = 0b00000111;
}

void __interrupt() isr(void) {
    if (PIR1bits.TMR2IF == 1) {
        if (pulseState == PULSE_HIGH) {
            LATAbits.LATA2 = 0;
            pulseState = PULSE_LOW;
        } else {
            LATAbits.LATA2 = 1;
            pulseState = PULSE_HIGH;
        }
        --pulseCount;
        if (pulseCount == 0) {
            T2CONbits.TMR2ON = 0;
        }
        PIR1bits.TMR2IF = 0;
    }
    if (PIR1bits.TMR1GIF == 1) {
        if (TMR1 <= OFF_PULSE) {
            if (inputState == INPUT_ON) {
                toggled = true;
            }
            inputState = INPUT_OFF;
        }
        else if (TMR1 >= ON_PULSE) {
            if (inputState == INPUT_OFF) {
                toggled = true;
            }
            inputState = INPUT_ON;
        }
        TMR1L = 0;
        TMR1H = 0;
        T1GCONbits.T1GGO_nDONE = 1;
        PIR1bits.TMR1GIF = 0;
    }
}