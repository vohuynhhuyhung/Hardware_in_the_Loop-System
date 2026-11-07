################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/CMDLine/Core/cmd.c \
../App/CMDLine/Core/cmdline.c 

OBJS += \
./App/CMDLine/Core/cmd.o \
./App/CMDLine/Core/cmdline.o 

C_DEPS += \
./App/CMDLine/Core/cmd.d \
./App/CMDLine/Core/cmdline.d 


# Each subdirectory must supply rules for building sources it contributes
App/CMDLine/Core/%.o App/CMDLine/Core/%.su App/CMDLine/Core/%.cyclo: ../App/CMDLine/Core/%.c App/CMDLine/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/include" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/App/CMDLine/Core" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/App/EmuDrivers/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-CMDLine-2f-Core

clean-App-2f-CMDLine-2f-Core:
	-$(RM) ./App/CMDLine/Core/cmd.cyclo ./App/CMDLine/Core/cmd.d ./App/CMDLine/Core/cmd.o ./App/CMDLine/Core/cmd.su ./App/CMDLine/Core/cmdline.cyclo ./App/CMDLine/Core/cmdline.d ./App/CMDLine/Core/cmdline.o ./App/CMDLine/Core/cmdline.su

.PHONY: clean-App-2f-CMDLine-2f-Core

