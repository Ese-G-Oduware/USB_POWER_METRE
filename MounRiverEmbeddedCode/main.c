/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/25
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *Multiprocessor communication mode routine:
 *Master:USART1_Tx(PD5)\USART1_Rx(PD6).
 *This routine demonstrates that USART1 receives the data sent by CH341 and inverts
 *it and sends it (baud rate 115200).
 *
 *Hardware connection:PD5 -- Rx
 *                     PD6 -- Tx
 *
 */

#include "debug.h"
#include<stdio.h>


/* Global define */
#define OLED_ADDR 0x3c //according to datasheet
//Since A0 and A1 are Grounded then from data sheet
#define INA_ADDR 0x40

/* Global Variable */

/*********************************************************************
 * @fn      USARTx_CFG
 *
 * @brief   Initializes the USART2 & USART3 peripheral.
 *
 * @return  none
 */
void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    /* USART1 TX-->D.5   RX-->D.6 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void IIC_Init(){
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    //Set Clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    //Initialiaze GPIO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;//SDA -PC1
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;//SCL -PC2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    //Initialize I2C
    I2C_InitStructure.I2C_ClockSpeed = 400000; //400 kHz Fast mode
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);

}

//Send a command to the SSD1306 (Control Byte is 0x00 for commands)
void I2C_WriteCommand(uint8_t cmd){
    //Check that I2C communication isnt doing smth
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);
    //Send START signal
    I2C_GenerateSTART(I2C1, ENABLE);
    //Confirm Start Signal and that Slave/Follower is connected
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    //Send to Follower address (NOTE: address in I2C communication is 7 bits, so we have to shift bits)
    I2C_Send7bitAddress(I2C1, OLED_ADDR << 1, I2C_Direction_Transmitter);
    //ACKnowledge that address has been selected
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    //Check if Transmit data register is empty
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET);
    //0x00 indicates that the next byte is a Command
    I2C_SendData(I2C1, 0x00);
    
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET);
    I2C_SendData(I2C1, cmd);

    //Check is data is sent, and STOP signal generated.
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    
    I2C_GenerateSTOP(I2C1, ENABLE);
}

//Send a display byte to SSD1306 RAM (Contorl byte is 0x40 for sending data)
void I2C_WriteData(uint8_t data){
    //I2C PROTOCOL
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, OLED_ADDR << 1, I2C_Direction_Transmitter);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET);
    I2C_SendData(I2C1, 0x40); 

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET);
    I2C_SendData(I2C1, data); //Indicates following byte is Display Data

    I2C_GenerateSTOP(I2C1, ENABLE);
}   

/*
6x8 hexadecimal table for the numbers, decimals, 'V', 'A', and spaces
Each hexadecimal number represents a coloumn of pixels one byte long, 
with 0 being off and 1 being on.
*/
const uint8_t CHAR6x8[][6] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //0: Space
    {0x00, 0x00, 0x5F, 0x00, 0x00, 0x00}, // 1: Period '.'
    {0x00, 0x00, 0x07, 0x50, 0x00, 0x00}, // 3: Colon ':'
    {0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E}, // 4: '0'
    {0x00, 0x00, 0x42, 0x7F, 0x40, 0x00}, // 5: '1'
    {0x00, 0x42, 0x61, 0x51, 0x49, 0x46}, // 6: '2'
    {0x00, 0x21, 0x41, 0x45, 0x4B, 0x31}, // 7: '3'
    {0x00, 0x18, 0x14, 0x12, 0x7F, 0x10}, // 8: '4'
    {0x00, 0x27, 0x45, 0x45, 0x45, 0x39}, // 9: '5'
    {0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30}, // 10: '6'
    {0x00, 0x01, 0x71, 0x09, 0x05, 0x03}, // 11: '7'
    {0x00, 0x36, 0x49, 0x49, 0x49, 0x36}, // 12: '8'
    {0x00, 0x06, 0x49, 0x49, 0x29, 0x1E}, // 13: '9'
    {0x00, 0x7E, 0x11, 0x11, 0x11, 0x7E}, // 14: 'A'
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x06},  // 15: 'V'
    {0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 15: 'W'
    {0x00, 0x7F, 0x09, 0x19, 0x29, 0x46}, // 16: 'P'
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x22}  // 17: 'G'
};

//CLear Screen (Datasheet requires this function), to get rid of pixel noise
void OLED_Clear(){
    for(uint8_t i = 0; i < 8; i++){ // loop through all 8 vertical rows
        I2C_WriteCommand(0xB0 + i); //0xB0 is the base code for Page 0, so 0xB2 is to move to the 2nd vertical row
        //The SSD1306 chip cant take in full 7 bit numbers, so we split them into lower and upper parts
        I2C_WriteCommand(0x00); //Lower column start address
        I2C_WriteCommand(0x10); //Higher coloumn start adress

        for(uint8_t j = 0; j < 128; j++){
            I2C_WriteData(0x00); //Send blank pixel data to all 128 columns of one of the 8 vertical rows
        }
    }
}

