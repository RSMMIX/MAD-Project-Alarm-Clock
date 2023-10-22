/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "rng.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "ILI9341_Touchscreen.h"

#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX.h"

#include "snow_tiger.h"

#include "DFPLAYER_MINI.h"
#include "ds3231_for_stm32_hal.h"
#include "string.h"
//#include "dfr0299.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
volatile uint32_t timer_ticks = 0;
uint32_t count = 0;
uint32_t starttime = 0;
uint32_t alarmtime = 0;
uint8_t screen = 1;
uint8_t sec = 0,min = 0,hur = 0,dow = 0,date = 0,month = 0;
uint32_t year = 0;
uint8_t selectionMode = 0;
uint8_t valueMode = 0;

// Time Variable
uint32_t setSec = 99,setMin = 99,setHour = 99;
uint32_t setDate = 99,setMonth = 99,setYear = 99;
uint32_t setDay = 0;

// Alarm Varable
uint32_t setAlarmSec = 0,setAlarmMin = 0,setAlarmHour = 0;
uint32_t setAlarmDate = 99,setAlarmMonth = 99,setAlarmYear = 99;
uint32_t setAlarmDay = 0;

// Additional Date Variable
char *day[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
char *months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
int monthdays[13] = {31,28,29,31,30,31,30,31,31,30,31,30,31};

uint8_t seconds;
uint8_t minutes;
uint8_t hours;

// Alarm Variable
int alarmtrigger = 0;

// Sensor Variable
float temp = 0.0;
float RH = 0.0;
uint8_t step = 0;
HAL_StatusTypeDef status;

//??
uint8_t cmdBuffer[3];
uint8_t dataBuffer[8];

// Display Variable
char strS[] ="";
char strM[] ="";
char strH[] ="";
char timedate[50];
char strdow[] ="";
char strdate[] ="";
char strmonth[15] ="";
char stryear[] ="";
char strsensor[50] ="";

char timeString[50];

//ปิดปลุก
uint32_t randomNumbers[4];
HAL_StatusTypeDef randomStatus;
uint32_t userButtonInput[4] = {0};
char buttonInput[] ="";
char space[] = "\r\n";

uint8_t pwm;
float dutyCycle = 0;
int playTheMusic = 1;

// Controller Variable
uint8_t currentIndex = 0, previousScreen = 0;
uint32_t clicktime = 0, c=0, debounceTime = 0, teratermTime = 0;
int enterStateFlag = 0;

uint32_t debounceTimeSave = 0;

uint16_t CRC16_2(uint8_t * , uint8_t);

void showSetTime();
void showSetAlarm();
void setPage();

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void sendStringViaUART(const char *str) {
  HAL_UART_Transmit(&huart3, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

void generateAndSortRandomNumbers(uint32_t array[], uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    array[i] = abs(HAL_RNG_GetRandomNumber(&hrng)) % 4 + 1;
  }

  char message[100];

  for (int i = 0; i < 4; i++) {
    sprintf(message, "%d ", array[i]);
    sendStringViaUART(message);
  }
  sendStringViaUART(space);
}

int compareArrays(int array1[], int array2[], int size) {
    int match = 1;  // Assume arrays match initially

    // Compare elements of the arrays
    for (int i = 0; i < size; i++) {
        if (array1[i] != array2[i]) {
            match = 0;  // Arrays do not match
            break;
        }
    }

    if (match) {
    	return 1;
    	//char message[] = "Match\r\n";
    	//HAL_UART_Transmit(&huart3, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
    } else {
    	return 0;
    	//char message[] = "Not Match\r\n";
    	//HAL_UART_Transmit(&huart3, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
    }
}

void letTheMusicLouderThanAnySound(float dt){
	float dutyCycle = dt;
	htim2.Instance -> CCR4 = (10000-1) * (dutyCycle);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
	pwm = (GPIOA->IDR & GPIO_PIN_3) >> 3;
}

void stopTheMusic(){
	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
	HAL_GPIO_WritePin(GPIOC, 8, GPIO_PIN_RESET);
	dutyCycle = 0;
}
/*void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim2) {
        timer_ticks++;
        if (timer_ticks >= 3000) { // 3 วินาที
            // หยุดนับเวลา�?ละรีเซ็ตค่า
            HAL_TIM_Base_Stop_IT(&htim2);
            timer_ticks = 0;

            // ทำสิ่งที่คุณต้อง�?ารเมื่อค้าง 3 วินาที
            // เช่น อ่านค่าต้อ
        }
    }
}
void generateAndSortRandomNumbers(uint32_t array[], uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    array[i] = abs(HAL_RNG_GetRandomNumber(&hrng)) % 4 + 1;
  }

  char message[100];

  for (int i = 0; i < 4; i++) {
    sprintf(message, "%d ", array[i]);
    sendStringViaUART(message);
  }
  sendStringViaUART(space);
}

void compareArrays(int array1[], int array2[], int size) {
    int match = 1;  // Assume arrays match initially

    // Compare elements of the arrays
    for (int i = 0; i < size; i++) {
        if (array1[i] != array2[i]) {
            match = 0;  // Arrays do not match
            break;
        }
    }

    if (match) {
    	char message[] = "Match\r\n";
    	HAL_UART_Transmit(&huart3, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
    } else {
    	char message[] = "Not Match\r\n";
    	HAL_UART_Transmit(&huart3, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
    }
}

void letTheMusicLouderThanAnySound(){
	float dutyCycle = 1;
	htim3.Instance -> CCR3 = (10000-1) * (dutyCycle);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
	pwm = (GPIOC->IDR & GPIO_PIN_8) >> 10;
}

void stopTheMusic(){
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
	HAL_GPIO_WritePin(GPIOC, 8, GPIO_PIN_RESET);
	dutyCycle = 0;
}*/
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */


  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_SPI5_Init();
  MX_TIM1_Init();
  MX_RNG_Init();
  MX_I2C4_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  MX_USART6_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  ILI9341_Init();//initial driver setup to drive ili9341
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_UART_Init(&huart3);

  DF_Init(30);
	DF_PlayFromStart();
	DF_SetEQ(3);
	DS3231_Init(&hi2c4);
	__disable_irq();
	//Set interrupt mode to square wave mode, enable square wave interrupt at pin 3.
	DS3231_SetInterruptMode(DS3231_SQUARE_WAVE_INTERRUPT);
	//Set interrupting frequency to 1 Hz.
	DS3231_SetRateSelect(DS3231_1HZ);
	//Enable interrupts after finishing.
	__enable_irq();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    ILI9341_Fill_Screen(WHITE);
    ILI9341_Set_Rotation(SCREEN_HORIZONTAL_1);
     starttime = count;

     cmdBuffer[0] = 0x03;
     cmdBuffer[1] = 0x00;
     cmdBuffer[2] = 0x04;
     //DS3231_SetFullDate(13, 10, 5, 2023);
     DS3231_SetInterruptMode(DS3231_ALARM_INTERRUPT);
	DS3231_ClearAlarm1Flag();

	DS3231_EnableAlarm1(DS3231_ENABLED);
	DS3231_SetAlarm1Mode(DS3231_A1_MATCH_S_M_H_DAY);
//	DS3231_SetAlarm1Second(5);
//	DS3231_SetAlarm1Minute(29);
//	DS3231_SetAlarm1Hour(19);
     ILI9341_Draw_Rectangle(0,0, 320,200,  WHITE);

     //generateAndSortRandomNumbers(randomNumbers, 4);
   //  DS3231_SetFullDate(30,5, 4, 2002);
   while (1)
   {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	   // Debug time Tera term
	   if ( count - teratermTime > 3000){
		   char message[20];
		   sprintf(message, "%d\r\n", screen);
		   HAL_UART_Transmit(&huart3, (uint8_t *)message, sizeof(message), HAL_MAX_DELAY);
		   teratermTime = count;

	   }

	   // If Alarm trigger
		if(alarmtrigger)
		{
			ILI9341_Fill_Screen(WHITE);
			alarmtime = count;
			alarm();
		   //screen = 2;
		   //alarmtrigger = 0;
		   //HAL_Delay(3000)
		}
		else{
	   // Screen state looping
	   switch(screen){
	   case 1: // First state : show time
		   // Update screen every second
		   if((count-starttime) > 1000){
				updateT();
				showT();
				starttime = count;
			}
//		   Check_Key();

			if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET){
				 HAL_Delay(50);
				if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET){
					DS3231_SetFullTime(21 , 12, 30);
				}
			 }
//			// If Alarm trigger
//			if(alarmtrigger)
//			{
//				ILI9341_Fill_Screen(WHITE);
//				alarm();
//			   //screen = 2;
//			   alarmtime = count;
//			   alarmtrigger = 0;
//			   //HAL_Delay(3000)
//			}
			// Go to state 2
			if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET && count - clicktime > 1000){
					ILI9341_Fill_Screen(WHITE);
					clicktime = count;
					screen = 2;
			}

			break;

	   case 2:// Second State : Set Time
		   // If enter this state at first , set flag and time to set to current time
		   if(enterStateFlag == 0)
		   {
			   setMin = DS3231_GetMinute();
			   setHour = DS3231_GetHour();
			   setDate = date;
			   setMonth = month;
			   setYear = year;
			   currentIndex = 0;
			   enterStateFlag = 1;
		   }
		   //?
		   if(setDate == 99){

			   	   setMin = DS3231_GetMinute();
			   	   setHour = DS3231_GetHour();
				   setDate = date;
				   setMonth = month;
				   setYear = year;
				   currentIndex = 0;

			   }
		   // Setting
			   if(count-starttime > 1000){
//			   		showSetTime();
				    setPage();
			   		starttime = count;
			   	}
			   break;

	   case 3:// Third State : Set Alarm
		   		   // If enter this state at first , set flag and index
		   if(enterStateFlag == 0)
		   {
			   currentIndex = 2;
			   enterStateFlag = 1;
		   }
		   if(setHour == 99){
			   setHour = hur;
			   setMin = min;
			   setSec = sec;
			   currentIndex = 0;
		   }
		   //Setting
		   if(count-starttime > 1000){
//				showSetAlarm();
				setPage();
				starttime = count;
			}
		   break;

	   case 4:
		   Check_Key();
		   setPage();
		   break;

	   }

	   char message[50];
	   //sprintf(message, "2: %d %d %d | 3: %d %d %d | cur = %d\r\n", setDate, setMonth, setYear, setHour, setMin, setSec, currentIndex);
	   //HAL_UART_Transmit(&huart3, (uint8_t *)message, sizeof(message), HAL_MAX_DELAY);

	   if (previousScreen !=  screen){
		   currentIndex = 0;
		   previousScreen = screen;
	   }

//	   for(int i = 0; i<= 100; i++){
//	   		  letTheMusicLouderThanAnySound(i*0.01);
//	   		  HAL_Delay(2000);
//	   		  stopTheMusic();
//	   	  }
   }
   }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 200;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
