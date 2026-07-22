Ring-Buffer
===========

A simple ring buffer (circular buffer) designed for embedded systems.

Examples are given in [examples/simple.c](examples/simple.c) and [examples/tail.c](examples/tail.c).

The size of the memory buffer must be a power-of-two, and the ring buffer can contain at most `buf_size-1` bytes.

A new ring buffer is created using the `ring_buffer_init(&ring_buffer, buff, sizeof(buff))` function:

```c
char buff[64];
ring_buffer_t ring_buffer;
ring_buffer_init(&ring_buffer, buff, sizeof(buff));
```

In this case, the buffer size is 64 bytes and the ring buffer can contain 63 bytes.

The module provides the following functions for accessing the ring buffer (documentation can be found in [ringbuffer.h](ringbuffer.h)):

```c
void ring_buffer_queue(ring_buffer_t *buffer, char data);
void ring_buffer_queue_arr(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size);
ring_buffer_size_t ring_buffer_queue_arr_safe(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size);
uint8_t ring_buffer_dequeue(ring_buffer_t *buffer, char *data);
ring_buffer_size_t ring_buffer_dequeue_arr(ring_buffer_t *buffer, char *data, ring_buffer_size_t len);
uint8_t ring_buffer_discard(ring_buffer_t *buffer);
ring_buffer_size_t ring_buffer_discard_arr(ring_buffer_t *buffer, ring_buffer_size_t len);
uint8_t ring_buffer_peek(ring_buffer_t *buffer, char *data, ring_buffer_size_t index);
ring_buffer_size_t ring_buffer_peek_arr(ring_buffer_t *buffer, char *data, ring_buffer_size_t len);
uint8_t ring_buffer_cmp(ring_buffer_t *buffer, uint8_t data, ring_buffer_size_t index);
uint8_t ring_buffer_is_empty(ring_buffer_t *buffer);
uint8_t ring_buffer_is_full(ring_buffer_t *buffer);
ring_buffer_size_t ring_buffer_num_items(ring_buffer_t *buffer);
```

Safe queueing
-------------

`ring_buffer_queue_arr_safe()` queues as many bytes as the available space permits without overwriting unread data. It returns the number of bytes actually queued. A return value smaller than the requested size means that the remaining input bytes were dropped.

This function is suitable for a single interrupt-context producer and a single task-context consumer.

```c
const char data[] = {0x11, 0x22, 0x33};
ring_buffer_size_t queued;

queued = ring_buffer_queue_arr_safe(&ring_buffer, data, sizeof(data));
if (queued != sizeof(data)) {
    /* The ring buffer did not have enough free space. */
}
```

Comparing a byte
----------------

`ring_buffer_cmp()` compares the byte at `index` with an input value without removing it from the ring buffer. The input may be either a variable or a constant. It returns `1U` when the byte exists and matches, otherwise it returns `0U`. An out-of-range index also returns `0U`.

```c
uint8_t expected = 0x55U;

if (ring_buffer_cmp(&ring_buffer, expected, 0U) == 1U) {
    /* The oldest byte matches the variable. */
}

if (ring_buffer_cmp(&ring_buffer, 0xAAU, 1U) == 1U) {
    /* The byte at index 1 matches the constant. */
}
```
