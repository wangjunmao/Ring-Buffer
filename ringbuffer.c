#include "ringbuffer.h"
#include <string.h>

/**
 * @file
 * Implementation of ring buffer functions.
 */

void ring_buffer_init(ring_buffer_t *buffer, char *buf, size_t buf_size) {
  RING_BUFFER_ASSERT(RING_BUFFER_IS_POWER_OF_TWO(buf_size) == 1);
  buffer->buffer = buf;
  buffer->buffer_mask = buf_size - 1;
  buffer->tail_index = 0;
  buffer->head_index = 0;
}

void ring_buffer_queue(ring_buffer_t *buffer, char data) {
  /* Is buffer full? */
  if(ring_buffer_is_full(buffer)) {
    /* Is going to overwrite the oldest byte */
    /* Increase tail index */
    buffer->tail_index = ((buffer->tail_index + 1) & RING_BUFFER_MASK(buffer));
  }

  /* Place data in buffer */
  buffer->buffer[buffer->head_index] = data;
  buffer->head_index = ((buffer->head_index + 1) & RING_BUFFER_MASK(buffer));
}

void ring_buffer_queue_arr(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size) {
  if(size == 0) {
    return;
  }

  ring_buffer_size_t mask = RING_BUFFER_MASK(buffer);
  ring_buffer_size_t buf_size = mask + 1;
  ring_buffer_size_t capacity = mask;
  ring_buffer_size_t head = buffer->head_index;
  ring_buffer_size_t tail = buffer->tail_index;
  ring_buffer_size_t original_size = size;

  if(size >= capacity) {
    ring_buffer_size_t head_index = ((head + original_size) & mask);
    ring_buffer_size_t tail_index = ((head_index + 1) & mask);
    ring_buffer_size_t first_chunk = buf_size - tail_index;

    data += (original_size - capacity);
    if(first_chunk > capacity) {
      first_chunk = capacity;
    }

    memcpy(buffer->buffer + tail_index, data, first_chunk);
    if(first_chunk < capacity) {
      memcpy(buffer->buffer, data + first_chunk, capacity - first_chunk);
    }

    buffer->head_index = head_index;
    buffer->tail_index = tail_index;
    return;
  }

  ring_buffer_size_t num_items = ((head - tail) & mask);
  ring_buffer_size_t free_space = capacity - num_items;
  if(size > free_space) {
    tail = ((tail + size - free_space) & mask);
  }

  ring_buffer_size_t first_chunk = buf_size - head;
  if(first_chunk > size) {
    first_chunk = size;
  }

  memcpy(buffer->buffer + head, data, first_chunk);
  if(first_chunk < size) {
    memcpy(buffer->buffer, data + first_chunk, size - first_chunk);
  }

  buffer->head_index = ((head + size) & mask);
  buffer->tail_index = tail;
}

ring_buffer_size_t ring_buffer_queue_arr_safe(ring_buffer_t *buffer,
                                              const char *data,
                                              ring_buffer_size_t size) {
  ring_buffer_size_t mask;
  ring_buffer_size_t buf_size;
  ring_buffer_size_t head;
  ring_buffer_size_t tail;
  ring_buffer_size_t free_space;
  ring_buffer_size_t first_chunk;

  if(size == 0) {
    return 0;
  }

  mask = RING_BUFFER_MASK(buffer);
  buf_size = mask + 1;
  head = buffer->head_index;
  tail = buffer->tail_index;
  free_space = mask - ((head - tail) & mask);

  if(size > free_space) {
    size = free_space;
  }
  if(size == 0) {
    return 0;
  }

  first_chunk = buf_size - head;
  if(first_chunk > size) {
    first_chunk = size;
  }

  memcpy(buffer->buffer + head, data, first_chunk);
  if(first_chunk < size) {
    memcpy(buffer->buffer, data + first_chunk, size - first_chunk);
  }

  /* Publish the new data only after both copies are complete. */
  __asm volatile ("" ::: "memory");
  buffer->head_index = ((head + size) & mask);
  return size;
}

