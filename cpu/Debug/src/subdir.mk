################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/auxiliaresDePrimitivas.c \
../src/cpu.c \
../src/primitivas.c \
../src/test.c 

OBJS += \
./src/auxiliaresDePrimitivas.o \
./src/cpu.o \
./src/primitivas.o \
./src/test.o 

C_DEPS += \
./src/auxiliaresDePrimitivas.d \
./src/cpu.d \
./src/primitivas.d \
./src/test.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


