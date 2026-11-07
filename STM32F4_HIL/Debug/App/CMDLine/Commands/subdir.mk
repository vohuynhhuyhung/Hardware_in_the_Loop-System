################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/CMDLine/Commands/analog_cmd.c \
../App/CMDLine/Commands/can_cmd.c \
../App/CMDLine/Commands/dut_control_cmd.c \
../App/CMDLine/Commands/gpio_cmd.c \
../App/CMDLine/Commands/i2c_cmd.c \
../App/CMDLine/Commands/pwm_cmd.c \
../App/CMDLine/Commands/spi_cmd.c \
../App/CMDLine/Commands/uart_cmd.c 

OBJS += \
./App/CMDLine/Commands/analog_cmd.o \
./App/CMDLine/Commands/can_cmd.o \
./App/CMDLine/Commands/dut_control_cmd.o \
./App/CMDLine/Commands/gpio_cmd.o \
./App/CMDLine/Commands/i2c_cmd.o \
./App/CMDLine/Commands/pwm_cmd.o \
./App/CMDLine/Commands/spi_cmd.o \
./App/CMDLine/Commands/uart_cmd.o 

C_DEPS += \
./App/CMDLine/Commands/analog_cmd.d \
./App/CMDLine/Commands/can_cmd.d \
./App/CMDLine/Commands/dut_control_cmd.d \
./App/CMDLine/Commands/gpio_cmd.d \
./App/CMDLine/Commands/i2c_cmd.d \
./App/CMDLine/Commands/pwm_cmd.d \
./App/CMDLine/Commands/spi_cmd.d \
./App/CMDLine/Commands/uart_cmd.d 


# Each subdirectory must supply rules for building sources it contributes
App/CMDLine/Commands/%.o App/CMDLine/Commands/%.su App/CMDLine/Commands/%.cyclo: ../App/CMDLine/Commands/%.c App/CMDLine/Commands/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS/include" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS/portable" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/App/CMDLine/Core" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/App/EmuDrivers/Inc" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-CMDLine-2f-Commands

clean-App-2f-CMDLine-2f-Commands:
	-$(RM) ./App/CMDLine/Commands/analog_cmd.cyclo ./App/CMDLine/Commands/analog_cmd.d ./App/CMDLine/Commands/analog_cmd.o ./App/CMDLine/Commands/analog_cmd.su ./App/CMDLine/Commands/can_cmd.cyclo ./App/CMDLine/Commands/can_cmd.d ./App/CMDLine/Commands/can_cmd.o ./App/CMDLine/Commands/can_cmd.su ./App/CMDLine/Commands/dut_control_cmd.cyclo ./App/CMDLine/Commands/dut_control_cmd.d ./App/CMDLine/Commands/dut_control_cmd.o ./App/CMDLine/Commands/dut_control_cmd.su ./App/CMDLine/Commands/gpio_cmd.cyclo ./App/CMDLine/Commands/gpio_cmd.d ./App/CMDLine/Commands/gpio_cmd.o ./App/CMDLine/Commands/gpio_cmd.su ./App/CMDLine/Commands/i2c_cmd.cyclo ./App/CMDLine/Commands/i2c_cmd.d ./App/CMDLine/Commands/i2c_cmd.o ./App/CMDLine/Commands/i2c_cmd.su ./App/CMDLine/Commands/pwm_cmd.cyclo ./App/CMDLine/Commands/pwm_cmd.d ./App/CMDLine/Commands/pwm_cmd.o ./App/CMDLine/Commands/pwm_cmd.su ./App/CMDLine/Commands/spi_cmd.cyclo ./App/CMDLine/Commands/spi_cmd.d ./App/CMDLine/Commands/spi_cmd.o ./App/CMDLine/Commands/spi_cmd.su ./App/CMDLine/Commands/uart_cmd.cyclo ./App/CMDLine/Commands/uart_cmd.d ./App/CMDLine/Commands/uart_cmd.o ./App/CMDLine/Commands/uart_cmd.su

.PHONY: clean-App-2f-CMDLine-2f-Commands