uint16_t CRC16_2(uint8_t *ptr, uint8_t lenght){
	uint16_t crc = 0xffff;
	uint8_t s = 0x00;

	while (lenght--){
		crc ^= *ptr++;

		for (s = 0 ; s <8 ;s++){
			if ((crc & 0x01) != 0){
				crc >>= 1;
				crc ^= 0xA001;

			}
			else{
				crc >>= 1;
			}
		}
	}
	return crc;
}

void updateSensor() {
	HAL_I2C_Master_Transmit(&hi2c1, 0x5c << 1 , cmdBuffer, 3, 200);

	HAL_I2C_Master_Transmit(&hi2c1, 0x5c << 1 , cmdBuffer, 3, 200);

	HAL_Delay(1);

	// receive sensor data

	HAL_I2C_Master_Receive(&hi2c1, 0x5c << 1 , dataBuffer , 8, 200);

	uint16_t Rcrc = dataBuffer[7] << 8 ;
	Rcrc += dataBuffer[6];

	if (Rcrc == CRC16_2(dataBuffer,6)){
		uint16_t temperature = ((dataBuffer[4] & 0x7F) << 8 ) + dataBuffer[5];

		temp = temperature  / 10.0 ;

		temp = (((dataBuffer[4] & 0x80) >> 7 ) == 1)? (temp * (-1)) : temp ;

		uint16_t humi = (dataBuffer[2] << 8) + dataBuffer[3] ;
		RH = humi / 10.0 ;
	}
}

