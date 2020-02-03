/**
 * Author : Tomas Nereca
 * login  : xnerec00
 * changes: This file is original by Tomas Nereca
 *          Some lines of code are inspirated by labs and fitkit 3sdk examples, so my own code is approximately 90%
 * date	  : 30.12.2018
 */

#include "MK60D10.h"
#include <stdio.h>      // sprintf()
#include <ctype.h>      // isdigit()
#include "header.h"

/********************************** GLOBALS ***********************************/

int melodyType = 1;             // Alarm melodyType type
int lightsType = 1;             // Light notification type

unsigned long alarmTime = 0;

int repeatNumber = 0;       // Number for alarm repetitions
int repeatDelay = 0;        // Time in seconds between repetitions
int remainingRepeat = 0;    // Remaining repetitions

int alarmOn = 0;            // Flag for on/off alarm
int buttonsEnabled = 0;     // Flag - buttons on/off

// Delay method (lab.3)
void Delay(long long bound) {
    for (long long i = 0; i < bound; i++) {
        ;
    }
}

/****************************************************** INITIALIZATIONS ****************************************************/

// Initialize the MCU (set up clock, stop watchdog)
void MCUInit() {
    MCG_C4 |= (MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01));
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK;
}

// Initialize ports
void PortsInit()
{
    // Clock ports
    SIM->SCGC1 |= SIM_SCGC1_UART5_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;
    SIM->SCGC6 = SIM_SCGC6_RTC_MASK;

    // LEDs
    PORTB->PCR[2] = PORT_PCR_MUX(0x01);
    PORTB->PCR[3] = PORT_PCR_MUX(0x01);
    PORTB->PCR[4] = PORT_PCR_MUX(0x01);
    PORTB->PCR[5] = PORT_PCR_MUX(0x01);

    PORTA->PCR[4] = (0 | PORT_PCR_MUX(0x01)); // Beeper
    PORTE->PCR[8] = (0 | PORT_PCR_MUX(0x03)); // UART_TX
    PORTE->PCR[9] = (0 | PORT_PCR_MUX(0x03)); // UART_RX

    // Buttons SW2, SW3, SW4(lab.2)
    PORTE->PCR[10] = PORTE->PCR[12] = PORTE->PCR[27] = ( PORT_PCR_ISF(0x01)     // Interrupt Status Flag)
	 			    | PORT_PCR_IRQC(0x0A)                                       // Interrupt enable on failing edge
	 			    | PORT_PCR_MUX(0x01)                                        // Pin Mux Control to GPIO
	 			    | PORT_PCR_PE(0x01)                                         // Pull resistor enable...
	 			    | PORT_PCR_PS(0x01));                                       // ...select Pull-Up

    // Needed for buttons handling
	NVIC_ClearPendingIRQ(PORTE_IRQn); 
	NVIC_EnableIRQ(PORTE_IRQn);
    
    PTA->PDDR = GPIO_PDDR_PDD(0x10); // Beeper
    PTB->PDDR = GPIO_PDDR_PDD(0x3C);   // Lights
    PTB->PDOR |= GPIO_PDOR_PDO(0x3C);  // turn all LEDs OFF for sure
}

// Initialize UART (1. lab)
void UART5Init() {
    UART5->C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);

    UART5->BDH = 0x00;
    UART5->BDL = 0x1A;  // Baud rate 115 200 Bd, 1 stop bit
    UART5->C4 = 0x0F;   // Oversampling ratio 16, match address mode disabled

    UART5->C1 = 0x00;   // 8 data bite, no parity
    UART5->C3 = 0x00;
    UART5->MA1 = 0x00;  // no match address (mode disabled in C4)
    UART5->MA2 = 0x00;  // no match address (mode disabled in C4)

    UART5->S2 |= 0xC0;

    UART5->C2 |= (UART_C2_TE_MASK | UART_C2_RE_MASK); // Turn on sender and receiver
}

