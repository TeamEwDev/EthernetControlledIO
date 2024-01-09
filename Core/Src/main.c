/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include "socket.h"
#include "dhcp.h"
#include "dns.h"

#define DHCP_SOCKET     0
#define DNS_SOCKET      1
#define HTTP_SOCKET     2
#define CLIENT_SOCKET	2	//tcp client socket 1
#define LISTEN_PORT	    7 	//server port

#if defined (__ICCARM__) || defined (__ARMCC_VERSION)
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#elif defined(__GNUC__)
/* With GCC, small printf (option LD Linker->Libraries->Small printf
set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#endif /* __ICCARM__ || __ARMCC_VERSION */

wiz_NetInfo netInfo = {
		.mac = { 0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef },
		.ip = { 192, 168, 1, 180 },
		.sn = { 255, 255, 255, 0 },
		.gw = { 192, 168, 1, 1 } };

wiz_NetTimeout timeout = {
		.retry_cnt = 3, 		//RCR = 3
		.time_100us = 5000};    //500ms

uint8_t buffer[8];		//client sends 8 bytes
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void W5500_Select(void) {
    HAL_GPIO_WritePin(SPI1_NSS_W5500_GPIO_Port, SPI1_NSS_W5500_Pin, GPIO_PIN_RESET);
}

void W5500_Unselect(void) {
    HAL_GPIO_WritePin(SPI1_NSS_W5500_GPIO_Port, SPI1_NSS_W5500_Pin, GPIO_PIN_SET);
}

void W5500_ReadBuff(uint8_t* buff, uint16_t len) {
    HAL_SPI_Receive(&hspi1, buff, len, HAL_MAX_DELAY);
}

void W5500_WriteBuff(uint8_t* buff, uint16_t len) {
    HAL_SPI_Transmit(&hspi1, buff, len, HAL_MAX_DELAY);
}

uint8_t W5500_ReadByte(void) {
    uint8_t byte;
    W5500_ReadBuff(&byte, sizeof(byte));
    return byte;
}

void W5500_WriteByte(uint8_t byte) {
    W5500_WriteBuff(&byte, sizeof(byte));
}

bool NetworkInit_W5500() {
	wiz_NetInfo tmpInfo;
	wiz_NetTimeout tmpTimeout;
	wizchip_setnetinfo(&netInfo);

	//get network information
	wizchip_getnetinfo(&tmpInfo);
	printf("IP: %03d.%03d.%03d.%03d\nGW: %03d.%03d.%03d.%03d\nNet: %03d.%03d.%03d.%03d\nPort : %d\n",
			tmpInfo.ip[0], tmpInfo.ip[1],tmpInfo.ip[2], tmpInfo.ip[3],
			tmpInfo.gw[0], tmpInfo.gw[1], tmpInfo.gw[2], tmpInfo.gw[3],
			tmpInfo.sn[0], tmpInfo.sn[1], tmpInfo.sn[2], tmpInfo.sn[3],
			LISTEN_PORT);

	if(tmpInfo.mac[0] != netInfo.mac[0] ||
			tmpInfo.mac[1] != netInfo.mac[1] ||
			tmpInfo.mac[2] != netInfo.mac[2] ||
			tmpInfo.mac[3] != netInfo.mac[3])
	{
		printf("wizchip_getnetinfo failed.\n");
		return false;
	}

	//set timeout
	ctlnetwork(CN_SET_TIMEOUT,(void*)&timeout);
	ctlnetwork(CN_GET_TIMEOUT, (void*)&tmpTimeout);

	if(tmpTimeout.retry_cnt != timeout.retry_cnt || tmpTimeout.time_100us != timeout.time_100us)
	{
		printf("ctlnetwork(CN_SET_TIMEOUT) failed.\n");
		return false;
	}

	return true;
}

