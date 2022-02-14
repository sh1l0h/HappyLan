#ifndef __STACK_H_
#define __STACK_H_

enum element_type{
    et_int,
    et_boolean
};

struct element{
    enum element_type type;
    union {
        int e_int;
    } data;
    struct element *next;
};


void add(struct element **stack, struct element *e);

struct element *pop(struct element **stack);

#endif