// Initialize RTC (fitkit 3 sdk examples)
void RTCInit() {
    // Reset registers
    RTC_CR |= RTC_CR_SWR_MASK;
    RTC_CR &= ~RTC_CR_SWR_MASK;
    RTC_TCR = 0x00;

    // Enable oscilator
    RTC_CR |= RTC_CR_OSCE_MASK;

    // Turn off RTC time counter
    RTC_SR &= ~RTC_SR_TCE_MASK;

    // Set TSR register
    RTC_TSR = 0xFFFFFFFF;

    // Set TAR register to max value
    RTC_TAR = 0xFFFFFFFE;

    // Enable interrupt
    RTC_IER |= RTC_IER_TAIE_MASK;

    // Needed for alarm handling
    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);
}

/********************************************* UART COMMUNICATION HELPERS ******************************************/

// Send 1 character (1.lab)
void SendCh(char ch) {
    while (!(UART5->S1 & UART_S1_TDRE_MASK) && !(UART5->S1 & UART_S1_TC_MASK)) {
        ;
    }
    UART5->D = ch;
}

// Receive 1 character (1.lab)
char ReceiveCh() {
    while (!(UART5->S1 & UART_S1_RDRF_MASK)) {
        ;
    }
    return UART5->D;
}

// Send string (1.lab)
void SendStr(const char s[]) {
    int i = 0;
    while (s[i] != 0) {
        SendCh(s[i++]);
    }
}

// Get time string in format HH:mm:ss, string parameter is prompt
unsigned long GetTime(const char prompt[]) {
    SendStr(prompt);

    char dayTime[8];
    char ch;

    /************************** Hours ***************************/
    while (ch = ReceiveCh()) {
        if (ch >= '0' && ch <= '2') {
            dayTime[0] = ch;
            SendCh(ch);
            break;
        }
    }

    while (ch = ReceiveCh()) {
        if (ch >= '0' && ch <='4') {
            dayTime[1] = ch;
            SendCh(ch);
            SendCh(':');
            break;
        }
    }

    /************************** Minutes ***************************/
    while (ch = ReceiveCh()) {
        if (ch >= '0' && ch <= '5') {
            dayTime[2] = ch;
            SendCh(ch);
            break;
        }
    }

    while (ch = ReceiveCh()) {
        if (isdigit(ch)) {
            dayTime[3] = ch;
            SendCh(ch);
            SendCh(':');
            break;
        }
    }

    /************************** Seconds ***************************/
    while (ch = ReceiveCh()) {
        if (ch >= '0' && ch <= '5') {
            dayTime[4] = ch;
            SendCh(ch);
            break;
        }
    }

    while (ch = ReceiveCh()) {
        if (isdigit(ch)) {
            dayTime[5] = ch;
            SendCh(ch);
            break;
        }
    }

    // Convert
    int hour = (((dayTime[0] - '0') * 10) + (dayTime[1] - '0')) * 3600;
    int minute = (((dayTime[2] - '0') * 10) + (dayTime[3] - '0')) * 60;
    int second = ((dayTime[4] - '0') * 10) + (dayTime[5] - '0');

    // Return seconds
    return hour + minute + second;
}

// Receive number(integer)
int GetNumber(const char prompt[]) {
    SendStr(prompt);
    char ch;
    int number = 0;

    while (ch = ReceiveCh()) {
        if (ch >= '0' && ch <='9') {
            number = (number * 10) + (ch - '0');
            SendCh(ch);
        }
        else {
            break;
        }
    }

    return number;
}

// Receive melodyType or light notification type, string parameter is prompt
int GetType(const char prompt[]) {
    SendStr(prompt);
    char ch;

    while (ch = ReceiveCh()) {
        if (ch >= '1' && ch <='3') {
            SendCh(ch);
            return ch - '0';
        }
    }
}

// Convert time in second to string (HH:mm:ss format)
void ConvertTime(char* buffer, unsigned long time) {
    time %= MAX_DAY_TIME;
    int hours = time / 3600;
    time %= 3600;
    int minutes = time / 60;
    time %= 60;

    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, time);
}

/********************************************* Interrupt Handlers ******************************************/