uint8_t ring_buffer_dequeue(ring_buffer_t *buffer, char *data) {
  if(ring_buffer_is_empty(buffer)) {
    /* No items */
    return 0;
  }

  *data = buffer->buffer[buffer->tail_index];
  buffer->tail_index = ((buffer->tail_index + 1) & RING_BUFFER_MASK(buffer));
  return 1;
}

ring_buffer_size_t ring_buffer_dequeue_arr(ring_buffer_t *buffer, char *data, ring_buffer_size_t len) {
  ring_buffer_size_t mask = RING_BUFFER_MASK(buffer);
  ring_buffer_size_t tail = buffer->tail_index;
  ring_buffer_size_t count = ((buffer->head_index - tail) & mask);

  if((count == 0) || (len == 0)) {
    /* No items */
    return 0;
  }

  if(len < count) {
    count = len;
  }

  ring_buffer_size_t buf_size = mask + 1;
  ring_buffer_size_t first_chunk = buf_size - tail;
  if(first_chunk > count) {
    first_chunk = count;
  }

  memcpy(data, buffer->buffer + tail, first_chunk);
  if(first_chunk < count) {
    memcpy(data + first_chunk, buffer->buffer, count - first_chunk);
  }

  buffer->tail_index = ((tail + count) & mask);
  return count;
}

uint8_t ring_buffer_discard(ring_buffer_t *buffer) {
  if(ring_buffer_is_empty(buffer)) {
    /* No items */
    return 0;
  }

  buffer->tail_index = ((buffer->tail_index + 1) & RING_BUFFER_MASK(buffer));
  return 1;
}

ring_buffer_size_t ring_buffer_discard_arr(ring_buffer_t *buffer, ring_buffer_size_t len) {
  ring_buffer_size_t count = ring_buffer_num_items(buffer);
  if((count == 0) || (len == 0)) {
    /* No items */
    return 0;
  }

  if(len < count) {
    count = len;
  }

  buffer->tail_index = ((buffer->tail_index + count) & RING_BUFFER_MASK(buffer));
  return count;
}

uint8_t ring_buffer_peek(ring_buffer_t *buffer, char *data, ring_buffer_size_t index) {
  if(index >= ring_buffer_num_items(buffer)) {
    /* No items at index */
    return 0;
  }

  /* Add index to pointer */
  ring_buffer_size_t data_index = ((buffer->tail_index + index) & RING_BUFFER_MASK(buffer));
  *data = buffer->buffer[data_index];
  return 1;
}

ring_buffer_size_t ring_buffer_peek_arr(ring_buffer_t *buffer, char *data, ring_buffer_size_t len) {
  ring_buffer_size_t mask = RING_BUFFER_MASK(buffer);
  ring_buffer_size_t tail = buffer->tail_index;
  ring_buffer_size_t count = ((buffer->head_index - tail) & mask);

  if((count == 0) || (len == 0)) {
    /* No items */
    return 0;
  }

  if(len < count) {
    count = len;
  }

  ring_buffer_size_t buf_size = mask + 1;
  ring_buffer_size_t first_chunk = buf_size - tail;
  if(first_chunk > count) {
    first_chunk = count;
  }

  memcpy(data, buffer->buffer + tail, first_chunk);
  if(first_chunk < count) {
    memcpy(data + first_chunk, buffer->buffer, count - first_chunk);
  }

  return count;
}

uint8_t ring_buffer_cmp(ring_buffer_t *buffer, uint8_t data, ring_buffer_size_t index) {
  if(index >= ring_buffer_num_items(buffer)) {
    /* No items at index */
    return 0U;
  }

  /* Add index to pointer */
  ring_buffer_size_t data_index = ((buffer->tail_index + index) & RING_BUFFER_MASK(buffer));
  return ((uint8_t)buffer->buffer[data_index] == data) ? 1U : 0U;
}

extern inline uint8_t ring_buffer_is_empty(ring_buffer_t *buffer);
extern inline uint8_t ring_buffer_is_full(ring_buffer_t *buffer);
extern inline ring_buffer_size_t ring_buffer_num_items(ring_buffer_t *buffer);
