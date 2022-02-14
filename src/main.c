#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include "btrstr.h"
#include "stack.h"

enum instruction_type {
    CONST,
    ADD,
    SUB,
    MUL,
    DIV,
    WRITE,
    JUMP,
    FJUMP,
    LESS,
    EQ,
    NEQ,
    AND,
    OR,
    HALT
};

struct instruction {
    enum instruction_type type;
    union {
        int arg_int;
        char *label;
    } arg;
    size_t line;
};

struct label {
    char *name;
    struct instruction *_inst;
};

void usage(char *command)
{
    printf("Usage: %s -s input\n   or: %s -c input [-o output]\n   or: %s -h\n\n  -h    Show this text\n  -c    Compile\n  -s    Simulate\n  -o    Output into a file\n", command, command, command);
}

char *get_label(struct instruction *inst, struct label *labels)
{
    struct label *temp = labels;
    while(temp->_inst != NULL){
        if(temp->_inst == inst){
            return temp->name;
        }
        temp++;
    }
    return NULL;
}

struct instruction *tokenize(FILE *f, struct label **labels, size_t *size)
{
    size_t lines = 0;
    while(!feof(f))
    {
        char ch = fgetc(f);
        if(ch == '\n'){
            lines++;
        }
    }
    fseek(f, 0, SEEK_SET);

    size_t labels_num = 0;
    while(!feof(f)){
        char ch = fgetc(f);
        if(ch == ':')
            labels_num++;
    }
    fseek(f, 0, SEEK_SET);

    *labels = (struct label *) malloc(sizeof(struct label)*(labels_num+1));
    (*labels + labels_num)->_inst = NULL;

    struct instruction *result = (struct instruction *) malloc(sizeof(struct instruction)*lines);
    char *line = NULL;
    size_t len = 0;

    regex_t to_skip;
    regex_t val_inst;

    int r1 = regcomp(&to_skip, "^[ ]*(//.*)?\n$", REG_EXTENDED);
    int r2 = regcomp(&val_inst, "^ *([A-Z]+( +(-?[0-9]+(.[0-9]+)?|\".*\"|[a-z]+))?( *(//.*)?)?|[a-z]+:)\n$", REG_EXTENDED);


    if(r1 || r2){
        exit(1);
    }

    int halt = 0;
    labels_num = 0;
    size_t line_num = 0;
    size_t inst_num = 0;
    while(getline(&line, &len, f) > 0){
        replace_tabs(line);
        line_num++;
        int v = regexec(&to_skip, line, 0, NULL, 0);
        if(v == 0) 
            goto con;

        if(regexec(&val_inst, line, 0, NULL, 0)){
            printf("err");
            //TODO:error
            exit(1);
        }


        char *inst = strtok(line,"\n ");
        if(inst == NULL)
            goto con;

        char *t;
        if((t = strrchr(inst,':')) != NULL){
            *t = '\0';
            (*labels + labels_num)->name = strdup(inst);
            (*labels + labels_num++)->_inst = (result + inst_num);     
            goto con;
        }

        struct instruction *curr = (result + inst_num++);
        curr->line = line_num;

        if(strcmp(inst,"CONST") == 0){
            curr->type = CONST;
            curr->arg.arg_int = (int) strtol(strtok(NULL, "\n "), (char **)NULL, 10);
        }
        else if(strcmp(inst,"ADD") == 0){
            curr->type = ADD;
        }
        else if(strcmp(inst,"WRITE") == 0){
            curr->type = WRITE;
        } 
        else if(strcmp(inst,"JUMP") == 0){
            curr->type = JUMP;
            curr->arg.label = strdup(strtok(NULL, "\n "));
        }
        else if(strcmp(inst,"FJUMP") == 0){
            curr->type = FJUMP;
            curr->arg.label = strdup(strtok(NULL, "\n "));
        } 
        else if(strcmp(inst, "HALT") == 0){
            curr->type = HALT;
            halt = 1;
        }
        else if(strcmp(inst, "EQ") == 0){
            curr->type = EQ;
        }
        else if(strcmp(inst, "LESS") == 0){
            curr->type = LESS;
        }

con:
        free(line);
        len =0;
        line = NULL;
    }

    if(realloc(result, sizeof(struct instruction)*(inst_num)) == NULL){
        fclose(f);
        //TODO: error massage
        exit(1);
    }