void  OLED_SetCursor(uint8_t page, uint8_t column){
    //Set the cusor position (page 0-7, column 0 till 127 )
    //Note: The SSD1306 chip reads the first four bits and last for bits seperately for the commands
    I2C_WriteCommand(0xB0 + page); // page to draw on
    I2C_WriteCommand(0x00 + (column & 0x0F));//the lower address bits of a particular column on the OLED
    I2C_WriteCommand(0x10 + ((column >> 4) & 0x0F));//the upper address bits of a column
    //Note: we shift the column bit right by 4 so that when we do the & operation and then add to the base command, the SSD chip can effectively read what we want
}

//Print a single character on the screen
void OLED_PrintChar(uint8_t page, uint8_t column, char chr){// a page is an 8 bit vertical row
    uint8_t index = 0;
    if (chr == ' ') index = 0;
    else if (chr == '.') index = 1;
    else if (chr == ':') index = 2;
    else if (chr >= '0' && chr <= '9') index = 3 + (chr - '0');
    else if (chr == 'A') index = 13;
    else if (chr == 'V') index = 14;
    else if (chr == 'W') index = 15;
    else if (chr == 'P') index = 16;
    else if (chr == 'G') index = 17;

    OLED_SetCursor(page, column);

    for(uint8_t i = 0; i < 6; i++){ //Writing data to OLED buffer (which will then show on screen)
        I2C_WriteData(CHAR6x8[index][i]);
    }

}

// Renders the rolling power graph across Pages 4 through 7 (bottom 32 pixels)
void OLED_DrawGraph(uint8_t *history)
{
    for(uint8_t page = 4; page < 8; page++)
    {
        OLED_SetCursor(page, 0);
        for(uint8_t x = 0; x < 128; x++)
        {
            uint8_t p_byte = 0x00;
            uint8_t height = history[x]; // 0 to 32 pixels
            int8_t base_pixel = (7 - page) * 8;
            
            for(int bit = 0; bit < 8; bit++)
            {
                if(height > (base_pixel + bit))
                {
                    p_byte |= (1 << bit);
                }
            }
            I2C_WriteData(p_byte);
        }
    }
}

void OLED_PrintString(uint8_t page, uint8_t column, char *str){
    //loop through entire string character by character
    while(*str){
        OLED_PrintChar(page, column, *str);
        column += 6;// move by 6 pixels to the next character (since characters are 6 pixels wide and 8 pixels tall)
        str++; //move pointer to the next character
    }
}

// Standard SSD1306 OLED Initialization Sequence (Datasheet Order)
void OLED_Init(void)
{
    Delay_Ms(100); // Power stabilization delay for 4-pin modules

    I2C_WriteCommand(0xAE); // Turn off OLED panel (Display OFF)
    I2C_WriteCommand(0x00); // Set low column address
    I2C_WriteCommand(0x10); // Set high column address
    I2C_WriteCommand(0x40); // Set start line address
    I2C_WriteCommand(0x81); // Set contrast control register
    I2C_WriteCommand(0xCF); // Set SEG Output Current Brightness
    I2C_WriteCommand(0xA1); // Set SEG/Column Mapping (0xA1 = normal, 0xA0 = mirrored)
    I2C_WriteCommand(0xC8); // Set COM/Row Scan Direction (0xC8 = normal, 0xC0 = flipped)
    I2C_WriteCommand(0xA6); // Set normal display (non-inverted)
    I2C_WriteCommand(0xA8); // Set multiplex ratio (1 to 64)
    I2C_WriteCommand(0x3F); // 1/64 duty
    I2C_WriteCommand(0xD3); // Set display offset shift mapping RAM counter
    I2C_WriteCommand(0x00); // Not offset
    I2C_WriteCommand(0xD5); // Set display clock divide ratio / oscillator frequency
    I2C_WriteCommand(0x80); // Set divide ratio
    I2C_WriteCommand(0xD9); // Set pre-charge period
    I2C_WriteCommand(0xF1); // Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    I2C_WriteCommand(0xDA); // Set com pins hardware configuration
    I2C_WriteCommand(0x12);
    I2C_WriteCommand(0xDB); // Set vcomh
    I2C_WriteCommand(0x30); // Set VCOM Deselect Level
    I2C_WriteCommand(0x20); // Set Memory Addressing Mode
    I2C_WriteCommand(0x02); // 0x02 = Page Addressing Mode (matching datasheet)
    I2C_WriteCommand(0x8D); // Set Charge Pump enable/disable
    I2C_WriteCommand(0x14); // Enable charge pump

    // Clear the display buffer while the panel is still OFF (prevents static flash)
    OLED_Clear();

    I2C_WriteCommand(0xAF); // Turn OLED panel ON
}

