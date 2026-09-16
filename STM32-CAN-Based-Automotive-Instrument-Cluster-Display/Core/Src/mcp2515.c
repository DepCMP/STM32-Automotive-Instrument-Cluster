#include "mcp2515.h"

// ==== Lệnh SPI của MCP2515 ====
#define CMD_RESET       0xC0
#define CMD_READ        0x03
#define CMD_WRITE       0x02
#define CMD_RTS         0x80  // Request To Send (gửi frame), cộng thêm 1/2/4 cho TXB0/1/2
#define CMD_READ_STATUS 0xA0
#define CMD_BIT_MODIFY  0x05

// ==== Địa chỉ thanh ghi ====
#define REG_CANCTRL   0x0F
#define REG_CANSTAT   0x0E
#define REG_CNF1      0x2A
#define REG_CNF2      0x29
#define REG_CNF3      0x28
#define REG_CANINTF   0x2C
#define REG_RXB0CTRL  0x60
#define REG_RXB0SIDH  0x61
#define REG_RXB0DLC   0x65
#define REG_RXB0D0    0x66
#define REG_TXB0CTRL  0x30
#define REG_TXB0SIDH  0x31
#define REG_TXB0DLC   0x35
#define REG_TXB0D0    0x36

#define MODE_NORMAL   0x00
#define MODE_CONFIG   0x80
#define MODE_MASK     0xE0

// ==== Hàm điều khiển CS ====
static void CS_Select(MCP2515_t *dev)   { HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET); }
static void CS_Deselect(MCP2515_t *dev) { HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET); }

// ==== Đọc/ghi 1 thanh ghi ====
static void MCP2515_WriteReg(MCP2515_t *dev, uint8_t addr, uint8_t data)
{
    uint8_t buf[3] = { CMD_WRITE, addr, data };
    CS_Select(dev);
    HAL_SPI_Transmit(dev->hspi, buf, 3, HAL_MAX_DELAY);
    CS_Deselect(dev);
}

uint8_t MCP2515_ReadReg(MCP2515_t *dev, uint8_t addr)
{
    uint8_t tx[2] = { CMD_READ, addr };
    uint8_t rx = 0;
    CS_Select(dev);
    HAL_SPI_Transmit(dev->hspi, tx, 2, HAL_MAX_DELAY);
    HAL_SPI_Receive(dev->hspi, &rx, 1, HAL_MAX_DELAY);
    CS_Deselect(dev);
    return rx;
}

static void MCP2515_Reset(MCP2515_t *dev)
{
    uint8_t cmd = CMD_RESET;
    CS_Select(dev);
    HAL_SPI_Transmit(dev->hspi, &cmd, 1, HAL_MAX_DELAY);
    CS_Deselect(dev);
    HAL_Delay(10);
}

// ==== Khởi tạo MCP2515: reset, cấu hình tốc độ 500kbps (thạch anh 8MHz), vào chế độ Normal ====
void MCP2515_Init(MCP2515_t *dev)
{
    MCP2515_Reset(dev);

    // Vào chế độ Config để chỉnh CNF1/2/3
    MCP2515_WriteReg(dev, REG_CANCTRL, MODE_CONFIG);
    HAL_Delay(10);

    // Bit timing chuẩn cho thạch anh 8MHz @ 125,kbps
    MCP2515_WriteReg(dev, REG_CNF1, 0x01);
    MCP2515_WriteReg(dev, REG_CNF2, 0xB8);
    MCP2515_WriteReg(dev, REG_CNF3, 0x05);

    // RXB0: nhận mọi ID, không lọc (RXM = 11)
    MCP2515_WriteReg(dev, REG_RXB0CTRL, 0x64);

    // Chuyển sang chế độ Normal để hoạt động thật
    MCP2515_WriteReg(dev, REG_CANCTRL, MODE_NORMAL);
    HAL_Delay(10);
}

// ==== Gửi 1 frame CAN (chỉ hỗ trợ Standard ID 11-bit, đủ dùng cho project này) ====
uint8_t MCP2515_SendFrame(MCP2515_t *dev, CAN_Frame_t *frame)
{
    MCP2515_WriteReg(dev, REG_TXB0SIDH, frame->id >> 3);
    MCP2515_WriteReg(dev, REG_TXB0SIDH + 1, (frame->id & 0x07) << 5); // TXB0SIDL

    MCP2515_WriteReg(dev, REG_TXB0DLC, frame->dlc & 0x0F);

    for (uint8_t i = 0; i < frame->dlc; i++)
        MCP2515_WriteReg(dev, REG_TXB0D0 + i, frame->data[i]);

    // Yêu cầu gửi (RTS cho TXB0)
    uint8_t cmd = CMD_RTS | 0x01;
    CS_Select(dev);
    HAL_SPI_Transmit(dev->hspi, &cmd, 1, HAL_MAX_DELAY);
    CS_Deselect(dev);

    return 1;
}

// ==== Kiểm tra & đọc frame mới nhận được (polling, chưa dùng ngắt INT) ====
uint8_t MCP2515_ReceiveFrame(MCP2515_t *dev, CAN_Frame_t *frame)
{
    uint8_t intf = MCP2515_ReadReg(dev, REG_CANINTF);

    if (!(intf & 0x01)) return 0; // bit0 = RX0IF, chưa có dữ liệu mới ở RXB0

    uint8_t sidh = MCP2515_ReadReg(dev, REG_RXB0SIDH);
    uint8_t sidl = MCP2515_ReadReg(dev, REG_RXB0SIDH + 1);
    frame->id = (sidh << 3) | (sidl >> 5);

    frame->dlc = MCP2515_ReadReg(dev, REG_RXB0DLC) & 0x0F;

    for (uint8_t i = 0; i < frame->dlc; i++)
        frame->data[i] = MCP2515_ReadReg(dev, REG_RXB0D0 + i);

    // Xoá cờ interrupt để sẵn sàng nhận frame tiếp theo
    MCP2515_WriteReg(dev, REG_CANINTF, intf & ~0x01);

    return 1;
}
