#include <stddef.h>

#define CircularBuffer(type, cap) \
    struct {                      \
        type items[cap];          \
        size_t index;             \
        size_t count;             \
    }                              

#define cbuf_cap(buf) (sizeof (buf).items / sizeof (buf).items[0])

#define cbuf_append(buf, val) do {                                      \
     (buf)->items[((buf)->index+(buf)->count) % cbuf_cap(*(buf))] = val;\
     (buf)->count++;                                                    \
     if (((buf)->count) > cbuf_cap(*(buf))) {                           \
        (buf)->count = cbuf_cap(*(buf));                                \
        (buf)->index++;                                                 \
     }                                                                  \
} while (0)                                                              

#define cbuf_pop_right(buf) ((buf)->items[( (buf)->index + (--(buf)->count) ) % cbuf_cap(*(buf))])

#define cbuf_pop_left(buf) (              \
    (buf)->index == cbuf_cap(*(buf))-1 ? (\
        (buf)->index = 0,                 \
        (buf)->count--,                   \
        (buf)->items[cbuf_cap(*(buf))-1]  \
    ) : (                                 \
        (buf)->count--,                   \
        (buf)->items[(buf)->index++]      \
    )                                     \
)                                          

#define cbuf_at(buf, i) ((buf)->items[( (buf)->index + i ) % cbuf_cap(*(buf))])
#define cbuf_get(buf, i) ((buf).items[( (buf).index + i ) % cbuf_cap(buf)])
#define cbuf_set(buf, i, ...) do { cbuf_at(buf, i) = __VA_ARGS__; } while(0)

#define cbuf_each(buf, val_ptr) (size_t cbuf__i = 0; (cbuf__i < buf.count) ? (*(val_ptr) = cbuf_get(buf, cbuf__i), 1) : 0 ; ++cbuf__i)