    if(!halt){
        printf("\033[0;31mError:\033[0m There is no \"HALT\" instruction.\n");
        exit(1);
    }
    
    *size = inst_num;
    fclose(f);

    return result;
}
    
void simulate(struct instruction *program, struct label *labels)
{
    struct element *stack = NULL;

    struct instruction *curr = program;
    while(curr->type != HALT){
        switch(curr->type){
            case CONST:
                struct element *e = (struct element *) malloc(sizeof(struct element));
                e->type = et_int;
                e->data.e_int = curr->arg.arg_int;
                add(&stack, e);
                break;

            case ADD:
                struct element *ADD_a = pop(&stack);
                struct element *ADD_b = pop(&stack);
                int result = ADD_a->data.e_int + ADD_b->data.e_int;
                struct element *ADD_e = (struct element *) malloc(sizeof(struct element));
                ADD_e->type = et_int;
                ADD_e->data.e_int = result;
                add(&stack, ADD_e);

                free(ADD_a);
                free(ADD_b);
                break;

            case WRITE:
                struct element *WRT_a = pop(&stack);

                printf("%d\n", WRT_a->data.e_int);
                free(WRT_a);
                break;

            case JUMP:
                struct label *t = labels;
                while(t->_inst != NULL){
                    if(strcmp(curr->arg.label, t->name) == 0){
                        curr = t->_inst;
                        goto con;
                    }
                }
                //TODO:error massage
                exit(1);

            case LESS:
                struct element *L_a = pop(&stack);
                struct element *L_b = pop(&stack);

                struct element *L_e = (struct element *) malloc(sizeof(struct element));
                L_e->type = et_boolean;
                L_e->data.e_int = L_b->data.e_int < L_a->data.e_int;
                add(&stack, L_e);

                free(L_a);
                free(L_b);
                break;

            case FJUMP:
                struct element *F_a = pop(&stack);

                if(F_a->type != et_boolean){
                    // TODO:error massage
                    exit(1);
                }

                if(F_a->data.e_int != 0)
                    break;

                struct label *F_t = labels;
                while(F_t->_inst != NULL){
                    if(strcmp(curr->arg.label, F_t->name) == 0){
                        curr = F_t->_inst;
                        goto con;
                    }
                }
                //TODO:error massage
                exit(1);
        }
        curr++;
con:
    }
}