/*INA219 I2C COMMUNICATION*/
///Writing to INA219 according to datasheet
void INA219_Write(uint8_t reg, uint16_t value){
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, INA_ADDR << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(I2C1, reg);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, (uint8_t)(value >> 8));//Send High byte
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, (uint8_t)(value & 0xFF));//Send Low byte
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}
//INA219 Initialization
void INA219_Init(){
    //Configure the INA 
    INA219_Write(0x00, 0x399F); //0x399F is from data sheet
    //Set Calibration Register (Calculated for a 0.01 ohm shunt resistor supporting up to 3.2A)
    INA219_Write(0x05, 0xA3D7);//this value should change after getting initial readings to match MCU output with multimeter readings

}
//INA129 Readind Data
uint16_t INA219_ReadReg(uint8_t reg){
    uint16_t value = 0;
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    //Write data to replace register pointer
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, INA_ADDR << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(I2C1, reg);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    //According to Datasheet, generate start signal again
    //The read data
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, INA_ADDR << 1, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    value = (uint16_t)(I2C_ReceiveData(I2C1) << 8);//MSB (Highside byte) is transmitted first

    I2C_AcknowledgeConfig(I2C1, DISABLE);//NACK
    I2C_GenerateSTOP(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    value |= I2C_ReceiveData(I2C1); //adds bits (low side and highside) to form 16 bit register value
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    return value;

}

float ReadVolts(){
    //Read data from INA219
    uint16_t rawBusVolt = INA219_ReadReg(0x02);
    return (rawBusVolt >> 3) *4.0 /1000.0; //The Datasheet says to do this
}

float ReadAmps(){
    //Read data from INA219
    int16_t rawCurrent = (int16_t)INA219_ReadReg(0x04);

    //No need to calculate current manually as the chip does it for me
    //Current LSB is 3.2A/ 2^15 bits = 0.00009765625, assuming 3.2A is max current
    return (float)rawCurrent * 0.00009765625; //The Datasheet says to do this
}

//Initialize Buttons
void Button_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}
/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    USARTx_CFG();

    //Project Name
    printf("OLED DISPLAY(HS96L03W2C0) I2C communication\n");

    //Initialiaze I2C Communication Protocol
    IIC_Init();
    OLED_Init();
    INA219_Init();
    Button_Init();

    uint8_t current_page = 0; // 0 = V/I/W Stats Page, 1 = Live Power Graph Page

    // Rolling power history buffer for 128 vertical pixel columns
    uint8_t power_history[128] = {0};

    char OLEDString[32]; //buffer for the shown strings

    while(1)
    {   

        if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2) == Bit_RESET){
            current_page++;
            if (current_page > 1) current_page = 0; // Toggle between Page 0 and 1
            OLED_Clear(); // Wipe screen clean when switching views
            Delay_Ms(250); 
        }
        //Voltage and current values 
        float volt = ReadVolts();
        float current = ReadAmps();
        float power = volt * current;

        // Shift old readings left
        for(int i = 0; i < 127; i++){
            power_history[i] = power_history[i+1];
        }

        // Scale power reading (assume max expected power = 15 Watts for scaling to 32 pixels)
        uint8_t scaled_height = (uint8_t)((power / 15.0f) * 32.0f);
        if(scaled_height > 32) scaled_height = 32;
        power_history[127] = scaled_height;
        
       if(current_page == 0){ 
            //The funny whole number and decimal calculations are because the CH32V003 does not lunk float formatting to flash memory.
            //Basically it will print the string as is, and not adjust to the formatting.
            //Voltage
            int v_whole = (int)volt;
            int v_dec = (int)((volt - v_whole) * 100);
            sprintf(OLEDString, "V: %d.%02dV", v_whole, v_dec);
            OLED_PrintString(0, 0, OLEDString);
            
            //Current
            int i_whole = (int)current;
            int i_dec = (int)((current - i_whole) * 100);
            sprintf(OLEDString, "I: %d.%02dA", i_whole, i_dec);
            OLED_PrintString(2, 0, OLEDString);
            
            //Power
            int p_whole = (int)power;
            int p_dec = (int)((power - p_whole) * 100);
            sprintf(OLEDString, "P: %d.%02dW", p_whole, p_dec);
            OLED_PrintString(4, 0, OLEDString);
        }else{
            //Title for graph view
            OLED_PrintString(0, 0, "PW");
            //Bottom rows graph
            OLED_DrawGraph(power_history);
        }

        Delay_Ms(100);
    }
}
