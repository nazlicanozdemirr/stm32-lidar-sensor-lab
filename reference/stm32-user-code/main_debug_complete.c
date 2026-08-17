/* ============================================
 * STM32 DEBUG KODU - BAŞTAN SONA
 * Bu kodları mevcut main.c dosyasının USER CODE bölümlerine ekle
 * ============================================ */

/* ============================================
 * 1. USER CODE BEGIN Includes bölümüne ekle:
 * ============================================ */
#include <stdio.h>
#include <string.h>


/* ============================================
 * 2. USER CODE BEGIN PV bölümüne ekle:
 * ============================================ */
// UART1'den gelen veri buffer'ı
#define UART1_RX_BUFFER_SIZE 256
uint8_t uart1_rx_buffer[UART1_RX_BUFFER_SIZE];
uint16_t uart1_rx_index = 0;

// Ölçüm sayacı (hız testi için)
volatile uint32_t measurement_count = 0;

// Sensör verisi (mesafe değeri)
uint16_t lidar_distance = 0;

// Debug için sayaçlar
uint32_t total_bytes_received = 0;
uint32_t last_debug_time = 0;


/* ============================================
 * 3. USER CODE BEGIN 0 bölümüne ekle:
 * ============================================ */
/**
  * @brief  UART1'den gelen veriyi işle (LiDAR sensöründen)
  * @note   Debug modunda tüm paketleri gösterir
  */
void Process_LiDAR_Data(uint8_t *data, uint16_t length)
{
  if (length == 0) return;
  
  // DEBUG: Gelen paketi göster
  char debug_packet[256];
  sprintf(debug_packet, "PAKET[%d]: ", length);
  HAL_UART_Transmit(&huart2, (uint8_t*)debug_packet, strlen(debug_packet), HAL_MAX_DELAY);
  
  // Paketi HEX formatında göster
  for (int i = 0; i < length && i < 32; i++)
  {
    char hex[8];
    sprintf(hex, "%02X ", data[i]);
    HAL_UART_Transmit(&huart2, (uint8_t*)hex, strlen(hex), HAL_MAX_DELAY);
  }
  
  // ASCII karakterleri göster (eğer varsa)
  HAL_UART_Transmit(&huart2, (uint8_t*)" | ASCII: ", 11, HAL_MAX_DELAY);
  for (int i = 0; i < length && i < 32; i++)
  {
    if (data[i] >= 32 && data[i] <= 126)
    {
      HAL_UART_Transmit(&huart2, &data[i], 1, HAL_MAX_DELAY);
    }
    else
    {
      HAL_UART_Transmit(&huart2, (uint8_t*)".", 1, HAL_MAX_DELAY);
    }
  }
  HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
  
  // Ölçüm sayacını artır
  measurement_count++;
  
  // Paket formatını analiz et ve mesafe değerini çıkar
  // ÖRNEK 1: Binary paket formatı (9 byte, başlık 0x59 0x59)
  if (length >= 9 && data[0] == 0x59 && data[1] == 0x59)
  {
    lidar_distance = (uint16_t)(data[2] | (data[3] << 8));
    
    char output[64];
    sprintf(output, "MESAFE: %u cm\r\n", lidar_distance);
    HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
  }
  // ÖRNEK 2: ASCII format (satır satır, "123\r\n" gibi)
  else if (length > 0 && length < 20)
  {
    // String olarak parse et
    data[length] = '\0';
    int distance = atoi((char*)data);
    if (distance > 0)
    {
      lidar_distance = (uint16_t)distance;
      
      char output[64];
      sprintf(output, "MESAFE: %u cm\r\n", lidar_distance);
      HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
    }
  }
  // ÖRNEK 3: İlk 2 byte mesafe ise
  else if (length >= 2)
  {
    lidar_distance = (uint16_t)(data[0] | (data[1] << 8));
    
    char output[64];
    sprintf(output, "MESAFE (2-byte): %u cm\r\n", lidar_distance);
    HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
  }
  
  // Ölçüm verisini PC'ye gönder (Python scripti için)
  char output[64];
  sprintf(output, "%lu,%u\r\n", HAL_GetTick(), lidar_distance);
  HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
}


/* ============================================
 * 4. USER CODE BEGIN 2 bölümüne ekle:
 * (MX_USART2_UART_Init() çağrısından SONRA)
 * ============================================ */
// Başlangıç mesajı
const char *start_msg = "\r\n========================================\r\n";
HAL_UART_Transmit(&huart2, (uint8_t*)start_msg, strlen(start_msg), HAL_MAX_DELAY);
const char *start_msg2 = "LiDAR Debug Test Basladi\r\n";
HAL_UART_Transmit(&huart2, (uint8_t*)start_msg2, strlen(start_msg2), HAL_MAX_DELAY);
const char *start_msg3 = "UART1 interrupt aktif ediliyor...\r\n";
HAL_UART_Transmit(&huart2, (uint8_t*)start_msg3, strlen(start_msg3), HAL_MAX_DELAY);
const char *start_msg4 = "========================================\r\n\r\n";
HAL_UART_Transmit(&huart2, (uint8_t*)start_msg4, strlen(start_msg4), HAL_MAX_DELAY);

