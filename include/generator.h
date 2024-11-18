#ifndef GENERATOR_H
#define GENERATOR_H

#include "lexer.h"

typedef enum {
    TYPE_TEXT,
    TYPE_TITLE,
    TYPE_LIST,
    TYPE_INLINE,
    TYPE_BLOCK,
    TYPE_IMAGE,
    TYPE_LINK,
    TYPE_RAW,
    TYPE_INVALID
} MarkdownDataType;

typedef struct {
    Token *content;
    Token *alt;
} MarkdownData;

typedef void (*MarkdownHandler)(const MarkdownData *data);

typedef struct {
    const MarkdownDataType type;
    MarkdownHandler start;
    MarkdownHandler end;
} MarkdownFunctionMap;

void generate(MarkdownFunctionMap fm[], Lexer *l, Token t, Token prev);

#endif
