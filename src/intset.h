#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct intset{
    uint32_t encoding;
    uint32_t length;
    int8_t contents[];
} intset;

intset *intsetNew(void);
intset *intsetAdd(intset *is, int64_t value, int8_t *success);