void compile(struct instruction *program, size_t program_size, struct label *labels, const char *in_name, const char *out_name)
{
    if(!out_name)
        out_name = "a.out";

    size_t in_name_len = strlen(in_name);
    char asm_name[in_name_len+2];
    char o_name[in_name_len];
    for(int i = 0; i < in_name_len - 2; i++){
        asm_name[i] = in_name[i]; 
        o_name[i] = in_name[i];
    }
    asm_name[in_name_len + 1] = '\0';
    asm_name[in_name_len] = 'm';
    asm_name[in_name_len - 1] = 's';
    asm_name[in_name_len - 2] = 'a';

    o_name[in_name_len - 1] = '\0';
    o_name[in_name_len - 2] = 'o';

    FILE *f = fopen(asm_name, "w");
    
    fprintf(f, "section .text\n");
    fprintf(f, "    global _start\n\n"); 
    fprintf(f, "write_int:\n");
    fprintf(f, "	push    rbp\n");
    fprintf(f, "	mov     rbp, rsp\n");
    fprintf(f, "	sub     rsp, 48\n");
    fprintf(f, "	mov     DWORD [rbp-36], edi\n");
    fprintf(f, "	mov     DWORD [rbp-4], 0\n");
    fprintf(f, "	cmp     DWORD [rbp-36], 0\n");
    fprintf(f, "	jns     .L2\n");
    fprintf(f, "	neg     DWORD [rbp-36]\n");
    fprintf(f, "	mov     DWORD [rbp-4], 1\n");
    fprintf(f, ".L2:\n");
    fprintf(f, "	mov     BYTE [rbp-9], 10\n");
    fprintf(f, "	mov     DWORD [rbp-8], 2\n");
    fprintf(f, ".L3:\n");
    fprintf(f, "	mov     edx, DWORD [rbp-36]\n");
    fprintf(f, "	movsx   rax, edx\n");
    fprintf(f, "	imul    rax, rax, 1717986919\n");
    fprintf(f, "	shr     rax, 32\n");
    fprintf(f, "	sar     eax, 2\n");
    fprintf(f, "	mov     esi, edx\n");
    fprintf(f, "	sar     esi, 31\n");
    fprintf(f, "	sub     eax, esi\n");
    fprintf(f, "	mov     ecx, eax\n");
    fprintf(f, "	mov     eax, ecx\n");
    fprintf(f, "	sal     eax, 2\n");
    fprintf(f, "	add     eax, ecx\n");
    fprintf(f, "	add     eax, eax\n");
    fprintf(f, "	mov     ecx, edx\n");
    fprintf(f, "	sub     ecx, eax\n");
    fprintf(f, "	mov     eax, ecx\n");
    fprintf(f, "	add     eax, 48\n");
    fprintf(f, "	mov     ecx, eax\n");
    fprintf(f, "	mov     eax, DWORD [rbp-8]\n");
    fprintf(f, "	lea     edx, [rax+1]\n");
    fprintf(f, "	mov     DWORD [rbp-8], edx\n");
    fprintf(f, "	mov     edx, 12\n");
    fprintf(f, "	sub     edx, eax\n");
    fprintf(f, "	movsx   rax, edx\n");
    fprintf(f, "	mov     BYTE [rbp-20+rax], cl\n");
    fprintf(f, "	mov     eax, DWORD [rbp-36]\n");
    fprintf(f, "	movsx   rdx, eax\n");
    fprintf(f, "	imul    rdx, rdx, 1717986919\n");
    fprintf(f, "	shr     rdx, 32\n");
    fprintf(f, "	sar     edx, 2\n");
    fprintf(f, "	sar     eax, 31\n");
    fprintf(f, "	mov     ecx, eax\n");
    fprintf(f, "	mov     eax, edx\n");
    fprintf(f, "	sub     eax, ecx\n");
    fprintf(f, "	mov     DWORD [rbp-36], eax\n");
    fprintf(f, "	cmp     DWORD [rbp-36], 0\n");
    fprintf(f, "	jne     .L3\n");
    fprintf(f, "	cmp     DWORD [rbp-4], 0\n");
    fprintf(f, "	je      .L4\n");
    fprintf(f, "	mov     eax, DWORD [rbp-8]\n");
    fprintf(f, "	lea     edx, [rax+1]\n");
    fprintf(f, "	mov     DWORD [rbp-8], edx\n");
    fprintf(f, "	mov     edx, 12\n");
    fprintf(f, "	sub     edx, eax\n");
    fprintf(f, "	movsx   rax, edx\n");
    fprintf(f, "	mov     BYTE [rbp-20+rax], 45\n");
    fprintf(f, ".L4:\n");
    fprintf(f, "	mov     eax, DWORD [rbp-8]\n");
    fprintf(f, "	sub     eax, 1\n");
    fprintf(f, "	cdqe\n");
    fprintf(f, "	mov     edx, 13\n");
    fprintf(f, "	sub     edx, DWORD [rbp-8]\n");
    fprintf(f, "	lea     rcx, [rbp-20]\n");
    fprintf(f, "	movsx   rdx, edx\n");
    fprintf(f, "	add     rcx, rdx\n");
    fprintf(f, "	mov     rdx, rax\n");
    fprintf(f, "	mov     rsi, rcx\n");
    fprintf(f, "	mov     edi, 1\n");
    fprintf(f, "	mov 	rax, 1\n");
    fprintf(f, "	syscall\n");
    fprintf(f, "	nop\n");
    fprintf(f, "	leave\n");
    fprintf(f, "	ret\n");


    fprintf(f, "\n_start:\n");
    for(int i = 0; i < program_size; i++){
        struct instruction curr = program[i];

        char *label_name = get_label(program + i, labels);

        if(label_name)
            fprintf(f, "%s:\n", label_name);

        switch(curr.type){
        
            case CONST:
                fprintf(f, ";   CONST\n");
                fprintf(f, "    push    %d\n", curr.arg.arg_int);
                break;
        
            case ADD:
                fprintf(f, ";   ADD\n");
                fprintf(f, "    pop     rax\n");
                fprintf(f, "    pop     rbx\n");
                fprintf(f, "    add     rax, rbx\n");
                fprintf(f, "    push    rax\n");
                break;

            case WRITE:
                fprintf(f, ";   WRITE\n");
                fprintf(f, "    pop     rdi\n");
                fprintf(f, "    call    write_int\n");
                break;

            case LESS:
                fprintf(f, ";   LESS\n");
                fprintf(f, "    pop     rax\n");
                fprintf(f, "    pop     rbx\n");
                fprintf(f, "    cmp     rax, rbx\n");
                fprintf(f, "    jg      L%d\n", i);
                fprintf(f, "    push    0\n");
                fprintf(f, "    jmp     L_%d\n", i);
                fprintf(f, "L%d:\n", i);
                fprintf(f, "    push    1\n");
                fprintf(f, "L_%d:\n", i);
                break;

            case JUMP:
                fprintf(f, ";   JUMP\n");
                fprintf(f, "    jmp     %s\n", curr.arg.label);
                break;

            case FJUMP:
                fprintf(f, ";   FJUMP\n");
                fprintf(f, "    pop     rax\n");
                fprintf(f, "    cmp     rax, 0\n");
                fprintf(f, "    je      %s\n", curr.arg.label);
                break;

            case EQ:
                fprintf(f, ";   EQ\n");
                fprintf(f, "    pop     rax\n");
                fprintf(f, "    pop     rbx\n");
                fprintf(f, "    cmp     rax, rbx\n");
                fprintf(f, "    je      E%d\n", i);
                fprintf(f, "    push    0\n");
                fprintf(f, "    jmp     E_%d\n", i);
                fprintf(f, "E%d:\n", i);
                fprintf(f, "    push    1\n");
                fprintf(f, "E_%d:\n", i);
                break;

            case HALT:
                fprintf(f, ";   HALT\n");
                fprintf(f, "    mov     rax, 60\n");
                fprintf(f, "    mov     rdi, 0\n");
                fprintf(f, "    syscall\n");
                break;
        }
    }


    fclose(f);

    char com[14+sizeof(asm_name)];
    sprintf(com, "nasm -f elf64 %s", asm_name);

    system(com);

    char com2[7 + sizeof(o_name) + strlen(out_name)];
    sprintf(com2, "ld %s -o %s", o_name, out_name); 

    system(com2);
}


