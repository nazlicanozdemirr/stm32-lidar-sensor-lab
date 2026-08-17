/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include <string.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Sensor packet structure (9 bytes)
// Format: [Header(0x59,0x59)] [Dist_L] [Dist_H] [Strength_L] [Strength_H] [Temp_L] [Temp_H] [CRC] [Reserved]
typedef struct {
    uint8_t header[2];      // 0x59, 0x59
    uint8_t distLow;        // Distance low byte
    uint8_t distHigh;       // Distance high byte
    uint8_t strengthLow;    // Signal strength low byte
    uint8_t strengthHigh;   // Signal strength high byte
    uint8_t tempLow;        // Temperature low byte
    uint8_t tempHigh;       // Temperature high byte
    uint8_t crc;            // Checksum
} SensorPacket_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SENSOR_PACKET_SIZE      9       // Single packet size
#define SENSOR_BUF_SIZE         18      // Buffer for 2 packets (for safety)
#define SENSOR_HEADER_BYTE1     0x59    // First header byte
#define SENSOR_HEADER_BYTE2     0x59    // Second header byte
#define UART_TIMEOUT_MS         100     // UART transmit timeout
#define MAX_DISTANCE_CM         1200    // Maximum valid distance in cm
#define TEST_MODE               1       // Set to 1 for raw data test, 0 for normal operation

// Connection test parameters
#define CONNECTION_TIMEOUT_MS   2000    // Timeout for sensor connection test (2 seconds)
#define LED_BLINK_INTERVAL_MS   500     // LED blink interval for status indication

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// Calculate distance from packet bytes
#define CALCULATE_DISTANCE(low, high)   ((uint16_t)(low) | ((uint16_t)(high) << 8))

// Calculate signal strength from packet bytes
#define CALCULATE_STRENGTH(low, high)   ((uint16_t)(low) | ((uint16_t)(high) << 8))

// Calculate temperature from packet bytes (signed)
#define CALCULATE_TEMPERATURE(low, high) ((int16_t)(low) | ((int16_t)(high) << 8))

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// UART1 receive buffer (DMA will fill this)
uint8_t rxBuffer[SENSOR_BUF_SIZE];

// Processed sensor packet
SensorPacket_t sensorPacket;

// Processed sensor data
uint16_t distance = 0;          // Distance in cm
uint16_t signalStrength = 0;    // Signal strength
int16_t temperature = 0;        // Temperature

// Buffer management
volatile uint8_t dmaCompleteFlag = 0;  // Flag to indicate DMA completion
uint16_t bytesReceived = 0;            // Number of bytes received

// Connection test variables
uint32_t lastDataReceivedTime = 0;     // Timestamp of last received data
uint8_t sensorConnected = 0;            // Sensor connection status (0=not connected, 1=connected)
uint32_t ledBlinkTime = 0;             // LED blink timing
uint8_t ledState = 0;                  // Current LED state

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// Function prototypes
static uint8_t FindPacketHeader(uint8_t *buffer, uint16_t bufferSize, uint16_t *packetIndex);
static uint8_t ValidatePacket(uint8_t *packet);
static uint8_t CalculateCRC(uint8_t *packet, uint8_t length);
static void ProcessSensorPacket(uint8_t *packet);
static void SendDataToPC(void);
static void SendRawDataToPC(uint8_t *data, uint16_t length);
static void UpdateConnectionStatus(void);
static void UpdateLEDStatus(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  Find packet header in buffer
  * @param  buffer: Pointer to receive buffer
  * @param  bufferSize: Size of buffer
  * @param  packetIndex: Output parameter for packet start index
  * @retval 1 if header found, 0 otherwise
  */
static uint8_t FindPacketHeader(uint8_t *buffer, uint16_t bufferSize, uint16_t *packetIndex)
{
    for (uint16_t i = 0; i < bufferSize - 1; i++)
    {
        if (buffer[i] == SENSOR_HEADER_BYTE1 && buffer[i + 1] == SENSOR_HEADER_BYTE2)
        {
            *packetIndex = i;
            return 1;
        }
    }
    return 0;
}

/**
  * @brief  Calculate CRC checksum for packet validation
  * @param  packet: Pointer to packet data
  * @param  length: Length of data to calculate CRC for
  * @retval Calculated CRC value
  */
static uint8_t CalculateCRC(uint8_t *packet, uint8_t length)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        crc += packet[i];
    }
    return crc;
}

/**
  * @brief  Validate sensor packet
  * @param  packet: Pointer to 9-byte packet
  * @retval 1 if valid, 0 otherwise
  */
static uint8_t ValidatePacket(uint8_t *packet)
{
    // Check header
    if (packet[0] != SENSOR_HEADER_BYTE1 || packet[1] != SENSOR_HEADER_BYTE2)
    {
        return 0;
    }

    // Calculate and verify CRC (CRC is sum of first 8 bytes)
    uint8_t calculatedCRC = CalculateCRC(packet, 8);
    if (calculatedCRC != packet[8])
    {
        return 0;
    }

    return 1;
}

