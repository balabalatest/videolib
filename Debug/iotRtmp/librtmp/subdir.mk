################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../iotRtmp/librtmp/amf.c \
../iotRtmp/librtmp/log.c \
../iotRtmp/librtmp/parseurl.c \
../iotRtmp/librtmp/rtmp.c 

OBJS += \
./iotRtmp/librtmp/amf.o \
./iotRtmp/librtmp/log.o \
./iotRtmp/librtmp/parseurl.o \
./iotRtmp/librtmp/rtmp.o 

C_DEPS += \
./iotRtmp/librtmp/amf.d \
./iotRtmp/librtmp/log.d \
./iotRtmp/librtmp/parseurl.d \
./iotRtmp/librtmp/rtmp.d 


# Each subdirectory must supply rules for building sources it contributes
iotRtmp/librtmp/%.o: ../iotRtmp/librtmp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-hisiv500-linux-uclibcgnueabi-gcc -DNO_CRYPTO -I"/home/lichen/eclipse-workspace/videolib/cJSON" -I"/home/lichen/eclipse-workspace/videolib/iotMqtt" -I"/home/lichen/eclipse-workspace/videolib/iotMqtt/client" -I"/home/lichen/eclipse-workspace/videolib/iotMqtt/libmqtt" -I"/home/lichen/eclipse-workspace/videolib/iotRtmp" -I"/home/lichen/eclipse-workspace/videolib/misc" -I"/home/lichen/eclipse-workspace/videolib/src" -I"/home/lichen/eclipse-workspace/videolib/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


