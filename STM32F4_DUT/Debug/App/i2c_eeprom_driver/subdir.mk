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
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DUSE_FULL_LL_DRIVER -DARM_MATH_CM4 -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS/include" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS/portable" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/w25q_driver" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/CMDLine" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/UART" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/i2c_eeprom_driver" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/USB" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/sine_wave" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/can_driver" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/uart_driver" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/Middlewares/ST/ARM/DSP/Inc" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/App" -I"D:/HCMUT/HK252/LVTN/STM32_Hardware_in_the_Loop/STM32F4_DUT/Middlewares/ST/ARM/DSP/Lib" -I../Middlewares/ST/ARM/DSP/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-i2c_eeprom_driver

clean-App-2f-i2c_eeprom_driver:
	-$(RM) ./App/i2c_eeprom_driver/i2c_eeprom.cyclo ./App/i2c_eeprom_driver/i2c_eeprom.d ./App/i2c_eeprom_driver/i2c_eeprom.o ./App/i2c_eeprom_driver/i2c_eeprom.su

.PHONY: clean-App-2f-i2c_eeprom_driver

