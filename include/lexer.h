#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define UNUSED(x) (void)(x)


#define ASSERT(cond, msg)           \
    if(!(cond)) {                   \
        fprintf(stderr, "%s", msg); \
        exit(1);                    \
    }

#define TOKEN_PRINT(t) (int)t.text_len, t.text

typedef enum {
    TOKEN_NEWLINE,
    TOKEN_TEXT,
    TOKEN_TITLE1,
    TOKEN_TITLE2,
    TOKEN_TITLE3,
    TOKEN_TITLE4,
    TOKEN_INLINE_CODE,
    TOKEN_BLOCK_CODE,
    TOKEN_LIST,
    TOKEN_IMAGE,
    TOKEN_LINK
} Token_Type;

typedef struct {
    Token_Type type;
    const char *text;
    size_t text_len;
    size_t row;
    size_t col;
} Token;

typedef struct {
    char *content;
    size_t content_len;
    size_t cursor;
    size_t line;
    size_t bol;
} Lexer;

Lexer lexer_init(char *content, size_t content_len);
Token lexer_next(Lexer *l);

#endif