int main(int argc, char **argv)
{
    if(argc == 1) {
        usage(*argv);
        exit(0);
    }

    int sim = strcmp(argv[1], "-s") == 0;
    int comp = strcmp(argv[1], "-c") == 0;
    int help = strcmp(argv[1], "-h") == 0;

    if(!sim && !comp && !help){
        if(*(argv[1]) == '-'){
            printf("\033[0;31mError:\033[0m Unknown operetor \"%s\". Try \"%s -h\".\n", argv[1], argv[0]);
        }
        else{
            printf("\033[0;31mError:\033[0m An operator must be provided. Try \"%s -h\".\n", argv[0]);
        }
        exit(1);
    }

    if(help){
        usage(argv[0]);
        exit(0);
    }

    if(sim && argc > 3){
        printf("\033[0;31mError:\033[0m Too many arguments.\n");
        exit(1);
    }

    if(argc == 2){
        printf("\033[0;31mError:\033[0m An input file must be provided.\n");
        exit(1);
    } 

    if(strcmp(strrchr(argv[2], '.')+1,"hl") != 0){
        printf("\033[0;31mError:\033[0m An input file extension must be \"hl\".\n");
        exit(1);
    } 

    char *out_name = NULL;
    if(comp){
        if(argc == 3)
            out_name = NULL;
        else if(argc == 5 && strcmp(argv[3], "-o") == 0)
            out_name = argv[4];
        else{
            printf("\033[0;31mError:\033[0m Invalid command. Try \"%s -h\".\n", argv[2]);
            exit(1);
        }
    }

    FILE *f = fopen(argv[2], "r");

    if(f == NULL){
        printf("\033[0;31mError:\033[0m Could not open \"%s\".\n", argv[2]);
        exit(1);
    }

    struct label *labels = NULL;
    
    size_t program_size;
    struct instruction *program = tokenize(f, &labels, &program_size);

    if(sim)
        simulate(program, labels);
    else
        compile(program, program_size, labels, argv[2], out_name);
    
    for(int i = 0; i < program_size; i++){
        if(program[i].type == JUMP){
            free(program[i].arg.label);
        }
    }
    
    free(labels);
    free(program);
    
    return 0;
}
