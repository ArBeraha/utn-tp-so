################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../code/cliente-servidor.c \
../code/commonTypes.c \
../code/header.c \
../code/hilos.c \
../code/log.c \
../code/serializacion.c \
../code/stack.c 

OBJS += \
./code/cliente-servidor.o \
./code/commonTypes.o \
./code/header.o \
./code/hilos.o \
./code/log.o \
./code/serializacion.o \
./code/stack.o 

C_DEPS += \
./code/cliente-servidor.d \
./code/commonTypes.d \
./code/header.d \
./code/hilos.d \
./code/log.d \
./code/serializacion.d \
./code/stack.d 


# Each subdirectory must supply rules for building sources it contributes
code/%.o: ../code/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