/**
  * @brief  Process valid sensor packet and extract data
  * @param  packet: Pointer to validated 9-byte packet
  * @retval None
  */
static void ProcessSensorPacket(uint8_t *packet)
{
    // Extract distance
    distance = CALCULATE_DISTANCE(packet[2], packet[3]);

    // Extract signal strength
    signalStrength = CALCULATE_STRENGTH(packet[4], packet[5]);

    // Extract temperature
    temperature = CALCULATE_TEMPERATURE(packet[6], packet[7]);

    // Validate distance range (typical LiDAR sensors have max range)
    if (distance > MAX_DISTANCE_CM)
    {
        distance = 0;  // Invalid reading
    }
}

/**
  * @brief  Send processed sensor data to PC via UART2
  * @retval None
  */
static void SendDataToPC(void)
{
    char txBuffer[64];
    int len;

    // Format: "D:XXX,S:XXXX,T:XXX\n"
    len = snprintf(txBuffer, sizeof(txBuffer),
                   "D:%03d,S:%04d,T:%03d\n",
                   distance, signalStrength, temperature);

    if (len > 0 && len < sizeof(txBuffer))
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)txBuffer, len, UART_TIMEOUT_MS);
    }
}

/**
  * @brief  Send raw sensor data in hex format to PC (for testing)
  * @param  data: Pointer to data buffer
  * @param  length: Length of data
  * @retval None
  */
static void SendRawDataToPC(uint8_t *data, uint16_t length)
{
    char txBuffer[128];
    int pos = 0;

    // Format: "RAW[XX XX XX ...]\n"
    pos = snprintf(txBuffer, sizeof(txBuffer), "RAW[");

    for (uint16_t i = 0; i < length && pos < (sizeof(txBuffer) - 10); i++)
    {
        pos += snprintf(&txBuffer[pos], sizeof(txBuffer) - pos, "%02X ", data[i]);
    }

    if (pos < sizeof(txBuffer) - 2)
    {
        txBuffer[pos++] = ']';
        txBuffer[pos++] = '\n';
        txBuffer[pos] = '\0';

        HAL_UART_Transmit(&huart2, (uint8_t*)txBuffer, pos, UART_TIMEOUT_MS);
    }
}

/**
  * @brief  Update sensor connection status based on data reception
  * @retval None
  */
static void UpdateConnectionStatus(void)
{
    uint32_t currentTime = HAL_GetTick();

    // Check if data was received recently
    if (dmaCompleteFlag)
    {
        lastDataReceivedTime = currentTime;
        if (!sensorConnected)
        {
            sensorConnected = 1;
            // Send connection status to PC
            const char *connMsg = ">>> SENSOR CONNECTED! <<<\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)connMsg, strlen(connMsg), UART_TIMEOUT_MS);
        }
    }
    else
    {
        // Check timeout
        if (sensorConnected && (currentTime - lastDataReceivedTime) > CONNECTION_TIMEOUT_MS)
        {
            sensorConnected = 0;
            // Send disconnection status to PC
            const char *disconnMsg = ">>> SENSOR DISCONNECTED! <<<\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)disconnMsg, strlen(disconnMsg), UART_TIMEOUT_MS);
        }
    }
}

/**
  * @brief  Update LED status based on sensor connection
  * @note   LED pin configuration for Nucleo-F446RE:
  *         LED2 (Green): PA5 - User LED on board
  *         LED3 (Red): PB14 or PB0 - User LED on board
  *
  *         To enable LED:
  *         1. Configure PA5 as GPIO_Output in CubeMX
  *         2. Uncomment the HAL_GPIO_WritePin line below
  *         3. Or use your custom LED pin configuration
  * @retval None
  */