void alarm(){
	//-----------------------------------
	ILI9341_Draw_Text("WAKE UP!!", 50,50, BLACK, 4, WHITE);
	//-----------------------------------
	generateAndSortRandomNumbers(randomNumbers,4);
	char pinNumber[4][2];
//	snprintf(buttonInput, 15, "%d %d %d %d",randomNumbers[0],randomNumbers[1],randomNumbers[2],randomNumbers[3]);
	for(int i=0;i<4;i++){
		sprintf(pinNumber[i], "%d", randomNumbers[i]);
		ILI9341_Draw_Text(pinNumber[i], 50 + i*30,100, BLACK, 4, WHITE);
	}
//	ILI9341_Draw_Text(buttonInput, 50,100, BLACK, 4, WHITE);
	int cnt = 0;
	while(1){
		if(cnt != 4){
			if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_RESET){
				userButtonInput[cnt++] = 1;
				HAL_Delay(1000);
			}
			else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET){
				userButtonInput[cnt++] = 2;
				HAL_Delay(1000);
			}
			else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET){
				userButtonInput[cnt++] = 3;
				HAL_Delay(1000);
			}
			else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET){
				userButtonInput[cnt++] = 4;
				HAL_Delay(1000);
			}
		}
		else{
//			snprintf(buttonInput, 15, "%d %d %d %d",userButtonInput[0],userButtonInput[1],userButtonInput[2],userButtonInput[3]);
//			ILI9341_Draw_Text(buttonInput, 50,10, BLACK, 4, WHITE);
			if(compareArrays(randomNumbers,userButtonInput,4) == 1){
				ILI9341_Fill_Screen(WHITE);
				alarmtrigger = 0;
				break;
			}else{
				generateAndSortRandomNumbers(randomNumbers,4);
				for(int i=0;i<4;i++){
					sprintf(pinNumber[i], "%d", randomNumbers[i]);
					ILI9341_Draw_Text(pinNumber[i], 50 + i*30,100, BLACK, 4, WHITE);
					userButtonInput[i] = 0;
				}
//				snprintf(buttonInput, 15, "%d %d %d %d",randomNumbers[0],randomNumbers[1],randomNumbers[2],randomNumbers[3]);
//				ILI9341_Draw_Text(buttonInput, 50,100, BLACK, 4, WHITE);
				cnt = 0;
			}
		}
		for(int i=0;i<4;i++){
			ILI9341_Draw_Text(pinNumber[i], 50 + i*30,100, (userButtonInput[i] == randomNumbers[i]) ? GREEN : ((userButtonInput[i] == 0) ? BLACK : RED), 4, WHITE);
		}
	}
}

 void updateT(){
 	sec = DS3231_GetSecond();
 	min = DS3231_GetMinute();
 	hur = DS3231_GetHour();
 	dow = DS3231_GetDayOfWeek()-1;
 	date = DS3231_GetDate()-1;
 	month = DS3231_GetMonth()-1;
 	year = DS3231_GetYear();

 }
 void showT(){

	updateSensor();

//	if(setHour == 99 || setMin == 99 || setSec == 99){
//		sprintf(strH,"%02d",hur);
//		sprintf(strM,"%02d",min);
//		sprintf(strS,"%02d",sec);
//	}
//	else {
//		sprintf(strH,"%02d",setHour);
//		sprintf(strM,"%02d",setMin);
//		sprintf(strS,"%02d",setSec);
//	}

	sprintf(strH,"%02d",hur);
	sprintf(strM,"%02d",min);
	sprintf(strS,"%02d",sec);

 	sprintf(strsensor,"%.1f C %.1f %%RH",temp,RH);
 	//sprintf(strdow,"%s",day[1]);
	//sprintf(strdate,"%d",date);
//	sprintf(strmonth,"%s",months[month]);
	//sprintf(stryear,"%d",year);

// 	if (setDate == 99 || setMonth == 99 || setYear == 99){
// 		dow = dayofweek(date, month + 1, year);
// 		snprintf(timedate,50,"%d %s %d",date, months[month], year);
// 	}
// 	else{
// 		dow = dayofweek(setDate, setMonth + 1, setYear);
// 		snprintf(timedate,50,"%d %s %d",setDate, months[setMonth], setYear);
// 	}

 	dow = dayofweek(date + 1, month + 1, year);
 	snprintf(timedate,50,"%d %s %d",date+1, months[month], year);

	snprintf(timeString, 50, "%s:%s:%s", strH, strM, strS);
	ILI9341_Draw_Text(timeString, 42, 100, BLACK, 5, WHITE);
//	ILI9341_Draw_Text(strH, 50,90, BLACK, 5, WHITE);
//	ILI9341_Draw_Text(strM, 120,90, BLACK, 5, WHITE);
//	ILI9341_Draw_Text(strS, 210,90, BLACK, 5, WHITE);

	uint32_t dow_x = 56 + (9 - strlen(day[dow])) * 12;
	ILI9341_Draw_Text(day[dow], dow_x,10 , BLACK, 4, WHITE);
	ILI9341_Draw_Text(strsensor, 75,40 , BLACK, 2, WHITE);
 //	ILI9341_Draw_Text(strsensor, 80,40 , BLACK, 1, WHITE);
//	ILI9341_Draw_Text(strdate, 50,210, BLACK, 2, WHITE);
//	ILI9341_Draw_Text(months[month], 80,210, BLACK, 2, WHITE);
//	ILI9341_Draw_Text(stryear, 200,210, BLACK, 2, WHITE);
	uint32_t time_x = 50 + (9-((setMonth == 99) ? strlen(months[month]) : strlen(months[setMonth]))) * 7;
//	if (month % 2 == 1)
//		x -= 5;
 	ILI9341_Draw_Text(timedate, time_x, 210, BLACK, 2, WHITE);
//	ILI9341_Draw_Text(timedate, 50, 210, BLACK, 2, WHITE);
	char willBeAlarmIn[50] = "";
	snprintf(willBeAlarmIn, 50, "Alarm at %02d:%02d", setAlarmHour, setAlarmMin);
	ILI9341_Draw_Text(willBeAlarmIn, 75, 140, BLACK, 2, WHITE);

 }

