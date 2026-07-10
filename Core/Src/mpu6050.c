#include "mpu6050.h"
#include "i2c.h"

#define MPU6050_ADDR          (0x68U << 1)
#define MPU6050_REG_SMPLRT    0x19U
#define MPU6050_REG_CONFIG    0x1AU
#define MPU6050_REG_GYRO_CFG  0x1BU
#define MPU6050_REG_ACCEL_CFG 0x1CU
#define MPU6050_REG_DATA      0x3BU
#define MPU6050_REG_PWR_MGMT1 0x6BU
#define MPU6050_REG_WHO_AM_I  0x75U

static uint8_t MPU6050_WriteReg(uint8_t reg, uint8_t value)
{
  return HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100) == HAL_OK;
}

static uint8_t MPU6050_ReadRegs(uint8_t reg, uint8_t *buffer, uint16_t length)
{
  return HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buffer, length, 100) == HAL_OK;
}

uint8_t MPU6050_ReadWhoAmI(uint8_t *id)
{
  if (id == 0) {
    return 0;
  }

  return MPU6050_ReadRegs(MPU6050_REG_WHO_AM_I, id, 1);
}

uint8_t MPU6050_Init(void)
{
  uint8_t id = 0;

  if (!MPU6050_ReadWhoAmI(&id)) {
    return 0;
  }

  if (id != 0x68U) {
    return 0;
  }

  if (!MPU6050_WriteReg(MPU6050_REG_PWR_MGMT1, 0x00U)) {
    return 0;
  }

  HAL_Delay(50);

  if (!MPU6050_WriteReg(MPU6050_REG_SMPLRT, 0x07U)) {
    return 0;
  }
  if (!MPU6050_WriteReg(MPU6050_REG_CONFIG, 0x06U)) {
    return 0;
  }
  if (!MPU6050_WriteReg(MPU6050_REG_GYRO_CFG, 0x00U)) {
    return 0;
  }
  if (!MPU6050_WriteReg(MPU6050_REG_ACCEL_CFG, 0x00U)) {
    return 0;
  }

  return 1;
}

uint8_t MPU6050_ReadData(MPU6050_Data *data)
{
  uint8_t buffer[14];

  if (data == 0) {
    return 0;
  }

  if (!MPU6050_ReadRegs(MPU6050_REG_DATA, buffer, sizeof(buffer))) {
    return 0;
  }

  data->ax = (int16_t)((buffer[0] << 8) | buffer[1]);
  data->ay = (int16_t)((buffer[2] << 8) | buffer[3]);
  data->az = (int16_t)((buffer[4] << 8) | buffer[5]);
  data->temp = (int16_t)((buffer[6] << 8) | buffer[7]);
  data->gx = (int16_t)((buffer[8] << 8) | buffer[9]);
  data->gy = (int16_t)((buffer[10] << 8) | buffer[11]);
  data->gz = (int16_t)((buffer[12] << 8) | buffer[13]);

  return 1;
}
