################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
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
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS/include" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS/portable" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/ThirdParty/FreeRTOS" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/App/CMDLine/Core" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_HIL/App/EmuDrivers/Inc" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-CMDLine-2f-Core

clean-App-2f-CMDLine-2f-Core:
	-$(RM) ./App/CMDLine/Core/cmd.cyclo ./App/CMDLine/Core/cmd.d ./App/CMDLine/Core/cmd.o ./App/CMDLine/Core/cmd.su ./App/CMDLine/Core/cmdline.cyclo ./App/CMDLine/Core/cmdline.d ./App/CMDLine/Core/cmdline.o ./App/CMDLine/Core/cmdline.su

.PHONY: clean-App-2f-CMDLine-2f-Core