void showSetTime(){
	sprintf(strdate,"%02d",setDate+1);
	sprintf(strmonth,"%s",months[setMonth]);
	sprintf(stryear,"%d",setYear);


//	snprintf(timedate,50,"%s %s %s",strdate, strmonth, stryear);
//	ILI9341_Draw_Text(timedate, 80,10, BLACK, 2, WHITE);


	char getStr[100] = "";
	sprintf(getStr,"Setting");
	ILI9341_Draw_Text(getStr, 72,10, BLACK, 4, WHITE);

	sprintf(getStr,"Date");
	ILI9341_Draw_Text(getStr, 115,60, BLACK, 3, WHITE);

	sprintf(getStr,"Time");
	ILI9341_Draw_Text(getStr, 115,130, BLACK, 3, WHITE);

	if (currentIndex == 0)
		ILI9341_Draw_Text(strdate, 55,100, BLUE, 2, WHITE);
	else
		ILI9341_Draw_Text(strdate, 55,100, BLACK, 2, WHITE);




	if (currentIndex == 1)
		ILI9341_Draw_Text(strmonth, 100,100, BLUE, 2, WHITE);
	else
		ILI9341_Draw_Text(strmonth, 100,100, BLACK, 2, WHITE);



	if (currentIndex== 2)
		ILI9341_Draw_Text(stryear, 215,100, BLUE, 2, WHITE);
	else
		ILI9341_Draw_Text(stryear, 215,100, BLACK, 2, WHITE);


	// hour

	sprintf(getStr,"%02d",setHour);



	if (currentIndex== 3)
			ILI9341_Draw_Text(getStr, 120,170, BLUE, 2, WHITE);
		else
			ILI9341_Draw_Text(getStr, 120,170, BLACK, 2, WHITE);

	sprintf(getStr,":");
	ILI9341_Draw_Text(getStr, 150,170, BLACK, 2, WHITE);

	// minute
	sprintf(getStr,"%02d",setMin);

	if (currentIndex== 4)
			ILI9341_Draw_Text(getStr, 160,170, BLUE, 2, WHITE);
		else
			ILI9341_Draw_Text(getStr, 160,170, BLACK, 2, WHITE);

//	ILI9341_Draw_Text(day[dow], 50, 10 , BLACK, 4, WHITE);
}

