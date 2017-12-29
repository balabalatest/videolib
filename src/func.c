/*
 * func.c
 *
 *  Created on: Dec 4, 2017
 *      Author: lichen
 */

#include "queue.h"
#include "cJSON.h"
#include "iotRtmp.h"
#include "iotMqtt.h"

#define HUAWEI_LITEOS

#ifndef HUAWEI_LITEOS
#define SAMPLE_VENC_CLASSIC_STOP  SAMPLE_VENC_1080P_CLASSIC_STOP
#define SAMPLE_VENC_CLASSIC       SAMPLE_VENC_1080P_CLASSIC
#endif

int func_MqttGetCmd(uint8_t cmdbuf[])
{
  int cmdid = 0;
  char* message;

  message = iotMqtt_GetMessage();

  if (message) {

    cJSON *json = cJSON_Parse(message);

    if (json) {

      cJSON *type = cJSON_GetObjectItem(json, "type");
      if (type && !strcmp(type->valuestring, "video")) {
        cJSON *jcmdid = cJSON_GetObjectItem(json, "cmdId");
        cJSON *jcmd   = cJSON_GetObjectItem(json, "cmd");

        if (jcmdid && jcmd) {
//printf("Platform cmdid: %d\n", jcmdid->valueint);
          switch (jcmdid->valueint) {

          case 1: {
            cJSON *jurl = cJSON_GetObjectItem(jcmd,  "pushUrl");
            if (jurl) {
              strcpy(cmdbuf, jurl->valuestring);
              cmdid = jcmdid->valueint;
            }
          } break;

          case 6: {
            cJSON *jlevel = cJSON_GetObjectItem(jcmd,  "level");
            if (jlevel) {
              sprintf(cmdbuf, "%d", jlevel->valueint);
              cmdid = jcmdid->valueint;
            }
          } break;

          default:
            break;
          }
        }
      }
    }

    cJSON_Delete(json);
  }

  return cmdid;
}


int g_rtmpState = 0;
static pthread_t pid;

#define QUEUE_BUFSIZE  (160*1024)
extern void SAMPLE_VENC_CLASSIC(void *arg);
uint8_t bufRecv[QUEUE_BUFSIZE] = {0};
uint8_t bufSend[QUEUE_BUFSIZE] = {0};


int func_RtmpScheduleStop(void)
{
  if (g_rtmpState == 2 || g_rtmpState == 3) {
    SAMPLE_VENC_CLASSIC_STOP();
    pthread_join(pid, 0);
    g_rtmpState = 1;
  }
  if (g_rtmpState == 1) {
    iotRtmp_Disconnect();
    queue_deinit();
    g_rtmpState = 0;
  }

  return g_rtmpState;
}

int func_RtmpSchedule(int cmdid, uint8_t cmdbuf[])
{
  int rc = 0;
  static int mode = 0;
  static uint32_t ts = 0;

  switch (g_rtmpState) {

  /* unconnect */
  case 0: {
    if (cmdid == 1) {
      if (!queue_init(QUEUE_BUFSIZE))
        break;
      if (!iotRtmp_Connect(cmdbuf, 10)) {
        queue_deinit();
        break;
      }
      g_rtmpState = 1;
    }
  } break;

  case 1: {
    int width, height, framerate = 30, bitrate = 2048;
    if (mode == 0) {
      width = 1280; height = 720;
    } else if (mode == 1) {
      width = 640; height = 480;
    } else if (mode == 2) {
      width = 320; height = 240; bitrate = 512;
    }

    if (!queue_empty()) queue_flush();
    pthread_create(&pid, NULL, SAMPLE_VENC_CLASSIC, (void*)&mode);

retry:
    while (queue_empty()) usleep(33000);

    rc = iotRtmp_SendMetadata(width, height, framerate, bitrate);
    if (rc == FALSE) {
      printf("iotRtmp_SendMetadata failed!!!\n");
    }

    int len = queue_recv(bufRecv, QUEUE_BUFSIZE);
    if (len) {

      ts = RTMP_GetTime();
      rc = iotRtmp_SendFirstFrame(bufRecv, len, 0);
      if (rc == 0)
        goto retry;
      if (rc == -1) {
        printf("iotRtmp_SendFirstFrame failed!!!\n");
        g_rtmpState = 3;
      } else if (rc == TRUE) {
        printf("iotRtmp_SendFirstFrame OK!\n");
      }
    } else {
      printf("queue_recv len invalid!!!\n");
    }

    g_rtmpState = 2;

  } break;

  /* connected, sending rtmp packet */
  case 2: {

    if (cmdid == 0) {

      int len = queue_recv(bufRecv, QUEUE_BUFSIZE);
      if (len) {
uint32_t t1 = RTMP_GetTime();
//        rc = iotRtmp_SendH264Packet(bufRecv, len, ts+=33);  /* will make VLC error: Gray Screen */
        rc = iotRtmp_SendH264Packet(bufRecv, len, RTMP_GetTime()-ts);
uint32_t t2 = RTMP_GetTime();
if (t2-t1 > 500)
  printf("Send time: %u\n", t2-t1);

        if (rc == FALSE) {
          printf("iotRtmp_SendH264Packet failed!!!\n");
          g_rtmpState = 3;
        }
      }
    } else if (cmdid == 1) {
      /* TODO */
      printf("Get RTMP Reconnect while Playing!!!\n");
    } else if (cmdid == 6) {

#if 1
      int level = cmdbuf[0] - '0';
      if (level == 1) mode = 2;
      else if (level == 2) mode = 1;
      else if (level >= 3) mode = 0;

      SAMPLE_VENC_CLASSIC_STOP();
      pthread_join(pid, 0);
      g_rtmpState = 1;
#endif
    }

  } break;

  /* disconnect */
  case 3: {

    SAMPLE_VENC_CLASSIC_STOP();
    pthread_join(pid, 0);

    iotRtmp_Disconnect();
    queue_deinit();
    g_rtmpState = 0;
  } break;

  default:
    break;
  }

  return 0;
}

