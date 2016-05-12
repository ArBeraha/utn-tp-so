################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/cliente-servidor.c \
/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/header.c \
/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/log.c \
/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/serializacion.c \
/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/stack.c 

OBJS += \
./code/cliente-servidor.o \
./code/header.o \
./code/log.o \
./code/serializacion.o \
./code/stack.o 

C_DEPS += \
./code/cliente-servidor.d \
./code/header.d \
./code/log.d \
./code/serializacion.d \
./code/stack.d 


# Each subdirectory must supply rules for building sources it contributes
code/cliente-servidor.o: /home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/cliente-servidor.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

code/header.o: /home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/header.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

code/log.o: /home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/log.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

code/serializacion.o: /home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/serializacion.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

code/stack.o: /home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code/stack.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2016-1c-Con-16-bits-me-hago-alto-kernel/compartido/code" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