void showSetAlarm(){



		char getStr[100] = "";


		sprintf(getStr,"Setting Alarm Time");
		ILI9341_Draw_Text(getStr, 50, 20, BLACK , 2, WHITE);


		ILI9341_Draw_Text("Hour   Minute", 90,90, BLACK, 2, WHITE);

		sprintf(getStr,"%02d",setAlarmHour);

		if (currentIndex== 2)
			ILI9341_Draw_Text(getStr, 95,110, BLUE, 3, WHITE);
		else
			ILI9341_Draw_Text(getStr, 95,110, BLACK, 3, WHITE);

		// minute
		sprintf(getStr,"%02d",setAlarmMin);

		if (currentIndex== 3)
			ILI9341_Draw_Text(getStr, 184,110, BLUE, 3, WHITE);
		else
			ILI9341_Draw_Text(getStr, 184,110, BLACK, 3, WHITE);

		sprintf(getStr,"%s",day[setAlarmDay]);
		uint32_t dow_x = 75 + (9 - strlen(day[setAlarmDay])) * 12;
		if (currentIndex== 4)
			ILI9341_Draw_Text(getStr, dow_x,175, BLUE, 3, WHITE);
		else
			ILI9341_Draw_Text(getStr, dow_x,175, BLACK, 3, WHITE);



		ILI9341_Draw_Text(":", 155,110, BLACK, 3, WHITE);
//	ILI9341_Draw_Text(strH, 60,20, BLACK, 2, WHITE);
//	ILI9341_Draw_Text(strM, 110,20, BLACK, 2, WHITE);
	//ILI9341_Draw_Text(strS, 160,20, BLACK, 2, WHITE);
	//ILI9341_Draw_Text(day[dow], 60,200, BLACK, 2, WHITE);


//	ILI9341_Draw_Text(day[dow], 10, 200 , BLACK, 4, WHITE);
}

