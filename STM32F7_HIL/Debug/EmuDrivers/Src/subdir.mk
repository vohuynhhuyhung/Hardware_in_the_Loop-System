################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../EmuDrivers/Src/Emutask.c \
../EmuDrivers/Src/i2c_eeprom.c 

OBJS += \
./EmuDrivers/Src/Emutask.o \
./EmuDrivers/Src/i2c_eeprom.o 

C_DEPS += \
./EmuDrivers/Src/Emutask.d \
./EmuDrivers/Src/i2c_eeprom.d 


# Each subdirectory must supply rules for building sources it contributes
EmuDrivers/Src/%.o EmuDrivers/Src/%.su EmuDrivers/Src/%.cyclo: ../EmuDrivers/Src/%.c EmuDrivers/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/EmuDrivers/Inc" -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/include" -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/App/CMDLine" -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1" -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/App/CMDLine/Core" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-EmuDrivers-2f-Src

clean-EmuDrivers-2f-Src:
	-$(RM) ./EmuDrivers/Src/Emutask.cyclo ./EmuDrivers/Src/Emutask.d ./EmuDrivers/Src/Emutask.o ./EmuDrivers/Src/Emutask.su ./EmuDrivers/Src/i2c_eeprom.cyclo ./EmuDrivers/Src/i2c_eeprom.d ./EmuDrivers/Src/i2c_eeprom.o ./EmuDrivers/Src/i2c_eeprom.su

.PHONY: clean-EmuDrivers-2f-Src

