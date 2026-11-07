################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/USB/usb.c 

OBJS += \
./App/USB/usb.o 

C_DEPS += \
./App/USB/usb.d 


# Each subdirectory must supply rules for building sources it contributes
App/USB/%.o App/USB/%.su App/USB/%.cyclo: ../App/USB/%.c App/USB/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -DUSE_FULL_LL_DRIVER -c -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F1_DUT_v2/ThirdParty/FreeRTOS" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F1_DUT_v2/ThirdParty/FreeRTOS/include" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F1_DUT_v2/ThirdParty/FreeRTOS/portable/GCC/ARM_CM3" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F1_DUT_v2/App/CMDLine" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F1_DUT_v2/App/i2c_eeprom_driver" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F1_DUT_v2/App/UART" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F1_DUT_v2/App/USB" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F1_DUT_v2/App/w25q_driver" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-App-2f-USB

clean-App-2f-USB:
	-$(RM) ./App/USB/usb.cyclo ./App/USB/usb.d ./App/USB/usb.o ./App/USB/usb.su

.PHONY: clean-App-2f-USB