void setPage(){
	if((count-starttime) > 1000){
		if(screen == 2)
			showSetTime();
		else if(screen == 3)
			showSetAlarm();
		starttime = count;
	}
//   Check_Key();


   if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) == GPIO_PIN_RESET && count - debounceTime > 1000){
	   debounceTime = count;
	   switch(currentIndex){
	   case 0:
		   if(screen == 2){
			   setDate++;
			   if(setMonth == 3 || setMonth == 5 || setMonth == 8 || setMonth == 10)
				   setDate %= 30;
			   else if(setMonth == 1)
			   {
				   setDate = setYear%4==0 ? setDate%29 : setDate%28;
			   }
			   else{
				   setDate %= 31;
			   }
		   }

		   break;
	   case 1:
		   if(screen == 2) {
			   setMonth++;
			   setMonth %= 12;


			   ILI9341_Draw_Rectangle(90,100, 125,30,  WHITE);
		   }

		   break;
	   case 2:
		   if(screen == 2) {setYear++;}
		   else if(screen == 3){
		   			   setAlarmHour++;
		   			   setAlarmHour %=24;
		   		   }

		   break;
	   case 3:
		   if(screen == 2){
			   setHour++;
			   setHour %= 24;
		   }

	   		else if(screen == 3){
	   			setAlarmMin++;
	   			setAlarmMin %=60;
	   		}

		   break;
	   case 4:
	   		if(screen == 2){
	   			   setMin++;
	   			   setMin %= 60;
	   		   }

	   		else if(screen == 3){
	   			setAlarmDay++;
	   			setAlarmDay %= 7;

	   			ILI9341_Draw_Rectangle( 70,175, 200,40,  WHITE);
	   		}

	   		break;

	   }
   }
   else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET && count - debounceTime > 1000){
	   debounceTime = count;
	   switch(currentIndex){
	   case 0:
		   if(screen == 2){
			   if(setDate > 0)
				   setDate--;
		   }

		   break;
	   case 1:
		   if(screen == 2) {
			   if (setMonth > 0)
				   setMonth--;
//			   ILI9341_Draw_Rectangle(75,200, 125,40,  WHITE);

		   }

		   break;
	   case 2:
		   if(screen == 2) {
			   if(setYear > 0)
				   setYear--;
		   }
		   else if(screen == 3){
			   if(setAlarmHour > 0){
					   setAlarmHour--;

						   }
		   }

		   break;

	   case 3:
		   if(screen == 2){
			   if(setHour > 0){
	   			   setHour--;
	   		   }
		   }

		   else if(screen == 3){
			   if(setAlarmMin > 0){
					   setAlarmMin--;
					   ILI9341_Draw_Rectangle(90,175, 125,30,  WHITE);
						   }
		   }
		   break;


	   case 4:
	   		   if(screen == 2){
	   			   if(setMin > 0){
	   	   			   setMin--;
	   	   		   }
	   		   }

	   		else if(screen == 3){
			   if(setAlarmDay > 0){
					   setAlarmDay--;
						   }
			   ILI9341_Draw_Rectangle(60,200, 200,40,  WHITE);
//			   ILI9341_Draw_Rectangle(90,200, 125,40,  RED);
		   }

	   		   break;

	   }
   }
   else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET && count - debounceTime > 100){
	   debounceTime = count;
//	   if(selectionMode){

//	   }
//	   else{

		   // update setting Time


		   if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET && count - debounceTimeSave > 3000){


			   if(screen == 2){
			   DS3231_SetFullDate(setDate+1,setMonth+1, dayofweek(setDate + 1, setMonth + 1, setYear), setYear);
			   DS3231_SetMinute(setMin);
			   DS3231_SetHour(setHour);


			   }
			   else if (screen == 3){

				   DS3231_SetAlarm1Hour(setAlarmHour);
				   DS3231_SetAlarm1Minute(setAlarmMin);
				   DS3231_SetAlarm1Second(0);
				   DS3231_SetAlarm1Day(setAlarmDay+1);  // ใส่ day of week
			   }


			   debounceTimeSave = count;


			   HAL_UART_Transmit(&huart3, (uint8_t *)"SAVED!!", sizeof("SAVED!!"), HAL_MAX_DELAY);
		   }
		   else{

			   if (screen==2){
			   currentIndex += 1;
			   currentIndex %= 5;
			   }
			   else if(screen==3){



				   currentIndex += 1;

				   if (currentIndex < 2 || currentIndex > 4){
					   currentIndex = 2;
				   }

//				   if (currentIndex > 6){
//					   currentIndex = 2;
//				   }
			   }
		   }

