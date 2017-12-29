################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../cJSON/cJSON.c 

OBJS += \
./cJSON/cJSON.o 

C_DEPS += \
./cJSON/cJSON.d 


# Each subdirectory must supply rules for building sources it contributes
cJSON/%.o: ../cJSON/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-hisiv500-linux-uclibcgnueabi-gcc -DNO_CRYPTO -I"/home/lichen/eclipse-workspace/videolib/cJSON" -I"/home/lichen/eclipse-workspace/videolib/iotMqtt" -I"/home/lichen/eclipse-workspace/videolib/iotMqtt/client" -I"/home/lichen/eclipse-workspace/videolib/iotMqtt/libmqtt" -I"/home/lichen/eclipse-workspace/videolib/iotRtmp" -I"/home/lichen/eclipse-workspace/videolib/misc" -I"/home/lichen/eclipse-workspace/videolib/src" -I"/home/lichen/eclipse-workspace/videolib/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


