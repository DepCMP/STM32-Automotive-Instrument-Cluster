#include "hcsr04.h"


extern void DWT_Delay_us(uint32_t us);



uint16_t HCSR04_ReadDistance(void)
{
    // Phát xung Trigger 10us
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
    DWT_Delay_us(2);
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);
    DWT_Delay_us(10);
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);

    // Chờ Echo lên mức HIGH (bắt đầu đo), có timeout tránh treo
    uint32_t timeout = HAL_GetTick() + 30; // 30ms timeout
    while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_RESET)
    {
        if (HAL_GetTick() > timeout) return 0;
    }

    // Đo thời gian Echo ở mức HIGH
    uint32_t startTick = DWT->CYCCNT;
    timeout = HAL_GetTick() + 30;
    while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_SET)
    {
        if (HAL_GetTick() > timeout) return 0;
    }
    uint32_t endTick = DWT->CYCCNT;

    uint32_t pulseCycles = endTick - startTick;
    uint32_t pulseUs = pulseCycles / (SystemCoreClock / 1000000);

    // Công thức chuẩn: khoảng cách(cm) = thời gian(us) / 58
    uint16_t distance = pulseUs / 58;

    return distance;
}