// RTC interrupt handler
void RTC_IRQHandler() {
    if(RTC_SR & RTC_SR_TAF_MASK) {
        // Check for repetition
        if (remainingRepeat > 0) {
            remainingRepeat--;
            (RTC->TAR) += repeatDelay;
        }
        else {
            // Reset alarm to next day
            remainingRepeat = repeatNumber;
            alarmTime += MAX_DAY_TIME;
            RTC->TAR = alarmTime;
        }

        if (alarmOn) {
            SendStr(alarmStr);
            TurnOnLights();
            PlayMelody();
        }
    }
}

// Button handler (2.lab)
void PORTE_IRQHandler() {
    // Check if buttons are enabled
    if (buttonsEnabled == 0)
        return;

    // Wait (for ray filtering)
	Delay(10000);
    
    // Turn on/off alarm
	if (((BTN_SW2 & PORTE->ISFR)) && ((BTN_SW2 & PTE->PDIR) == 0)) {
        // Check if on
    	if (alarmOn == 1) {
            alarmOn = 0;
            SendStr(alarmOffStr);
        }
        else {
            alarmOn = 1;
            SendStr(alarmOnStr);
        }
    	PORTE->ISFR = BTN_SW2 | PORTE->ISFR;
    	return;
    }
    // Print time
    if (((BTN_SW3 & PORTE->ISFR)) && ((BTN_SW3 & PTE->PDIR) == 0)) {
        SendStr(currentTimeStr);
        char buffer[9];
        ConvertTime(buffer, RTC->TSR);
    	SendStr(buffer);
    	PORTE->ISFR = BTN_SW3 | PORTE->ISFR;
    	return;
    }
    // New initialization
    if (((BTN_SW4 & PORTE->ISFR)) && ((BTN_SW4 & PTE->PDIR) == 0)) {
    	alarmOn = 0;
        AppInit();
    	PORTE->ISFR = BTN_SW4 | PORTE->ISFR;
    	return;
    }
}

/********************************************* Sound notification ******************************************/

// Modified Beeper(lab.1) for Fitkit3, playing notes
void Beep(int note, int duration, int delay) {
    int time = ((duration * 100) / (note * 2));  // We need to evaluate time from note and duration

    for (int i = 0; i < time; i++) {
    	PTA->PDOR = GPIO_PDOR_PDO(0x0010);
        Delay(note);
        PTA->PDOR = GPIO_PDOR_PDO(0x0000);
        Delay(note);
    }

    if (delay == 1) {
        Delay(600000);     // Additional long delay for note separation
    }
    else if (delay == 0) {
        Delay(100000);     // Additional short delay for note separation
    }
}

// Play chosen melodyType
void PlayMelody() {
    switch(melodyType) {
        case 1:
            PlayBohemian();
            break;
        case 2:
            PlaySmoke();
            break;
        case 3:
            PlayIronman();
    }
}

// Play bohemian rhapsody melodyType
void PlayBohemian() {
    for (int i = 0; i < 2; i++) {
        Beep(Ais_4, 2500, 1);
        Beep(F_4, 2500, 1);
        Beep(Ais_4, 2500, 1);
        Beep(D_5, 2500, 1);
        Beep(G_6, 9000, 1);
        Beep(F_6, 9000, 1);
    }
}

// Play smoke on the water melodyType
void PlaySmoke() {
    Beep(D_4, 3500, 1);
    Beep(F_4, 3500, 1);
    Beep(G_4, 9000, 1);

    Beep(D_4, 3500, 1);
    Beep(F_4, 3500, 1);
    Beep(Gis_4, 3500, 0);
    Beep(G_4, 9000, 1);

    Beep(D_4, 3500, 1);
    Beep(F_4, 3500, 1);
    Beep(G_4, 9000, 1);

    Beep(F_4, 3500, 1);
    Beep(D_4, 9000, 1);
}

// Play Ironman melodyType
void PlayIronman() {
    Beep(A_3, 7500, 1);
    Beep(C_4, 10500, 1);
    Beep(C_4, 7500, 1);
    Beep(D_4, 5500, 1);
    Beep(D_4, 5500, 1);

    Beep(F_4, 3500, 0);
    Beep(E_4, 3500, 0);
    Beep(F_4, 3500, 0);
    Beep(E_4, 3500, 0);
    Beep(F_4, 3500, 0);
    Beep(E_4, 3500, 0);

    Beep(C_4, 7500, 1);

    Beep(C_4, 7500, 1);
    Beep(D_4, 7500, 1);
    Beep(D_4, 7500, 1);
}