bool Init_W5500() {
    printf("\r\ninit() called!\r\n");

	HAL_GPIO_WritePin(RESET_W5500_GPIO_Port, RESET_W5500_Pin, GPIO_PIN_RESET);
	HAL_Delay(500);
	HAL_GPIO_WritePin(RESET_W5500_GPIO_Port, RESET_W5500_Pin, GPIO_PIN_SET);
	HAL_Delay(500);
    printf("Registering W5500 callbacks...\r\n");
    reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
    reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
    reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);

    printf("Calling wizchip_init()...\r\n");
	uint8_t version = getVERSIONR();
	if(version != 0x04)
	{
		printf("getVERSIONR returns wrong version!\n");
		return false;
	}
	wiz_PhyConf phyConf;
	wizphy_getphystat(&phyConf);
	printf("PHY conf.by = {%d}, conf.mode={%d}, conf.speed={%d}, conf.duplex={%d}\n",
			phyConf.by, phyConf.mode, phyConf.speed, phyConf.duplex);
}

void Start_Listening_To_TCP_Client() {
	int32_t ret;
	uint8_t remoteIP[4];
	uint16_t remotePort;
	while(1) {
		uint8_t ret = socket(CLIENT_SOCKET, Sn_MR_TCP, LISTEN_PORT, SF_TCP_NODELAY);

		if (ret < 0) {
			printf("socket failed{%ld}.\n", ret);
			close(CLIENT_SOCKET);
			HAL_Delay(100);
			continue;
		}

		//check initialization
		while(getSn_SR(CLIENT_SOCKET) != SOCK_INIT)
		{
			HAL_Delay(10);
		}

		printf("listening....\n");
		ret = listen(CLIENT_SOCKET);
		if (ret < 0) {
			printf("listen failed{%ld}.\n", ret);
			close(CLIENT_SOCKET);
			HAL_Delay(100);
			continue;
		}

		//check listening status
		while(getSn_SR(CLIENT_SOCKET) == SOCK_LISTEN)
		{
			HAL_Delay(10);
		}

		if(getSn_SR(CLIENT_SOCKET) == SOCK_ESTABLISHED)
		{
			//client accepted
			printf("accepted....\n");

			//get remote information
			getsockopt(CLIENT_SOCKET, SO_DESTIP, remoteIP);
			getsockopt(CLIENT_SOCKET, SO_DESTPORT, (uint8_t*)&remotePort);
			printf("remote IP[PORT]:%03d.%03d.%03d.%03d[%05d]\n",
					remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], remotePort);

			//receive data
			ret = recv(CLIENT_SOCKET, buffer, sizeof(buffer));
			if (ret < 0) {
				printf("recv failed.{%ld}\n", ret);
				close(CLIENT_SOCKET); //unexpected close
				continue;
			}

			printf("received...\n %s", buffer);

			//send back data
			ret = send(CLIENT_SOCKET, buffer, sizeof(buffer));
			if (ret < 0) {
				printf("send failed{%ld}.\n", ret);
				close(CLIENT_SOCKET); //unexpected close
				continue;
			}

			printf("sent back...\n");
		}
		else
		{
			printf("getSn_SR() != SOCKET_ESTABLISHED.\n");
		}

		//close socket
		close(CLIENT_SOCKET);
		printf("closed...\n");
	}
}

void loop() {
	HAL_Delay(1000);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

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
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  Init_W5500();
  NetworkInit_W5500();
  Start_Listening_To_TCP_Client();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, RESET_W5500_Pin|SPI1_NSS_W5500_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(REJECTOR_OUTPUT_24V_GPIO_Port, REJECTOR_OUTPUT_24V_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : RESET_W5500_Pin SPI1_NSS_W5500_Pin */
  GPIO_InitStruct.Pin = RESET_W5500_Pin|SPI1_NSS_W5500_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : REJECTOR_INPUT_24V_Pin */
  GPIO_InitStruct.Pin = REJECTOR_INPUT_24V_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(REJECTOR_INPUT_24V_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : REJECTOR_OUTPUT_24V_Pin */
  GPIO_InitStruct.Pin = REJECTOR_OUTPUT_24V_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(REJECTOR_OUTPUT_24V_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
PUTCHAR_PROTOTYPE
{
    /* Place your implementation of fputc here */
    volatile uint32_t retry = 500;
    while ((CDC_Transmit_FS((uint8_t *)&ch, 1) != 0) && retry)
    {
        retry--;
    }
    return ch;
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
  __disable_irq();
  while (1)
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
