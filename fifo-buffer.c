#include <string.h>
#include <util/atomic.h>

#include "fifo-buffer.h"

void fifo_buffer_init(struct fifo_buffer *fifo, uint8_t *ptr, size_t size)
{
        memset(fifo, 0, sizeof(struct fifo_buffer));

        fifo->mem = ptr;
        fifo->mem_size = size;
}

bool fifo_buffer_put_byte(struct fifo_buffer *fifo, uint8_t byte)
{
        bool retval = false;


        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                if (fifo->size < fifo->mem_size) {
                        fifo->mem[fifo->put_offset] = byte;

                        fifo->put_offset = (size_t)((fifo->put_offset + 1u) % fifo->mem_size);
                        fifo->size++;

                        retval = true;
                }
        }


        return retval;
}

bool fifo_buffer_get_byte(struct fifo_buffer *fifo, uint8_t *store)
{
        bool retval = false;


        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                if (fifo->size > 0u) {
                        *store = fifo->mem[fifo->get_offset];

                        fifo->get_offset = (size_t)((fifo->get_offset + 1u) % fifo->mem_size);
                        fifo->size--;

                        retval = true;
                }
        }


        return retval;
}

size_t fifo_buffer_get_size(struct fifo_buffer *fifo)
{
        size_t size = 0u;


        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                size = fifo->size;
        }


        return size;
}