#include "hi_comm_venc.h"

#ifndef HUAWEI_LITEOS

HI_S32 iotQueue_PutData(VENC_STREAM_S *pstStream)
{
  int i, len;
  uint8_t *buf = bufSend;

  if (queue_full()) {
    printf("Queue is Full!\n");
    return 0;
  }

  for (i = 0, len = 0; i < pstStream->u32PackCount; i++) {
    len += pstStream->pstPack[i].u32Len-pstStream->pstPack[i].u32Offset;
  }
  if ((len <= 0) || (len > QUEUE_BUFSIZE)) {
    printf("Queue the data is invalid: %d !!!\n", len);
    return 0;
  }

  for (i = 0; i < pstStream->u32PackCount; i++) {
    memcpy(buf, pstStream->pstPack[i].pu8Addr+pstStream->pstPack[i].u32Offset, pstStream->pstPack[i].u32Len-pstStream->pstPack[i].u32Offset);
    buf += pstStream->pstPack[i].u32Len-pstStream->pstPack[i].u32Offset;
  }

  queue_send(bufSend, len);

  return 0;
}
#else
HI_S32 iotQueue_PutData(VENC_STREAM_BUF_INFO_S *pstStreamBuf, VENC_STREAM_S *pstStream)
{

  int i, len = 0;
  uint8_t *buf = bufSend;

  if (queue_full()) {
    printf("Queue is Full!\n");
    return 0;
  }

    HI_U32 u32SrcPhyAddr;
    HI_U32 u32Left;

    for(i=0; i<pstStream->u32PackCount; i++)
    {
        if (pstStream->pstPack[i].u32PhyAddr + pstStream->pstPack[i].u32Len >=
                pstStreamBuf->u32PhyAddr + pstStreamBuf->u32BufSize)
        {
            if (pstStream->pstPack[i].u32PhyAddr + pstStream->pstPack[i].u32Offset >=
                pstStreamBuf->u32PhyAddr + pstStreamBuf->u32BufSize)
            {
                /* physical address retrace in offset segment */
                u32SrcPhyAddr = pstStreamBuf->u32PhyAddr +
                                ((pstStream->pstPack[i].u32PhyAddr + pstStream->pstPack[i].u32Offset) -
                                (pstStreamBuf->u32PhyAddr + pstStreamBuf->u32BufSize));

                len += pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset;
                if (len > QUEUE_BUFSIZE) {
                  printf("Queue the data is invalid: %d !!!\n", len);
                  return HI_SUCCESS;
                }
                memcpy(buf, (void*)u32SrcPhyAddr, pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset);
                buf += pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset;
            }
            else
            {
                /* physical address retrace in data segment */
                u32Left = (pstStreamBuf->u32PhyAddr + pstStreamBuf->u32BufSize) - pstStream->pstPack[i].u32PhyAddr;

                len += u32Left - pstStream->pstPack[i].u32Offset;
                if (len > QUEUE_BUFSIZE) {
                  printf("Queue the data is invalid: %d !!!\n", len);
                  return HI_SUCCESS;
                }
                memcpy(buf, (void*)pstStream->pstPack[i].u32PhyAddr + pstStream->pstPack[i].u32Offset, u32Left - pstStream->pstPack[i].u32Offset);
                buf += u32Left - pstStream->pstPack[i].u32Offset;

                len += pstStream->pstPack[i].u32Len - u32Left;
                if (len > QUEUE_BUFSIZE) {
                  printf("Queue the data is invalid: %d !!!\n", len);
                  return HI_SUCCESS;
                }
                memcpy(buf, (void*)pstStreamBuf->u32PhyAddr, pstStream->pstPack[i].u32Len - u32Left);
                buf += pstStream->pstPack[i].u32Len - u32Left;
            }
        }
        else
        {
            /* physical address retrace does not happen */
            len += pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset;
            if (len > QUEUE_BUFSIZE) {
              printf("Queue the data is invalid: %d !!!\n", len);
              return HI_SUCCESS;
            }
            memcpy(buf, (void*)pstStream->pstPack[i].u32PhyAddr + pstStream->pstPack[i].u32Offset, pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset);
            buf += pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset;
        }
    }

    if (len > 0) queue_send(bufSend, len);
    return HI_SUCCESS;
}
#endif

