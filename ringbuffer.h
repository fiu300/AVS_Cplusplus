#ifndef __RINGBUFFER_H__

#define __RINGBUFFER_H__



#include <pthread.h>

#define MEM_TYPE_HEAP  0


typedef struct
{
    char *buffer;
    int wr_pointer;
    int rd_pointer;
    int size;
    int checksize;
    pthread_mutex_t lock;
    int memType;
} RingBuffer_t;



/* ring buffer functions */

int rb_init (RingBuffer_t *, int bufSize, int memType);

int rb_deinit(RingBuffer_t *);

int rb_write (RingBuffer_t *, unsigned char *, int);

int rb_free (RingBuffer_t *);

int rb_read (RingBuffer_t *, unsigned char *, int);

int rb_data_size (RingBuffer_t *);

int rb_clear (RingBuffer_t *);

int rb_seek(RingBuffer_t *, int offset);

int rb_check_full(RingBuffer_t *rb);

#endif





