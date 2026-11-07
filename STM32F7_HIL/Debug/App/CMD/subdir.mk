################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/CMD/USB_console.c \
../App/CMD/cmd.c 

OBJS += \
./App/CMD/USB_console.o \
./App/CMD/cmd.o 

C_DEPS += \
./App/CMD/USB_console.d \
./App/CMD/cmd.d 


# Each subdirectory must supply rules for building sources it contributes
App/CMD/%.o App/CMD/%.su App/CMD/%.cyclo: ../App/CMD/%.c App/CMD/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/EmuDrivers/Inc" -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/include" -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/App/CMDLine" -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/App/CMD" -I"D:/STUDY/00_SCHOOL/DA/HIL/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1" -O0 -ffunction-sections -fdata-sections -Wall -mfpu=fpv5-d16 -mfloat-abi=hard -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-CMD

clean-App-2f-CMD:
	-$(RM) ./App/CMD/USB_console.cyclo ./App/CMD/USB_console.d ./App/CMD/USB_console.o ./App/CMD/USB_console.su ./App/CMD/cmd.cyclo ./App/CMD/cmd.d ./App/CMD/cmd.o ./App/CMD/cmd.su

.PHONY: clean-App-2f-CMD

