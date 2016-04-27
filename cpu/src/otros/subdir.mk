################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/otros/commonTypes.c \
../src/otros/handshake.c \
../src/otros/header.c \
../src/otros/log.c 

OBJS += \
./src/otros/commonTypes.o \
./src/otros/handshake.o \
./src/otros/header.o \
./src/otros/log.o 

C_DEPS += \
./src/otros/commonTypes.d \
./src/otros/handshake.d \
./src/otros/header.d \
./src/otros/log.d 


# Each subdirectory must supply rules for building sources it contributes
src/otros/%.o: ../src/otros/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


