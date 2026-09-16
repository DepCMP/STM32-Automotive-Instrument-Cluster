#ifndef DHT11_H
#define DHT11_H
#include "main.h"

// Trả về 1 nếu đọc thành công, 0 nếu lỗi (checksum sai/timeout)
uint8_t DHT11_Read(int8_t *temperature, uint8_t *humidity);

#endif
