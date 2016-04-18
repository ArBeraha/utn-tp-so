################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/otros/sockets/cliente-servidor.c 

OBJS += \
./src/otros/sockets/cliente-servidor.o 

C_DEPS += \
./src/otros/sockets/cliente-servidor.d 


# Each subdirectory must supply rules for building sources it contributes
src/otros/sockets/%.o: ../src/otros/sockets/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


