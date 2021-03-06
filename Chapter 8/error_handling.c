#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"
#ifdef _WIN32

#include <string.h>

static char buffer[2048];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused) {}

#else

#include <editline/readline.h>
#include <editline/history.h>

#endif

/* Declare union to store data in lval */
typedef union {
    long num;
    int err;
} lval_data;

/* Declare new lval Struct */
typedef struct {
    int type;
    lval_data data;
} lval;

/* Create Enumeration of Possible lval Types */
enum lval_types { 
        LVAL_NUM, 
        LVAL_ERR 
};

/* Create Enumeration of Possible Error Types */
enum lval_err_types { 
    LERR_DIV_ZERO, 
    LERR_BAD_OP, 
    LERR_BAD_NUM
};

/* Create a new number type lval */
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.data.num = x;
    return v;
}

lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.data.err = x;
    return v;
}

/* Print an "lval" */
void lval_print(lval v) {
    switch(v.type) {
        /* In the case the type is a number print it, then 'break' out of the switch. */
        case LVAL_NUM: printf("%li", v.data.num); break;

        /* In the case the type is an error */
        case LVAL_ERR:
            /* Check What exact type of error it is and print it */
            printf("Error: ");
            if (v.data.err == LERR_DIV_ZERO) { printf("Division by zero."); }
            if (v.data.err == LERR_BAD_OP)   { printf("Invalid operator."); }
            if (v.data.err == LERR_BAD_NUM)  { printf("Invalid number."); }
        break;
    }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {

    /* If either value is an error return it */
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    /* Otherwise do maths on the number values */
    if (strcmp(op, "+") == 0) { return lval_num(x.data.num + y.data.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.data.num - y.data.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.data.num * y.data.num); }
    if (strcmp(op, "/") == 0) {
        /* If second operand is zero return error instead of result */
        return y.data.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.data.num - y.data.num);
    }
    if (strcmp(op, "%") == 0) { return lval_num(x.data.num % y.data.num); }
    if (strcmp(op, "^") == 0) { return lval_num(pow(x.data.num, y.data.num)); }
    if (strcmp(op, "min") == 0) { 
        return x.data.num < y.data.num ? lval_num(x.data.num) : lval_num(y.data.num);
    }
    if (strcmp(op, "max") == 0) {
        return x.data.num > y.data.num ? lval_num(x.data.num) : lval_num(y.data.num);
    }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {

    if (strstr(t->tag, "number")) { 
        /*CHeck if there is some error in conversion */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    char* op = t->children[1]->contents;
    lval x = eval(t->children[2]);

    if (strcmp(op, "-") == 0 && !(strstr(t->children[3]->tag, "expr"))) {
        return lval_num(-x.data.num);
    }

    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char** argv) {

    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    mpca_lang(MPC_LANG_DEFAULT,
        "                                                                       \
            number   : /-?[0-9]+/ ;                                             \
            operator : '+' | '-' | '*' | '/' | \"min\" | \"max\" | '^' | '%' ;  \
            expr     : <number> | '(' <operator> <expr>+ ')' ;                  \
            lispy    : /^/ <operator> <expr>+ /$/ ;                             \
        ",
        Number, Operator, Expr, Lispy);

    puts("Lispy Version 0.0.0.0.2");
    puts("Press Ctrl+C to exit\n");

    while (1) {

        char* input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);
        } else {    
          mpc_err_print(r.error);
          mpc_err_delete(r.error);
        } 

        free(input);
        
    }

    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;

}