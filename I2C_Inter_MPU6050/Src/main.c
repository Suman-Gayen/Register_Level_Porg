
#include "RCC_Config.h"
#include "I2C.h"
#include "delay.h"


/**** The process of writing the data to any I2C Device is mentioned below.  ************
1. START the I2C
2. Send the ADDRESS of the Device
3. Send the ADDRESS of the Register, where you want to write the data to
4. Send the DATA
5. STOP the I2C
*/
void MPU_Write (uint8_t Address, uint8_t Reg, uint8_t Data)
{
	I2C_Start ();
	I2C_Address (Address);
	I2C_Write (Reg);
	I2C_Write (Data);
	I2C_Stop ();
}
/**** Read Data from Slave Device  ************
1. START the I2C
2. Send the ADDRESS of the Device
3. Send the ADDRESS of the Register, where you want to READ the data from
4. Send the RESTART condition
5. Send the Address (READ) of the device
6. Read the data
7. STOP the I2C
*/
void MPU_Read (uint8_t Address, uint8_t Reg, uint8_t *buffer, uint8_t size)
{
	I2C_Start ();
	I2C_Address (Address);
	I2C_Write (Reg);
	I2C_Start ();  // repeated start
	I2C_Read (Address+0x01, buffer, size);
	I2C_Stop ();
}


#define MPU6050_ADDR 0xD0

#define SMPLRT_DIV_REG 0x19
#define GYRO_CONFIG_REG 0X1B
#define ACCEL_CONFIG_REG 0X1C
#define ACCEL_XOUT_H_REG 0X3B
#define TEMP_OUT_H_REG 0X41
#define GYRO_XOUT_H_REG 0X43
#define PWR_MGHT_1_REG 0X6B
#define WHO_AM_I_REG 0X75

int16_t Accel_X_RAW = 0;
int16_t Accel_Y_RAW = 0;
int16_t Accel_Z_RAW = 0;

int16_t Gyro_X_RAW = 0;
int16_t Gyro_Y_RAW = 0;
int16_t Gyro_Z_RAW = 0;

float Ax, Ay, Az, Gx, Gy, Gz;


uint8_t check;

void MPU6050_Init(void){
	uint8_t check;
	uint8_t Data;

	// check device ID WHO_AM_I
	MPU_Read(MPU6050_ADDR, WHO_AM_I_REG, &check, 1);

	if(check==104) // 0x68 will be returned by the sensor if everything goes well
	{
		// power management register 0x6B we should write all 0's to wake the sensor up
		Data = 0;
		MPU_Write(MPU6050_ADDR, PWR_MGHT_1_REG, Data);

		// set data rate to 1kHZ by writing SMPLRT_DIV Register
		Data = 0x07;
		MPU_Write(MPU6050_ADDR, SMPLRT_DIV_REG, Data);

		// Set accelerometer configuration in ACCEL_CONFIG register
		// XA_ST=0, YA_ST=0, ZA_ST=0, FS_SEL=0 -> ? 2g
		Data = 0x00;
		MPU_Write(MPU6050_ADDR, ACCEL_CONFIG_REG, Data);

		// Set Gyroscopic configuration in GYROL_CONFIG register
		// XA_ST=0, YA_ST=0, ZA_ST=0, FS_SEL=0 -> ? 250g ?/s
		Data = 0x00;
		MPU_Write(MPU6050_ADDR, GYRO_CONFIG_REG, Data);
	}
}

void MPU6050_Read_Accel (void){
	uint8_t Rx_Data[6];

	// Read 6 bytes of data starting from ACCEL_XOUT_H register

	MPU_Read(MPU6050_ADDR, ACCEL_XOUT_H_REG, Rx_Data, 6);

	Accel_X_RAW = (int16_t) (Rx_Data[0] << 8 | Rx_Data[1]);
	Accel_Y_RAW = (int16_t) (Rx_Data[2] << 8 | Rx_Data[3]);
	Accel_Z_RAW = (int16_t) (Rx_Data[4] << 8 | Rx_Data[5]);

	/*    Convert the RAW values into acceleration in 'g'
	      we have to define according to the full scale value set in FS_SEl
	      I have configured FS_SEl = 0. So I'm dividing by 16384.0
	      for more details check ACCEL_CONFIG register
	*/

	Ax = Accel_X_RAW/16384.0;
	Ay = Accel_Y_RAW/16384.0;
	Az = Accel_Z_RAW/16384.0;
}

int main(void)
{
	SysClockConfig();
	TIM6Config();
	I2C_Config();

	MPU6050_Init();

	while(1){
		MPU6050_Read_Accel();
		delay_ms(1000);
	}

}
