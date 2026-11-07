################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/i2c_eeprom_driver/i2c_eeprom.c 

OBJS += \
./App/i2c_eeprom_driver/i2c_eeprom.o 

C_DEPS += \
./App/i2c_eeprom_driver/i2c_eeprom.d 


# Each subdirectory must supply rules for building sources it contributes
App/i2c_eeprom_driver/%.o App/i2c_eeprom_driver/%.su App/i2c_eeprom_driver/%.cyclo: ../App/i2c_eeprom_driver/%.c App/i2c_eeprom_driver/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -DUSE_FULL_LL_DRIVER -c -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F1_DUT/ThirdParty/FreeRTOS" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F1_DUT/ThirdParty/FreeRTOS/include" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F1_DUT/ThirdParty/FreeRTOS/portable/GCC/ARM_CM3" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F1_DUT/App/CMDLine" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F1_DUT/App/i2c_eeprom_driver" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F1_DUT/App/UART" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F1_DUT/App/USB" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F1_DUT/App/w25q_driver" -I../Core/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-App-2f-i2c_eeprom_driver

clean-App-2f-i2c_eeprom_driver:
	-$(RM) ./App/i2c_eeprom_driver/i2c_eeprom.cyclo ./App/i2c_eeprom_driver/i2c_eeprom.d ./App/i2c_eeprom_driver/i2c_eeprom.o ./App/i2c_eeprom_driver/i2c_eeprom.su

.PHONY: clean-App-2f-i2c_eeprom_driver