// UART1 interrupt'ı aktif et (LiDAR sensöründen veri okumak için)
HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer[uart1_rx_index], 1);

// Test mesajı
const char *test_msg = "Nucleo hazir, sensorden veri bekleniyor...\r\n";
HAL_UART_Transmit(&huart2, (uint8_t*)test_msg, strlen(test_msg), HAL_MAX_DELAY);


/* ============================================
 * 5. USER CODE BEGIN WHILE bölümüne ekle:
 * (while(1) döngüsü içine)
 * ============================================ */
// Her 1 saniyede bir ölçüm sayısını ve istatistikleri gönder
static uint32_t last_report = 0;
if (HAL_GetTick() - last_report >= 1000)
{
  char report[128];
  sprintf(report, "\r\n--- ISTATISTIKLER ---\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t*)report, strlen(report), HAL_MAX_DELAY);
  
  sprintf(report, "Toplam paket: %lu\r\n", measurement_count);
  HAL_UART_Transmit(&huart2, (uint8_t*)report, strlen(report), HAL_MAX_DELAY);
  
  sprintf(report, "Toplam byte: %lu\r\n", total_bytes_received);
  HAL_UART_Transmit(&huart2, (uint8_t*)report, strlen(report), HAL_MAX_DELAY);
  
  if (measurement_count > 0)
  {
    float hiz = (float)measurement_count;
    sprintf(report, "Olcum hizi: %.2f Hz\r\n", hiz);
    HAL_UART_Transmit(&huart2, (uint8_t*)report, strlen(report), HAL_MAX_DELAY);
  }
  else
  {
    sprintf(report, "UYARI: Henuz paket alinmadi!\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)report, strlen(report), HAL_MAX_DELAY);
  }
  
  sprintf(report, "----------------------\r\n\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t*)report, strlen(report), HAL_MAX_DELAY);
  
  last_report = HAL_GetTick();
}


/* ============================================
 * 6. USER CODE BEGIN 4 bölümüne ekle:
 * ============================================ */
/**
  * @brief  UART1 Rx Interrupt Callback
  * @note   LiDAR sensöründen gelen her byte burada işlenir
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    // Gelen byte'ı al
    uint8_t received_byte = uart1_rx_buffer[uart1_rx_index];
    uart1_rx_index++;
    total_bytes_received++;
    
    // DEBUG: Her byte'ı göster (ilk 100 byte için)
    if (total_bytes_received <= 100)
    {
      char debug_msg[64];
      sprintf(debug_msg, "RX[%lu]: 0x%02X ('%c')\r\n", 
              total_bytes_received, 
              received_byte,
              (received_byte >= 32 && received_byte <= 126) ? received_byte : '.');
      HAL_UART_Transmit(&huart2, (uint8_t*)debug_msg, strlen(debug_msg), HAL_MAX_DELAY);
    }
    
    // Paket tamamlanma kontrolü
    // Senaryo 1: '\n' karakteri ile bitiyorsa (ASCII format)
    if (received_byte == '\n' || received_byte == '\r')
    {
      // Eğer '\r\n' kombinasyonu ise, sadece '\n' geldiğinde işle
      if (received_byte == '\n')
      {
        // Paketi işle
        Process_LiDAR_Data(uart1_rx_buffer, uart1_rx_index);
        
        // Buffer'ı temizle
        uart1_rx_index = 0;
        memset(uart1_rx_buffer, 0, UART1_RX_BUFFER_SIZE);
      }
    }
    // Senaryo 2: Sabit uzunlukta paket (örn: 9 byte)
    else if (uart1_rx_index >= 9)
    {
      // Paketi işle
      Process_LiDAR_Data(uart1_rx_buffer, uart1_rx_index);
      
      // Buffer'ı temizle
      uart1_rx_index = 0;
      memset(uart1_rx_buffer, 0, UART1_RX_BUFFER_SIZE);
    }
    // Senaryo 3: Buffer dolduysa
    else if (uart1_rx_index >= UART1_RX_BUFFER_SIZE - 1)
    {
      // Paketi işle
      Process_LiDAR_Data(uart1_rx_buffer, uart1_rx_index);
      
      // Buffer'ı temizle
      uart1_rx_index = 0;
      memset(uart1_rx_buffer, 0, UART1_RX_BUFFER_SIZE);
    }
    
    // Sonraki byte'ı dinlemeye devam et
    HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer[uart1_rx_index], 1);
  }
}

/**
  * @brief  UART Error Callback
  * @note   Hata durumunda çağrılır
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    char error_msg[64];
    sprintf(error_msg, "UART1 HATA: 0x%08lX\r\n", huart->ErrorCode);
    HAL_UART_Transmit(&huart2, (uint8_t*)error_msg, strlen(error_msg), HAL_MAX_DELAY);
    
    // UART'ı yeniden başlat
    HAL_UART_AbortReceive_IT(&huart1);
    HAL_Delay(10);
    uart1_rx_index = 0;
    memset(uart1_rx_buffer, 0, UART1_RX_BUFFER_SIZE);
    HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer[uart1_rx_index], 1);
  }
}
