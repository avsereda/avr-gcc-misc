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

#ifndef MEM_CHUNK_H
#define MEM_CHUNK_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mem_chunk {
        void *ptr;

        size_t size;
        size_t offset;
};

#define MEM_CHUNK_OFFSET_PTR(chunk) ((void *) &(((uint8_t *) (chunk)->ptr)[(chunk)->offset]))

static inline bool mem_chunk_slice_from_offset(struct mem_chunk *base,
                                               struct mem_chunk *slice, size_t size)
{
        if (base->size < (base->offset + size)) {
                /* Can't create slice with a given size */
                return false;
        }

        slice->ptr = MEM_CHUNK_OFFSET_PTR(base);
        slice->offset = 0u;
        slice->size = size;


        return true;
}

static inline void mem_chunk_set(struct mem_chunk *chunk, void *ptr, size_t size)
{
        chunk->ptr = ptr;
        chunk->size = size;
        chunk->offset = 0u;
}

#ifdef __cplusplus
}
#endif

#endif /* MEM_CHUNK_H */
