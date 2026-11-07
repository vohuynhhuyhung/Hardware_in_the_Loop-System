################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/FreeRTOS-Kernel/croutine.c \
../FreeRTOS/FreeRTOS-Kernel/event_groups.c \
../FreeRTOS/FreeRTOS-Kernel/list.c \
../FreeRTOS/FreeRTOS-Kernel/queue.c \
../FreeRTOS/FreeRTOS-Kernel/stream_buffer.c \
../FreeRTOS/FreeRTOS-Kernel/tasks.c \
../FreeRTOS/FreeRTOS-Kernel/timers.c 

OBJS += \
./FreeRTOS/FreeRTOS-Kernel/croutine.o \
./FreeRTOS/FreeRTOS-Kernel/event_groups.o \
./FreeRTOS/FreeRTOS-Kernel/list.o \
./FreeRTOS/FreeRTOS-Kernel/queue.o \
./FreeRTOS/FreeRTOS-Kernel/stream_buffer.o \
./FreeRTOS/FreeRTOS-Kernel/tasks.o \
./FreeRTOS/FreeRTOS-Kernel/timers.o 

C_DEPS += \
./FreeRTOS/FreeRTOS-Kernel/croutine.d \
./FreeRTOS/FreeRTOS-Kernel/event_groups.d \
./FreeRTOS/FreeRTOS-Kernel/list.d \
./FreeRTOS/FreeRTOS-Kernel/queue.d \
./FreeRTOS/FreeRTOS-Kernel/stream_buffer.d \
./FreeRTOS/FreeRTOS-Kernel/tasks.d \
./FreeRTOS/FreeRTOS-Kernel/timers.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/FreeRTOS-Kernel/%.o FreeRTOS/FreeRTOS-Kernel/%.su FreeRTOS/FreeRTOS-Kernel/%.cyclo: ../FreeRTOS/FreeRTOS-Kernel/%.c FreeRTOS/FreeRTOS-Kernel/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/include" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM7/r0p1" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/App/CMDLine/Core" -I"D:/STUDY/00_SCHOOL/DA/0_Project/STM32F7_HIL/App/EmuDrivers/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-FreeRTOS-2f-FreeRTOS-2d-Kernel

clean-FreeRTOS-2f-FreeRTOS-2d-Kernel:
	-$(RM) ./FreeRTOS/FreeRTOS-Kernel/croutine.cyclo ./FreeRTOS/FreeRTOS-Kernel/croutine.d ./FreeRTOS/FreeRTOS-Kernel/croutine.o ./FreeRTOS/FreeRTOS-Kernel/croutine.su ./FreeRTOS/FreeRTOS-Kernel/event_groups.cyclo ./FreeRTOS/FreeRTOS-Kernel/event_groups.d ./FreeRTOS/FreeRTOS-Kernel/event_groups.o ./FreeRTOS/FreeRTOS-Kernel/event_groups.su ./FreeRTOS/FreeRTOS-Kernel/list.cyclo ./FreeRTOS/FreeRTOS-Kernel/list.d ./FreeRTOS/FreeRTOS-Kernel/list.o ./FreeRTOS/FreeRTOS-Kernel/list.su ./FreeRTOS/FreeRTOS-Kernel/queue.cyclo ./FreeRTOS/FreeRTOS-Kernel/queue.d ./FreeRTOS/FreeRTOS-Kernel/queue.o ./FreeRTOS/FreeRTOS-Kernel/queue.su ./FreeRTOS/FreeRTOS-Kernel/stream_buffer.cyclo ./FreeRTOS/FreeRTOS-Kernel/stream_buffer.d ./FreeRTOS/FreeRTOS-Kernel/stream_buffer.o ./FreeRTOS/FreeRTOS-Kernel/stream_buffer.su ./FreeRTOS/FreeRTOS-Kernel/tasks.cyclo ./FreeRTOS/FreeRTOS-Kernel/tasks.d ./FreeRTOS/FreeRTOS-Kernel/tasks.o ./FreeRTOS/FreeRTOS-Kernel/tasks.su ./FreeRTOS/FreeRTOS-Kernel/timers.cyclo ./FreeRTOS/FreeRTOS-Kernel/timers.d ./FreeRTOS/FreeRTOS-Kernel/timers.o ./FreeRTOS/FreeRTOS-Kernel/timers.su

.PHONY: clean-FreeRTOS-2f-FreeRTOS-2d-Kernel