static void UpdateLEDStatus(void)
{
    uint32_t currentTime = HAL_GetTick();

    // Toggle LED based on connection status
    if ((currentTime - ledBlinkTime) >= LED_BLINK_INTERVAL_MS)
    {
        ledBlinkTime = currentTime;
        ledState = !ledState;

        // LED2 (Green) on PA5 - Uncomment if configured in CubeMX
        // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, ledState ? GPIO_PIN_SET : GPIO_PIN_RESET);

        // Alternative: LED3 (Red) on PB14 - Uncomment if configured in CubeMX
        // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, ledState ? GPIO_PIN_SET : GPIO_PIN_RESET);

        // Note: LED will blink fast when connected, slow when disconnected
        // You can modify LED_BLINK_INTERVAL_MS for different blink speeds
    }
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
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  // Initialize sensor data
  distance = 0;
  signalStrength = 0;
  temperature = 0;
  dmaCompleteFlag = 0;
  sensorConnected = 0;
  lastDataReceivedTime = HAL_GetTick();
  ledBlinkTime = HAL_GetTick();
  ledState = 0;

  // Send startup message to PC
  #if TEST_MODE
  const char *startMsg = "========================================\n"
                          "STM32 SEN0885 LiDAR Connection Test\n"
                          "========================================\n"
                          "Waiting for sensor data...\n"
                          "If sensor is connected, you should see:\n"
                          "  - RAW data packets\n"
                          "  - Header detection messages\n"
                          "  - LED blinking when data received\n"
                          "========================================\n";
  #else
  const char *startMsg = "STM32 Sensor Ready\n";
  #endif
  HAL_UART_Transmit(&huart2, (uint8_t*)startMsg, strlen(startMsg), UART_TIMEOUT_MS);

  // Start DMA reception for UART1 (sensor data)
  HAL_UART_Receive_DMA(&huart1, rxBuffer, SENSOR_BUF_SIZE);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // Update connection status
    UpdateConnectionStatus();

    // Update LED status
    UpdateLEDStatus();

    // Check if DMA transfer is complete
    if (dmaCompleteFlag)
    {
        // Reset flag
        dmaCompleteFlag = 0;

        // Update last data received time
        lastDataReceivedTime = HAL_GetTick();

        #if TEST_MODE
        // TEST MODE: Send raw data in hex format
        SendRawDataToPC(rxBuffer, SENSOR_BUF_SIZE);

        // Also check for header and send info
        uint16_t packetIndex = 0;
        if (FindPacketHeader(rxBuffer, SENSOR_BUF_SIZE, &packetIndex))
        {
            char infoMsg[64];
            int len = snprintf(infoMsg, sizeof(infoMsg),
                              "[OK] Header found at index: %d\n", packetIndex);
            HAL_UART_Transmit(&huart2, (uint8_t*)infoMsg, len, UART_TIMEOUT_MS);

            // Try to validate packet
            if (packetIndex + SENSOR_PACKET_SIZE <= SENSOR_BUF_SIZE)
            {
                if (ValidatePacket(&rxBuffer[packetIndex]))
                {
                    ProcessSensorPacket(&rxBuffer[packetIndex]);
                    char validMsg[80];
                    len = snprintf(validMsg, sizeof(validMsg),
                                   "[VALID] Distance:%dcm Strength:%d Temp:%d\n",
                                   distance, signalStrength, temperature);
                    HAL_UART_Transmit(&huart2, (uint8_t*)validMsg, len, UART_TIMEOUT_MS);
                }
                else
                {
                    const char *invalidMsg = "[ERROR] Invalid CRC - Check wiring!\n";
                    HAL_UART_Transmit(&huart2, (uint8_t*)invalidMsg, strlen(invalidMsg), UART_TIMEOUT_MS);
                }
            }
        }
        else
        {
            const char *noHeaderMsg = "[WARNING] No header found - Check baud rate!\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)noHeaderMsg, strlen(noHeaderMsg), UART_TIMEOUT_MS);
        }
        #else
        // NORMAL MODE: Process packets normally
        // Find packet header in buffer
        uint16_t packetIndex = 0;
        if (FindPacketHeader(rxBuffer, SENSOR_BUF_SIZE, &packetIndex))
        {
            // Check if we have a complete packet
            if (packetIndex + SENSOR_PACKET_SIZE <= SENSOR_BUF_SIZE)
            {
                // Validate packet
                if (ValidatePacket(&rxBuffer[packetIndex]))
                {
                    // Process valid packet
                    ProcessSensorPacket(&rxBuffer[packetIndex]);

                    // Send data to PC
                    SendDataToPC();
                }
                else
                {
                    // Invalid packet - send error marker
                    uint8_t errorMarker = 'E';
                    HAL_UART_Transmit(&huart2, &errorMarker, 1, UART_TIMEOUT_MS);
                }
            }
        }
        else
        {
            // Header not found - send error marker
            uint8_t errorMarker = 'H';
            HAL_UART_Transmit(&huart2, &errorMarker, 1, UART_TIMEOUT_MS);
        }
        #endif

        // Restart DMA reception
        HAL_UART_Receive_DMA(&huart1, rxBuffer, SENSOR_BUF_SIZE);
    }

    // Check for timeout - send periodic status if no data
    uint32_t currentTime = HAL_GetTick();
    static uint32_t lastStatusTime = 0;
    if ((currentTime - lastStatusTime) > 5000) // Every 5 seconds
    {
        lastStatusTime = currentTime;
        if (!sensorConnected)
        {
            char statusMsg[64];
            int len = snprintf(statusMsg, sizeof(statusMsg),
                              "[STATUS] No sensor data for %lu ms - Check connections!\n",
                              currentTime - lastDataReceivedTime);
            HAL_UART_Transmit(&huart2, (uint8_t*)statusMsg, len, UART_TIMEOUT_MS);
        }
    }

    // Small delay to prevent CPU spinning
    HAL_Delay(1);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief  UART receive complete callback (called when DMA transfer completes)
  * @param  huart: UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // Set flag to indicate DMA completion
        // Main loop will process the data
        dmaCompleteFlag = 1;
    }
}

/**
  * @brief  UART error callback (called on UART errors)
  * @param  huart: UART handle
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // Restart DMA reception on error
        HAL_UART_Receive_DMA(&huart1, rxBuffer, SENSOR_BUF_SIZE);
    }
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

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source name file
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
