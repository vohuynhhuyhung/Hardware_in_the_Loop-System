################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
../Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
../Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c 

OBJS += \
./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.o \
./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.o \
./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.o 

C_DEPS += \
./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.d \
./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.d \
./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/STM32_USB_Device_Library/Core/Src/%.o Middlewares/ST/STM32_USB_Device_Library/Core/Src/%.su Middlewares/ST/STM32_USB_Device_Library/Core/Src/%.cyclo: ../Middlewares/ST/STM32_USB_Device_Library/Core/Src/%.c Middlewares/ST/STM32_USB_Device_Library/Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -DUSE_FULL_LL_DRIVER -DARM_MATH_CM4 -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS/include" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS/portable/GCC/ARM_CM4F" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS/portable" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/ThirdParty/FreeRTOS" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/w25q_driver" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/CMDLine" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/UART" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/USB" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/sine_wave" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/can_driver" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/App/uart_driver" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/Middlewares/ST/ARM/DSP/Inc" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/App" -I"D:/HCMUT/DA2/STM32_Hardware_in_the_Loop/STM32F4_DUT/Middlewares/ST/ARM/DSP/Lib" -I../Middlewares/ST/ARM/DSP/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-STM32_USB_Device_Library-2f-Core-2f-Src

clean-Middlewares-2f-ST-2f-STM32_USB_Device_Library-2f-Core-2f-Src:
	-$(RM) ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.cyclo ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.d ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.o ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.su ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.cyclo ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.d ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.o ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.su ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.cyclo ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.d ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.o ./Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.su

.PHONY: clean-Middlewares-2f-ST-2f-STM32_USB_Device_Library-2f-Core-2f-Src

