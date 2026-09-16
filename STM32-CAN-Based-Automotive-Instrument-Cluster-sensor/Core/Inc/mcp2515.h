#ifndef MCP2515_H
#define MCP2515_H

#include "main.h"
#define REG_CANSTAT   0x0E
#define REG_TXB0CTRL   0x30

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} MCP2515_t;

typedef struct {
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[8];
} CAN_Frame_t;

void    MCP2515_Init(MCP2515_t *dev);
uint8_t MCP2515_SendFrame(MCP2515_t *dev, CAN_Frame_t *frame);
uint8_t MCP2515_ReceiveFrame(MCP2515_t *dev, CAN_Frame_t *frame); // trả 1 nếu có frame mới
uint8_t MCP2515_ReadReg(MCP2515_t *dev, uint8_t addr);
static uint8_t MCP2515_WaitTxReady(MCP2515_t *dev);
#endif
