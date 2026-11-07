################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/UART/uart.c 

OBJS += \
./App/UART/uart.o 

C_DEPS += \
./App/UART/uart.d 


# Each subdirectory must supply rules for building sources it contributes
App/UART/%.o App/UART/%.su App/UART/%.cyclo: ../App/UART/%.c App/UART/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS/include" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS/portable" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/USB" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/w25q_driver" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/CMDLine" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/UART" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/i2c_eeprom_driver" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-UART

clean-App-2f-UART:
	-$(RM) ./App/UART/uart.cyclo ./App/UART/uart.d ./App/UART/uart.o ./App/UART/uart.su

.PHONY: clean-App-2f-UART

