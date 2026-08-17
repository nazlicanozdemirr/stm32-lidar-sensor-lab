/* ============================================
 * SADECE USER CODE BÖLÜMLERİNE EKLENECEK KODLAR
 * Mevcut main.c dosyasının USER CODE bölümlerine kopyala-yapıştır yap
 * ============================================ */

/* ============================================
 * 1. USER CODE BEGIN Includes bölümüne ekle:
 * ============================================ */
#include <stdio.h>
#include <string.h>


/* ============================================
 * 2. USER CODE BEGIN PV bölümüne ekle:
 * ============================================ */
#define UART1_RX_BUFFER_SIZE 256
uint8_t uart1_rx_buffer[UART1_RX_BUFFER_SIZE];
uint16_t uart1_rx_index = 0;
volatile uint32_t measurement_count = 0;
uint16_t lidar_distance = 0;


/* ============================================
 * 3. USER CODE BEGIN 0 bölümüne ekle:
 * ============================================ */
void Process_LiDAR_Data(uint8_t *data, uint16_t length)
{
  // ÖRNEK: Binary paket formatı (9 byte, başlık 0x59 0x59)
  if (length >= 9)
  {
    if (data[0] == 0x59 && data[1] == 0x59)
    {
      lidar_distance = (uint16_t)(data[2] | (data[3] << 8));
      measurement_count++;
      
      char output[64];
      sprintf(output, "%lu,%u\r\n", HAL_GetTick(), lidar_distance);
      HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
    }
  }
  
  // ALTERNATIF: ASCII format (satır satır)
  // Yukarıdaki if bloğunu yorum satırı yap ve bunu kullan:
  /*
  if (length > 0)
  {
    data[length] = '\0';
    lidar_distance = atoi((char*)data);
    measurement_count++;
    
    char output[64];
    sprintf(output, "%lu,%u\r\n", HAL_GetTick(), lidar_distance);
    HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
  }
  */
}


/* ============================================
 * 4. USER CODE BEGIN 2 bölümüne ekle:
 * (MX_USART2_UART_Init() çağrısından SONRA)
 * ============================================ */
HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer[uart1_rx_index], 1);
const char *start_msg = "LiDAR Test Started\r\n";
HAL_UART_Transmit(&huart2, (uint8_t*)start_msg, strlen(start_msg), HAL_MAX_DELAY);


/* ============================================
 * 5. USER CODE BEGIN WHILE bölümüne ekle:
 * (while(1) döngüsü içine)
 * ============================================ */
static uint32_t last_report = 0;
if (HAL_GetTick() - last_report >= 1000)
{
  char report[64];
  sprintf(report, "COUNT:%lu\r\n", measurement_count);
  HAL_UART_Transmit(&huart2, (uint8_t*)report, strlen(report), HAL_MAX_DELAY);
  last_report = HAL_GetTick();
}


/* ============================================
 * 6. USER CODE BEGIN 4 bölümüne ekle:
 * ============================================ */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uart1_rx_index++;
    
    if (uart1_rx_buffer[uart1_rx_index - 1] == '\n' || uart1_rx_index >= UART1_RX_BUFFER_SIZE - 1)
    {
      Process_LiDAR_Data(uart1_rx_buffer, uart1_rx_index);
      uart1_rx_index = 0;
      memset(uart1_rx_buffer, 0, UART1_RX_BUFFER_SIZE);
    }
    
    HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer[uart1_rx_index], 1);
  }
}
