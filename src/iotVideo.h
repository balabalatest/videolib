/*
 * iotVideo.h
 *
 *  Created on: Dec 20, 2017
 *      Author: root
 */

#ifndef IOTVIDEO_H_
#define IOTVIDEO_H_

struct deviceInfo
{
  char* cameraName;
  char* clientid;
  char* username;
  char* password;
};

#define HUAWEI_LITEOS

#ifndef HUAWEI_LITEOS
HI_S32 iotQueue_PutData(VENC_STREAM_S *pstStream);
#else
HI_S32 iotQueue_PutData(VENC_STREAM_BUF_INFO_S *pstStreamBuf, VENC_STREAM_S *pstStream);
#endif

int func_MqttGetCmd(uint8_t cmdbuf[]);
int func_RtmpScheduleStop(void);
int func_RtmpSchedule(int cmdid, uint8_t cmdbuf[]);

int iotMqtt_StartTask(struct deviceInfo *device);
int iotMqtt_Connect(const char *clientid, const char *username, const char *password, int txBufsize, int rxBufsize);
int iotMqtt_Disconnect(void);
int iotMqtt_isConnect(void);

int queue_full(void);
int queue_empty(void);
void queue_flush(void);

int queue_init(int size);
int queue_deinit(void);
int queue_send(uint8_t *data, int dataLen);
int queue_recv(uint8_t *buf, int bufLen);


#endif /* IOTVIDEO_H_ */
