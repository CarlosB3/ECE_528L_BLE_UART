/**
 * @file BLE_UART.c
 *
 * @brief Source code for the BLE_UART driver.
 *
 * This file contains the function definitions for the BLE_UART driver.
 *
 * It interfaces with the Adafruit Bluefruit LE UART Friend Bluetooth Low Energy (BLE) module, which uses the UART communication protocol.
 *  - Product Link: https://www.adafruit.com/product/2479
 *
 * The following connections must be made:
 *  - BLE UART MOD  (Pin 1)     <-->  MSP432 LaunchPad Pin P1.6
 *  - BLE UART CTS  (Pin 2)     <-->  MSP432 LaunchPad GND
 *  - BLE UART TXO  (Pin 3)     <-->  MSP432 LaunchPad Pin P9.6 (PM_UCA3RXD)
 *  - BLE UART RXI  (Pin 4)     <-->  MSP432 LaunchPad Pin P9.7 (PM_UCA3TXD)
 *  - BLE UART VIN  (Pin 5)     <-->  MSP432 LaunchPad VCC (3.3V)
 *  - BLE UART RTS  (Pin 6)     <-->  Not Connected
 *  - BLE UART GND  (Pin 7)     <-->  MSP432 LaunchPad GND
 *  - BLE UART DFU  (Pin 8)     <-->  Not Connected
 *
 * @note For more information regarding the Enhanced Universal Serial Communication Interface (eUSCI),
 * refer to the MSP432Pxx Microcontrollers Technical Reference Manual
 *
 * @author Carlos Bautista
 *
 */

#include "../inc/BLE_UART.h"

// Initializes the BLE UART communication interface.
//
//  This function configures the eUSCI_A3 module for UART
//  communication at 9600 baud. It configures P9.6 as RX,
//  P9.7 as TX, and P1.6 for the BLE module's operating mode.
//
//  The UART uses 8 data bits, no parity, and 1 stop bit.

void BLE_UART_Init()
{
        P9->SEL0 |= 0xC0;
        P9->SEL1 &= ~0xC0;

        P1->SEL0 &= ~0x40;
        P1->SEL1 &= ~0x40;
        P1->DIR |= 0x40;

        EUSCI_A3->CTLW0 |= 0x01;
        EUSCI_A3->MCTLW = 0x0000;

        EUSCI_A3->CTLW0 &= ~0x8000;
        EUSCI_A3->CTLW0 &= ~0x4000;
        EUSCI_A3->CTLW0 &= ~0x2000;
        EUSCI_A3->CTLW0 &= ~0x1000;
        EUSCI_A3->CTLW0 &= ~0x0800;
        EUSCI_A3->CTLW0 &= ~0x0600;
        EUSCI_A3->CTLW0 &= ~0x0100;
        EUSCI_A3->CTLW0 |= 0x00C0;

        EUSCI_A3->BRW = 1250;

        EUSCI_A3->IE &= ~0x0C;
        EUSCI_A3->IE |= 0x03;

        EUSCI_A3->CTLW0 &= ~0x01;
}

// Receives a single character through UART.
//
// This function waits until the UART receive flag is set,
// then reads the received character from the RXBUF register.

uint8_t BLE_UART_InChar()
{
    while((EUSCI_A3->IFG & 0x01) == 0);

    return EUSCI_A3->RXBUF;
}

//Transmits a single character through UART.
//
//  This function waits until the UART transmit buffer is ready,
//  then writes the specified character to the TXBUF register.

void BLE_UART_OutChar(uint8_t data)
{
    while((EUSCI_A3->IFG & 0x02) == 0);

    EUSCI_A3->TXBUF = data;
}

//Receives a string of characters through UART.
//
//  This function receives characters until a line feed (LF)
//  character is detected. It supports backspace processing
//  and stores the received characters in the provided buffer.
//
//  A null terminator is added at the end of the string.

int BLE_UART_InString(char *buffer_pointer, uint16_t buffer_size)
{
    int length = 0;
       int string_size = 0;

       if (buffer_size == 0)
       {
           return 0;
       }

       char character = BLE_UART_InChar();

       while (character != LF)
       {
           if (character == BS)
           {
               if (length)
               {
                   buffer_pointer--;
                   length--;
                   BLE_UART_OutChar(BS);
               }
           }
           else if (length < buffer_size - 1)
           {
               *buffer_pointer = character;
               buffer_pointer++;
               length++;
               string_size++;
           }

           character = BLE_UART_InChar();
       }

       *buffer_pointer = 0;

       return string_size;
}

//  Transmits a string of characters through UART.
//
//  This function transmits each character in the provided
//  string using BLE_UART_OutChar() until the null terminator
//  is reached.

void BLE_UART_OutString(char *pt)
{
    while (*pt)
     {
         BLE_UART_OutChar(*pt);
         pt++;
     }
}

//  Checks for a specified string in the received BLE data.
//
//  This function uses strstr() to search the received Bluetooth
//  data buffer for a matching command string.

uint8_t Check_BLE_UART_Data(char BLE_UART_Data_Buffer[], char *data_string)
{
    if (strstr(BLE_UART_Data_Buffer, data_string) != NULL)
    {
        return 0x01;
    }
    else
    {
        return 0x00;
    }
}

//Resets the Adafruit Bluefruit LE UART module.
//
//  This function sets P1.6 HIGH to enter Command Mode,
//  waits for the mode change, and transmits the ATZ reset
//  command through UART.
//
//  After allowing the module to reset, P1.6 is cleared
//  to return the module to Data Mode.

void BLE_UART_Reset()
{
    P1->OUT |= 0x40;
    Clock_Delay1ms(1000);

    BLE_UART_OutString("ATZ\r\n");
    Clock_Delay1ms(3000);

    P1->OUT &= ~0x40;
}

//  Receives a five-byte Bluefruit Controller packet.
//
//  This function waits for the packet header character ('!')
//  and receives the remaining four bytes through UART.
//
//  The packet contains the header, command type, button number,
//  button state, and checksum.
//
//  A null terminator is added after the received packet.
//  The function does not validate the checksum.

void BLE_UART_InPacket(char BLE_UART_Buffer[])
{
    uint8_t character;
    uint8_t i;

    do
    {
        character = BLE_UART_InChar();
    }
    while (character != '!');

    BLE_UART_Buffer[0] = '!';

    for (i = 1; i < 5; i++)
    {
        BLE_UART_Buffer[i] = BLE_UART_InChar();
    }

    BLE_UART_Buffer[5] = '\0';
}
