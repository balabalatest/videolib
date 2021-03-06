/*
 * queue.h
 *
 *  Created on: Nov 28, 2017
 *      Author: lichen
 */

#ifndef QUEUE_H_
#define QUEUE_H_


#define QUEUE_SIZE  24

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>


typedef struct {
  int bufLen;
  uint8_t *buf;
} mem_t;

struct queue {
  pthread_mutex_t lock;
  int head;
  int tail;
  mem_t memArray[QUEUE_SIZE];
  uint8_t *memStroage;
  int memPerSize;
};

int queue_full(void);
int queue_empty(void);
void queue_flush(void);

int queue_init(int size);
int queue_deinit(void);
int queue_send(uint8_t *data, int dataLen);
int queue_recv(uint8_t *buf, int bufLen);

#endif /* QUEUE_H_ */
