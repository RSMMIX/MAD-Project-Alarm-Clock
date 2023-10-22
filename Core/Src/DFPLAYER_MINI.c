/*
 * DFPLAYER_MINI.c
 *
 *  Created on: May 16, 2020
 *      Author: controllerstech
 */


#include "stm32f7xx_hal.h"
#include "stdio.h"
//#include "main.c"

extern UART_HandleTypeDef huart6;
#define DF_UART &huart6

#define Source      0x02  // TF CARD

#define Previous_Key   GPIO_PIN_15
#define Previous_Port  GPIOB
#define Pause_Key      GPIO_PIN_13
#define Pause_Port     GPIOB
#define Next_Key       GPIO_PIN_12
#define Next_Port      GPIOB

/*************************************** NO CHANGES AFTER THIS *************************************************/

int ispause =0;
int isplaying=1;

extern uint16_t songList;
extern RNG_HandleTypeDef hrng;
uint16_t savedSong = 0, pre_num = 0;


# define Start_Byte 0x7E
# define End_Byte   0xEF
# define Version    0xFF
# define Cmd_Len    0x06
# define Feedback   0x00    //If need for Feedback: 0x01,  No Feedback: 0

void Send_cmd (uint8_t cmd, uint8_t Parameter1, uint8_t Parameter2)
{
	uint16_t Checksum = Version + Cmd_Len + cmd + Feedback + Parameter1 + Parameter2;
	Checksum = 0-Checksum;

	uint8_t CmdSequence[10] = { Start_Byte, Version, Cmd_Len, cmd, Feedback, Parameter1, Parameter2, (Checksum>>8)&0x00ff, (Checksum&0x00ff), End_Byte};

	HAL_UART_Transmit(DF_UART, CmdSequence, 10, HAL_MAX_DELAY);
}

void DF_PlayFromStart(void)
{
  Send_cmd(0x03,0x00,0x01);
  HAL_Delay(200);
}

void DF_SetEQ(uint8_t mode)
{
  Send_cmd(0x07,0x00, mode);
  HAL_Delay(200);
}

void DF_SetVolume(uint8_t volume)
{
  Send_cmd(0x06,0x00, volume);
  HAL_Delay(200);
}


void DF_Init (uint8_t volume)
{
	Send_cmd(0x3F, 0x00, Source);
	HAL_Delay(200);
	Send_cmd(0x06, 0x00, volume);
	HAL_Delay(500);
}

void DF_Next (void)
{
	Send_cmd(0x01, 0x00, 0x00);
	HAL_Delay(200);
}

void DF_Pause (void)
{
	Send_cmd(0x0E, 0, 0);
	HAL_Delay(200);
}

void DF_Previous (void)
{
	Send_cmd(0x02, 0, 0);
	HAL_Delay(200);
}

void DF_Playback (void)
{
	Send_cmd(0x0D, 0, 0);
	HAL_Delay(200);
}

void Check_Key (void)
{
	if (!HAL_GPIO_ReadPin(Pause_Port, Pause_Key))
	{
		while (!HAL_GPIO_ReadPin(Pause_Port, Pause_Key));
		if (isplaying)
		{
			ispause = 1;
			isplaying = 0;
			DF_Pause();
		}

		else if (ispause)
		{
			isplaying = 1;
			ispause = 0;
			DF_Playback();
		}
	}

	if (!HAL_GPIO_ReadPin(Previous_Port, Previous_Key))
	{
		while (!HAL_GPIO_ReadPin(Previous_Port, Previous_Key));
		DF_Previous();
	}

	if (!HAL_GPIO_ReadPin(Next_Port, Next_Key))
	{
		while (!HAL_GPIO_ReadPin(Next_Port, Next_Key));
		DF_Next();
	}
}


void selectSong (void)
{
	if (!HAL_GPIO_ReadPin(Pause_Port, Pause_Key))
	{
		HAL_Delay(50);
		if (!HAL_GPIO_ReadPin(Pause_Port, Pause_Key)){
			DF_SetFolder(1, songList);
			savedSong = songList;
		}
	}

	if (!HAL_GPIO_ReadPin(Previous_Port, Previous_Key))
	{
		HAL_Delay(50);
		if (!HAL_GPIO_ReadPin(Previous_Port, Previous_Key));
		songList--;
		//DF_Previous();
	}

	if (!HAL_GPIO_ReadPin(Next_Port, Next_Key))
	{
		HAL_Delay(50);
		if (!HAL_GPIO_ReadPin(Next_Port, Next_Key));
		songList++;
		//DF_Next();
	}

	if(songList < 1)
		songList = 10;
	else if(songList > 10)
		songList = 1;
}





void DF_SetFolder(uint8_t fol ,uint8_t num)
{
  Send_cmd(0x0F, fol, num);
  HAL_Delay(200);
}


void MusicController (uint32_t val)
{


	uint32_t value = val*30/4095;

	DF_SetVolume(value);

	if (!HAL_GPIO_ReadPin(Pause_Port, Pause_Key))
	{
		while (!HAL_GPIO_ReadPin(Pause_Port, Pause_Key));
		if (isplaying)
		{
			ispause = 1;
			isplaying = 0;
			DF_Pause();
		}

		else if (ispause)
		{
			isplaying = 1;
			ispause = 0;
			DF_Playback();
		}
	}

	if (!HAL_GPIO_ReadPin(Previous_Port, Previous_Key))
	{
		while (!HAL_GPIO_ReadPin(Previous_Port, Previous_Key)){
			uint16_t num = HAL_RNG_GetRandomNumber(&hrng) % 8 + 1;
			while(pre_num == num){
				num = HAL_RNG_GetRandomNumber(&hrng) % 8 + 1;
			}
			pre_num = num;
			DF_SetFolder(2, num);
		}

//		DF_SetFolder(1,4);
	}

	if (!HAL_GPIO_ReadPin(Next_Port, Next_Key))
	{
		while (!HAL_GPIO_ReadPin(Next_Port, Next_Key));
		//DF_SetFolder();
	}
}

void DF_loop(uint8_t file)
{
  Send_cmd(0x08,0x00, file);
  HAL_Delay(200);
}

void DF_repeat(void)
{
  Send_cmd(0x11,0x00, 0x01);
  HAL_Delay(200);
}

void DF_openFolder(uint8_t vol)
{
  Send_cmd(0x17,0x00, vol);
  HAL_Delay(200);
}






