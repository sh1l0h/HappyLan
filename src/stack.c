#include <stdlib.h>
#include "stack.h"


void add(struct element **stack, struct element *e){
    e->next = *stack;
    *stack = e;
}

struct element *pop(struct element **stack){
    struct element *temp = *stack;
    *stack = (*stack)->next;
    return temp;
} 

