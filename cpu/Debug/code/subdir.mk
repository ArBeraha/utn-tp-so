################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../../compartido/code/cliente-servidor.c \
../../compartido/code/commonTypes.c \
../../compartido/code/header.c \
../../compartido/code/log.c \
../../compartido/code/serializacion.c \
../../compartido/code/stack.c 

OBJS += \
./code/cliente-servidor.o \
./code/commonTypes.o \
./code/header.o \
./code/log.o \
./code/serializacion.o \
./code/stack.o 

C_DEPS += \
./code/cliente-servidor.d \
./code/commonTypes.d \
./code/header.d \
./code/log.d \
./code/serializacion.d \
./code/stack.d 


# Each subdirectory must supply rules for building sources it contributes
code/cliente-servidor.o: ../../compartido/code/cliente-servidor.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../../compartido/code -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

code/commonTypes.o: ../../compartido/code/commonTypes.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../../compartido/code -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

code/header.o: ../../compartido/code/header.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../../compartido/code -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

code/log.o: ../../compartido/code/log.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../../compartido/code -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

code/serializacion.o: ../../compartido/code/serializacion.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../../compartido/code -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

code/stack.o: ../../compartido/code/stack.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I../../compartido/code -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