//
//	   }
   }

   else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET && count - debounceTime > 200 ){
	   debounceTime = count;
	   ILI9341_Fill_Screen(WHITE);
	   clicktime = count;

	   while (count - clicktime < 1000){
		   char message[50];
		   sprintf(message, "%d %d\r\n", clicktime, count);
		   HAL_UART_Transmit(&huart3, (uint8_t *)message, sizeof(message), HAL_MAX_DELAY);
	   }

	   if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET && count - debounceTime >3000) {
		   screen--;
		   debounceTime = count;
	   }
	   else {
		   enterStateFlag = 0;
		   screen++;
		   if (screen > 4)
			   screen = 1;
	   }

//	   	   while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET)
//	   	   {
//	   		   if(count - clicktime > 3000){
//	   			   c = 1;
//	   			   screen--;
//	   			   lastclicktime = count;
//	   			   break;
//	   		   }
//	   	   }
//
	   	   //if (c == 0)
	   	   //{
	   		//   char message[20];
	   		//   sprintf(message, "XXXX");
	   		//   HAL_UART_Transmit(&huart3, (uint8_t *)message, sizeof(message), HAL_MAX_DELAY);
	   	  // }
	   	   //c = 0;

//		if(screen == 2) {
//		   setDate = param1;
//		   setMonth = param2;
//		   setYear = param3;
//	   }
//	   else if (screen == 3){
//		   setHour = param1;
//		   setMin = param2;
//		   setSec = param3;
//	   }
   }
}


int dayofweek(int d, int m, int y)
{
	int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

	// if month is less than 3 reduce year by 1
	if (m < 3)
		y -= 1;

	return ((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
