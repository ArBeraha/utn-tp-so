################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ssasadssad/nucleo.c 

OBJS += \
./ssasadssad/nucleo.o 

C_DEPS += \
./ssasadssad/nucleo.d 


# Each subdirectory must supply rules for building sources it contributes
ssasadssad/%.o: ../ssasadssad/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


