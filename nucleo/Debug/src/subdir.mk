################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/nucleo.c \
../src/planificacion.c \
../src/primitivas.c \
../src/procesos.c \
../src/test.c 

OBJS += \
./src/nucleo.o \
./src/planificacion.o \
./src/primitivas.o \
./src/procesos.o \
./src/test.o 

C_DEPS += \
./src/nucleo.d \
./src/planificacion.d \
./src/primitivas.d \
./src/procesos.d \
./src/test.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../../compartido/code -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


