/**
 * @file main.c
 *
 * @brief Main source code for the BLE_UART program.
 *
 * This file contains the main entry point for the BLE_UART program,
 * which is used to demonstrate the BLE_UART driver.
 *
 * It interfaces with the Adafruit Bluefruit LE UART Friend Bluetooth Low Energy (BLE) module, which uses the UART communication protocol.
 *  - Product Link: https://www.adafruit.com/product/2479
 *
 * @note For more information regarding the Enhanced Universal Serial Communication Interface (eUSCI),
 * refer to the MSP432Pxx Microcontrollers Technical Reference Manual
 *
 * @author Carlos Bautista
 *
 */

#include <stdint.h>
#include <math.h>
#include "msp.h"

#include "inc/Clock.h"
#include "inc/CortexM.h"
#include "inc/GPIO.h"
#include "inc/EUSCI_A0_UART.h"
#include "inc/Motor.h"
#include "inc/BLE_UART.h"

#define PWM_NOMINAL 3000
#define PWM_CONTROLLER_30 4500

//  Processes commands received from the BLE UART module.
//  This function checks the received data for commands that
//  turn the onboard RGB LED green or off.
//
void Process_BLE_UART_Data(char BLE_UART_Buffer[])
{
    if (Check_BLE_UART_Data(BLE_UART_Buffer, "RGB LED GREEN"))
    {
        LED2_Output(RGB_LED_GREEN);
    }
    else if (Check_BLE_UART_Data(BLE_UART_Buffer, "RGB LED OFF"))
    {
        LED2_Output(RGB_LED_OFF);
    }
    else
    {
        printf("BLE UART Command Not Found\n");
    }
}

//  This function is used for Task 6.1A to transmit
//  hexadecimal values 0xAA, 0x55, 0xFF, and 0x00
//  through P9.7 (UART TX).

void UART_Transmit_Test(void)
{
//    BLE_UART_OutChar(0xAA);
//    Clock_Delay1ms(500);
//
//    BLE_UART_OutChar(0x55);
//    Clock_Delay1ms(500);
//
//    BLE_UART_OutChar(0xFF);
//    Clock_Delay1ms(500);

    BLE_UART_OutChar(0x00);
    Clock_Delay1ms(500);
}

// Initializes the PMOD SWT switches as GPIO inputs.
//
//  This function configures P10.0 through P10.3 as digital
//  inputs with internal pull-down resistors.

void UART_Switch_Init(void)
{
    PMOD_SWT_Init();

    P10->SEL0 &= ~0x0F;
    P10->SEL1 &= ~0x0F;
    P10->DIR &= ~0x0F;

    P10->REN |= 0x0F;
    P10->OUT &= ~0x0F;
}

//  Transmits UART data based on the active PMOD switch.
//
//  This function reads the four PMOD switches and transmits
//  a corresponding UART byte:
//  SW1 = 0xAA, SW2 = 0x10, SW3 = 0xF0, SW4 = 0x0F.
//  If no switches or multiple switches are active,
//  the function transmits 0x00.

void UART_Switch_Transmit_Test(void)
{
    uint8_t switches = P10->IN & 0x0F;
    uint8_t data;

    switch (switches)
    {
        case 0x01:
            data = 0xAA;
            break;

        case 0x02:
            data = 0x10;
            break;

        case 0x04:
            data = 0xF0;
            break;

        case 0x08:
            data = 0x0F;
            break;

        default:
            data = 0x00;
            break;
    }

    BLE_UART_OutChar(data);
    Clock_Delay1ms(500);
}

//Processes Bluetooth Controller packets for robot movement.
//
//  This function decodes directional button commands received
//  from the Bluefruit Connect application.
//
//  Buttons 5, 6, 7, and 8 control forward, backward,
//  left, and right movement at 30% PWM duty cycle.
//  Releasing a directional button stops the motors.

void Decode_Controller_Packet(char BLE_UART_Buffer[])
{
    char button;
    char state;

    if (BLE_UART_Buffer[0] != '!' ||
        BLE_UART_Buffer[1] != 'B')
    {
        return;
    }

    button = BLE_UART_Buffer[2];
    state = BLE_UART_Buffer[3];

    if (button != '5' && button != '6' &&
        button != '7' && button != '8')
    {
        return;
    }

    if (state == '0')
    {
        Motor_Stop();
        return;
    }

    if (state != '1')
    {
        return;
    }

    switch (button)
    {
        case '5':
            Motor_Forward(PWM_CONTROLLER_30, PWM_CONTROLLER_30);
            break;

        case '6':
            Motor_Backward(PWM_CONTROLLER_30, PWM_CONTROLLER_30);
            break;

        case '7':
            Motor_Left(PWM_CONTROLLER_30, PWM_CONTROLLER_30);
            break;

        case '8':
            Motor_Right(PWM_CONTROLLER_30, PWM_CONTROLLER_30);
            break;

        default:
            Motor_Stop();
            break;
    }
}

/**
    Initializes the MSP432 and processes incoming Bluetooth commands.
    This function does not return during normal operation.
 */
int main(void)
{
    char BLE_UART_Buffer[BLE_UART_BUFFER_SIZE] = {0};

    DisableInterrupts();

    Clock_Init48MHz();

    LED2_Init();

    EUSCI_A0_UART_Init_Printf();

    BLE_UART_Init();

    Motor_Init();

    Motor_Stop();

    EnableInterrupts();

    Clock_Delay1ms(1000);

    BLE_UART_Reset();

    BLE_UART_OutString("BLE UART Active\r\n");

    Clock_Delay1ms(1000);

    while(1)
    {
//        int string_size = BLE_UART_InString(
//            BLE_UART_Buffer,
//            BLE_UART_BUFFER_SIZE
//        );
//
//        int i;
//
//        printf("BLE UART Data: ");
//
//        for (i = 0; i < string_size; i++)
//        {
//            printf("%c", BLE_UART_Buffer[i]);
//        }
//
//        printf("\n");
//
//        Process_BLE_UART_Data(BLE_UART_Buffer);
//        UART_Switch_Transmit_Test();
            BLE_UART_InPacket(BLE_UART_Buffer);
            Decode_Controller_Packet(BLE_UART_Buffer);
    }
}
