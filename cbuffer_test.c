#include <stdio.h>
#include "cbuffer.h"

int main() {
    CircularBuffer(int, 10) buf = {0};
    cbuf_append(&buf, 1);
    cbuf_append(&buf, 2);
    cbuf_append(&buf, 3);
    cbuf_append(&buf, 4);
    cbuf_append(&buf, 5);

    cbuf_at(&buf, 3) = 69;
    cbuf_set(&buf, 4, 68);

    int elm;
    for cbuf_each(buf, &elm) {
        printf("elm: %d\n", elm);
    }

    puts("backwards");
    while (buf.count > 0) {
        printf("elm: %d\n", cbuf_pop_right(&buf));
    }

    return 0;
}
