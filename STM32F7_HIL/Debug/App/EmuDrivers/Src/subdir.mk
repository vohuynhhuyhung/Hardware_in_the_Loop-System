################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/EmuDrivers/Src/i2c_device.c \
../App/EmuDrivers/Src/i2c_eeprom.c \
../App/EmuDrivers/Src/i2c_rtc.c \
../App/EmuDrivers/Src/w25q_slave.c 

OBJS += \
./App/EmuDrivers/Src/i2c_device.o \
./App/EmuDrivers/Src/i2c_eeprom.o \
./App/EmuDrivers/Src/i2c_rtc.o \
./App/EmuDrivers/Src/w25q_slave.o 

C_DEPS += \
./App/EmuDrivers/Src/i2c_device.d \
./App/EmuDrivers/Src/i2c_eeprom.d \
./App/EmuDrivers/Src/i2c_rtc.d \
./App/EmuDrivers/Src/w25q_slave.d 


# Each subdirectory must supply rules for building sources it contributes
App/EmuDrivers/Src/%.o App/EmuDrivers/Src/%.su App/EmuDrivers/Src/%.cyclo: ../App/EmuDrivers/Src/%.c App/EmuDrivers/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/include" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/App/CMDLine/Core" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/App/EmuDrivers/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-EmuDrivers-2f-Src

clean-App-2f-EmuDrivers-2f-Src:
	-$(RM) ./App/EmuDrivers/Src/i2c_device.cyclo ./App/EmuDrivers/Src/i2c_device.d ./App/EmuDrivers/Src/i2c_device.o ./App/EmuDrivers/Src/i2c_device.su ./App/EmuDrivers/Src/i2c_eeprom.cyclo ./App/EmuDrivers/Src/i2c_eeprom.d ./App/EmuDrivers/Src/i2c_eeprom.o ./App/EmuDrivers/Src/i2c_eeprom.su ./App/EmuDrivers/Src/i2c_rtc.cyclo ./App/EmuDrivers/Src/i2c_rtc.d ./App/EmuDrivers/Src/i2c_rtc.o ./App/EmuDrivers/Src/i2c_rtc.su ./App/EmuDrivers/Src/w25q_slave.cyclo ./App/EmuDrivers/Src/w25q_slave.d ./App/EmuDrivers/Src/w25q_slave.o ./App/EmuDrivers/Src/w25q_slave.su

.PHONY: clean-App-2f-EmuDrivers-2f-Src

