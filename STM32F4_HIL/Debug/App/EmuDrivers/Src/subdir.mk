################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/EmuDrivers/Src/analog.c \
../App/EmuDrivers/Src/can_driver.c \
../App/EmuDrivers/Src/gpio_control.c \
../App/EmuDrivers/Src/i2c_device.c \
../App/EmuDrivers/Src/i2c_eeprom.c \
../App/EmuDrivers/Src/i2c_rtc.c \
../App/EmuDrivers/Src/pwm_control.c \
../App/EmuDrivers/Src/uart_driver.c \
../App/EmuDrivers/Src/w25q_slave.c 

OBJS += \
./App/EmuDrivers/Src/analog.o \
./App/EmuDrivers/Src/can_driver.o \
./App/EmuDrivers/Src/gpio_control.o \
./App/EmuDrivers/Src/i2c_device.o \
./App/EmuDrivers/Src/i2c_eeprom.o \
./App/EmuDrivers/Src/i2c_rtc.o \
./App/EmuDrivers/Src/pwm_control.o \
./App/EmuDrivers/Src/uart_driver.o \
./App/EmuDrivers/Src/w25q_slave.o 

C_DEPS += \
./App/EmuDrivers/Src/analog.d \
./App/EmuDrivers/Src/can_driver.d \
./App/EmuDrivers/Src/gpio_control.d \
./App/EmuDrivers/Src/i2c_device.d \
./App/EmuDrivers/Src/i2c_eeprom.d \
./App/EmuDrivers/Src/i2c_rtc.d \
./App/EmuDrivers/Src/pwm_control.d \
./App/EmuDrivers/Src/uart_driver.d \
./App/EmuDrivers/Src/w25q_slave.d 


# Each subdirectory must supply rules for building sources it contributes
App/EmuDrivers/Src/%.o App/EmuDrivers/Src/%.su App/EmuDrivers/Src/%.cyclo: ../App/EmuDrivers/Src/%.c App/EmuDrivers/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS/include" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS/portable" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/App/CMDLine/Core" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/App/EmuDrivers/Inc" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-EmuDrivers-2f-Src

clean-App-2f-EmuDrivers-2f-Src:
	-$(RM) ./App/EmuDrivers/Src/analog.cyclo ./App/EmuDrivers/Src/analog.d ./App/EmuDrivers/Src/analog.o ./App/EmuDrivers/Src/analog.su ./App/EmuDrivers/Src/can_driver.cyclo ./App/EmuDrivers/Src/can_driver.d ./App/EmuDrivers/Src/can_driver.o ./App/EmuDrivers/Src/can_driver.su ./App/EmuDrivers/Src/gpio_control.cyclo ./App/EmuDrivers/Src/gpio_control.d ./App/EmuDrivers/Src/gpio_control.o ./App/EmuDrivers/Src/gpio_control.su ./App/EmuDrivers/Src/i2c_device.cyclo ./App/EmuDrivers/Src/i2c_device.d ./App/EmuDrivers/Src/i2c_device.o ./App/EmuDrivers/Src/i2c_device.su ./App/EmuDrivers/Src/i2c_eeprom.cyclo ./App/EmuDrivers/Src/i2c_eeprom.d ./App/EmuDrivers/Src/i2c_eeprom.o ./App/EmuDrivers/Src/i2c_eeprom.su ./App/EmuDrivers/Src/i2c_rtc.cyclo ./App/EmuDrivers/Src/i2c_rtc.d ./App/EmuDrivers/Src/i2c_rtc.o ./App/EmuDrivers/Src/i2c_rtc.su ./App/EmuDrivers/Src/pwm_control.cyclo ./App/EmuDrivers/Src/pwm_control.d ./App/EmuDrivers/Src/pwm_control.o ./App/EmuDrivers/Src/pwm_control.su ./App/EmuDrivers/Src/uart_driver.cyclo ./App/EmuDrivers/Src/uart_driver.d ./App/EmuDrivers/Src/uart_driver.o ./App/EmuDrivers/Src/uart_driver.su ./App/EmuDrivers/Src/w25q_slave.cyclo ./App/EmuDrivers/Src/w25q_slave.d ./App/EmuDrivers/Src/w25q_slave.o ./App/EmuDrivers/Src/w25q_slave.su

.PHONY: clean-App-2f-EmuDrivers-2f-Src