/********************************************* Light notification ******************************************/

// Start chosen lights schema
void TurnOnLights() {
    switch(lightsType) {
        case 1:
            LightAll();
            break;
        case 2:
            LightCradle();
            break;
        case 3:
            LightJump();
    }
}

// All lights blinking
void LightAll() {
    for (int i = 0; i < 4; i++) {
        // 0x3C - all LEDs
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x3C);  // Turn on
        Delay(500000);

        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);   // Turn off
        Delay(500000);
    }
}

// Left to right and back
void LightCradle() {
    for (int i = 0; i < 4; i++) {
        // Left to right
        GPIOB_PDOR ^= LED_D12;              // Activate this LED
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);  // Turn on
        Delay(150000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);   // Turn off

        GPIOB_PDOR ^= LED_D11;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(150000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        GPIOB_PDOR ^= LED_D10;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(150000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        GPIOB_PDOR ^= LED_D9;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(150000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        // Right to left
        GPIOB_PDOR ^= LED_D9;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(150000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        GPIOB_PDOR ^= LED_D10;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(150000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        GPIOB_PDOR ^= LED_D11;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(150000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        GPIOB_PDOR ^= LED_D12;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(150000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);
    }
}

// 1, 3, 2, 4
void LightJump() {
    for (int i = 0; i < 4; i++) {
        GPIOB_PDOR ^= LED_D12;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(600000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        GPIOB_PDOR ^= LED_D10;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(600000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        GPIOB_PDOR ^= LED_D11;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(600000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        GPIOB_PDOR ^= LED_D9;
        PTB->PDOR &= ~GPIO_PDOR_PDO(0x01);
        Delay(600000);
        PTB->PDOR |= GPIO_PDOR_PDO(0x3C);
    }
}

void AppInit() {
    // Disable buttons
    buttonsEnabled = 0;

    // Disable alarm
    alarmOn = 0;

    // Ask user for current time
    unsigned long dayTime = GetTime(timePrompt);

    // Turn on RTC timer
    EnableTimer(dayTime);

    // Ask user for alarm time
    alarmTime = GetTime(alarmPrompt);

    if (alarmTime <= dayTime) {
        alarmTime += MAX_DAY_TIME;  // Tomorrow
    }

    // Ask user for repetition number
    repeatNumber = GetNumber(repeatPrompt);

    if (repeatNumber > 30) {
        SendStr(defaultValue);
        repeatNumber = 0;
    }
    else {
        remainingRepeat = repeatNumber;
    }

    // Ask user for repetition delay if needed
    if (repeatNumber > 0) {
        repeatDelay = GetNumber(delayPrompt);

        if (repeatDelay > 3600 || repeatDelay < 15) {
            SendStr(defaultValue);
            repeatDelay = 60;
        }
    }

    // Ask user for sound notification type
    melodyType = GetType(melodyPrompt);

    // Ask user for light notification type
    lightsType = GetType(lightPrompt);

    // Set TAR register (alarm)
    RTC->TAR = alarmTime;

    // Enable buttons
    buttonsEnabled = 1;

    // Enable alarm
    alarmOn = 1;

    SendStr(success);
    SendStr(currentTimeStr);

    char buffer[9];
    ConvertTime(buffer, RTC->TSR);
    SendStr(buffer);

    SendStr(alarmTimeStr);
    ConvertTime(buffer, RTC->TAR);
    SendStr(buffer);
}

// Set and enable RTC module
void EnableTimer(int daySeconds) {
    RTC->SR &= ~RTC_SR_TCE_MASK; // RTC Time counter disabled (needed for TSR setting)
    RTC->TSR = daySeconds;       // Set time second register
    RTC->SR |= RTC_SR_TCE_MASK;  // Time counter enabled
}

// Main method
int main()
{
    // Initializations
    MCUInit();
    PortsInit();
    UART5Init();
    RTCInit();
    
    AppInit();

    while (1)
    {
        // App running forever
    }
}
