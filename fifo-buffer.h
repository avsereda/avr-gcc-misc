/*
 * Copyright (c) 2018 Sereda Anton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fifo_buffer {
        uint8_t *mem;

        size_t mem_size;
        size_t size;

        size_t get_offset;
        size_t put_offset;
};

void fifo_buffer_init(struct fifo_buffer *fifo, uint8_t *ptr, size_t size);
bool fifo_buffer_put_byte(struct fifo_buffer *fifo, uint8_t byte);
bool fifo_buffer_get_byte(struct fifo_buffer *fifo, uint8_t *store);
size_t fifo_buffer_get_size(struct fifo_buffer *fifo);

static inline bool fifo_buffer_is_empty(struct fifo_buffer *fifo)
{
        return fifo_buffer_get_size(fifo) == 0u;
}

static inline bool fifo_buffer_is_full(struct fifo_buffer *fifo)
{
        return fifo_buffer_get_size(fifo) == fifo->mem_size;
}

static inline size_t fifo_buffer_get_unused_size(struct fifo_buffer *fifo)
{
        return fifo->mem_size - fifo_buffer_get_size(fifo);
}

#ifdef __cplusplus
}
#endif

#endif /* FIFO_BUFFER_H */
