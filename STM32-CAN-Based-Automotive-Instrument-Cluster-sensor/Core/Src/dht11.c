#include "dht11.h"


extern void DWT_Delay_us(uint32_t us);

// Chuyển chân DHT11 sang Output
static void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_Pin_GPIO_Port, &GPIO_InitStruct);
}

// Chuyển chân DHT11 sang Input (để đọc phản hồi)
static void DHT11_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_Pin_GPIO_Port, &GPIO_InitStruct);
}

uint8_t DHT11_Read(int8_t *temperature, uint8_t *humidity)
{
    uint8_t data[5] = {0, 0, 0, 0, 0};

    // ---- Bước 1: Gửi tín hiệu Start ----
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_Pin_GPIO_Port, DHT11_Pin_Pin, GPIO_PIN_RESET);
    HAL_Delay(18); // giữ LOW tối thiểu 18ms
    HAL_GPIO_WritePin(DHT11_Pin_GPIO_Port, DHT11_Pin_Pin, GPIO_PIN_SET);
    DWT_Delay_us(30);
    DHT11_SetInput();

    // ---- Bước 2: Chờ DHT11 phản hồi (kéo LOW 80us, rồi HIGH 80us) ----
    uint32_t timeout = DWT->CYCCNT + (SystemCoreClock / 1000) * 1; // ~1ms timeout tính bằng cycle
    while (HAL_GPIO_ReadPin(DHT11_Pin_GPIO_Port, DHT11_Pin_Pin) == GPIO_PIN_SET)
        if (DWT->CYCCNT > timeout) return 0;

    timeout = DWT->CYCCNT + (SystemCoreClock / 1000) * 1;
    while (HAL_GPIO_ReadPin(DHT11_Pin_GPIO_Port, DHT11_Pin_Pin) == GPIO_PIN_RESET)
        if (DWT->CYCCNT > timeout) return 0;

    timeout = DWT->CYCCNT + (SystemCoreClock / 1000) * 1;
    while (HAL_GPIO_ReadPin(DHT11_Pin_GPIO_Port, DHT11_Pin_Pin) == GPIO_PIN_SET)
        if (DWT->CYCCNT > timeout) return 0;

    // ---- Bước 3: Đọc 40 bit dữ liệu (5 byte) ----
    for (uint8_t i = 0; i < 40; i++)
    {
        // Mỗi bit bắt đầu bằng 50us LOW
        timeout = DWT->CYCCNT + (SystemCoreClock / 1000) * 1;
        while (HAL_GPIO_ReadPin(DHT11_Pin_GPIO_Port, DHT11_Pin_Pin) == GPIO_PIN_RESET)
            if (DWT->CYCCNT > timeout) return 0;

        // Đo độ dài xung HIGH để phân biệt bit 0 (~26-28us) hay bit 1 (~70us)
        uint32_t startCycle = DWT->CYCCNT;
        timeout = DWT->CYCCNT + (SystemCoreClock / 1000) * 1;
        while (HAL_GPIO_ReadPin(DHT11_Pin_GPIO_Port, DHT11_Pin_Pin) == GPIO_PIN_SET)
            if (DWT->CYCCNT > timeout) return 0;
        uint32_t pulseLen = (DWT->CYCCNT - startCycle) / (SystemCoreClock / 1000000);

        data[i / 8] <<= 1;
        if (pulseLen > 40) // > 40us -> bit 1
            data[i / 8] |= 1;
    }

    // ---- Bước 4: Kiểm tra checksum ----
    if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3]))
        return 0; // checksum sai -> đọc lỗi

    *humidity = data[0];
    *temperature = (int8_t)data[2];

    return 1; // đọc thành công
}
