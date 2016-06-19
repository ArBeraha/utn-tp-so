################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/algoritmosReemplazo.c \
../src/auxiliaresUmc.c \
../src/conexionesUmc.c \
../src/consolaUmc.c \
../src/umc.c 

OBJS += \
./src/algoritmosReemplazo.o \
./src/auxiliaresUmc.o \
./src/conexionesUmc.o \
./src/consolaUmc.o \
./src/umc.o 

C_DEPS += \
./src/algoritmosReemplazo.d \
./src/auxiliaresUmc.d \
./src/conexionesUmc.d \
./src/consolaUmc.d \
./src/umc.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../../compartido/code -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


