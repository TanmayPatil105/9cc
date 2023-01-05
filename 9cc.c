#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>


typedef enum{
    TK_RESERVED,
    TK_NUM,
    TK_EOF,
}TokenKind;



typedef struct Token Token;

struct Token{
    TokenKind kind;
    Token *next;
    int val;
    char *str;
};

char *user_input;

Token *token;


typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
} Nodekind;




typedef struct Node Node;

struct Node {
    Nodekind kind;
    Node *lhs;
    Node *rhs;
    int val;
};

Node *new_node(Nodekind kind, Node *lhs, Node *rhs){
    Node *node = calloc(1,sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val){
    Node *node = calloc(1,sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " ");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}


int expect_number(){
    if(token->kind != TK_NUM){
        error_at(token->str,"expected a number");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof(){
    return token->kind == TK_EOF;
}



void expect(char op){
    if(token->kind != TK_RESERVED || token->str[0] != op){
        error_at(token->str,"expected '%c'",op);
        exit(1);
    }
    token = token->next;
}

bool consume(char op){
    if (token->kind != TK_RESERVED || token->str[0] != op){
        return false;
    }
    token = token->next;
    return true;
}


Node *mul();
Node *expr();
Node *primary();

Node *mul(){
    Node *node = primary();

    for(;;){
        if (consume('*')){
            node = new_node(ND_MUL, node, primary());
        } else if (consume('/')){
            node = new_node(ND_DIV, node, primary());
        } else {
            return node;
        }
    }
}

Node *expr(){

    Node *node = mul();

    for(;;){
        if (consume('+')){
            node = new_node(ND_ADD, node, mul());
        } else if (consume('-')){
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }

    return node;
}

Node *primary(){
    
    if (consume('(')){
        Node *node = expr();
        expect(')');
        return node;
    }

    return new_node_num(expect_number());
}


void gen(Node *node){
    if (node->kind == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind){
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
    }

    printf("    push rax\n");

}


Token *new_token(TokenKind kind, Token *cur, char *str){
    Token *new = calloc(1,sizeof(Token));
    new->kind = kind;
    new->str = str;
    cur->next=new;
    return new;
}



Token *tokenize(char *p){
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while(*p){
        if (isspace(*p)){
            p++;
            continue;
        }

        if (isdigit(*p)){
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p,&p, 10);
            continue;
        }

        if (strchr("+-*/()", *p)){
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

        error_at(token->str,"Cannot Tokenize");
        
    }

    new_token(TK_EOF, cur, p);
    
    return head.next;
}


int main(int argc, char **argv){
    if(argc != 2){
        error("%s: invalid number of arguments", argv[0]);
        return 1;
    }

    user_input = argv[1];

    token = tokenize(argv[1]);

    Node *node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    gen(node);

    printf("    pop rax\n");
    printf("    ret\n");    

    
    return 0;

}


