#ifndef CMD_H
#define CMD_H


#include <stddef.h>
#include <stdio.h>


#define UNUSED(x) (void)(x)

#define ASSERT(cond, msg)           \
if(!(cond)) {                   \
fprintf(stderr, "%s", msg); \
exit(1);                    \
}

#define TOKEN_PRINT(t) (int)t.text_len, t.text

typedef enum {
    TOKEN_INVALID,
    TOKEN_NEWLINE,
    TOKEN_TEXT,
    TOKEN_TITLE1,
    TOKEN_TITLE2,
    TOKEN_TITLE3,
    TOKEN_TITLE4,
    TOKEN_INLINE_CODE,
    TOKEN_BLOCK_CODE,
    TOKEN_LIST,
    TOKEN_URL,
    TOKEN_ALTTEXT,
    TOKEN_LINKTEXT
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

#ifdef CMD_IMPLEMENTATION
#undef CMD_IMPLEMENTATION

Lexer lexer_init(char *content, size_t content_len){
    Lexer l = {0};
    l.content = content;
    l.content_len = content_len;
    return l;
}

char lexer_chop_char(Lexer *l){
    ASSERT(l->cursor < l->content_len, "Unexpected EOF")

    char x = l->content[l->cursor];
    l->cursor++;
    if(x == '\n'){
        l->line++;
        l->bol = l->cursor;
    }
    return x;
}

Token lexer_next(Lexer *l) {
    Token token = {
        .text = &l->content[l->cursor],
        .row = l->line,
        .col = l->cursor - l->bol,
    };

    if (l->cursor >= l->content_len) return token;

    // Parse titles
    if (l->content[l->cursor] == '#' && l->bol == l->cursor) {
        lexer_chop_char(l);
        token.text_len++;
        token.type = TOKEN_TITLE1;
        while (lexer_chop_char(l) == '#'){
            token.type++;
            token.text_len++;
        }
    }

    // Parse list
    else if (l->content[l->cursor] == '-' && l->content[l->cursor + 1] == ' '
             && l->bol == l->cursor) {
        lexer_chop_char(l);
        lexer_chop_char(l);
        token.type = TOKEN_LIST;
        token.text = &l->content[l->cursor];
        token.text_len = 2;
    }

    // Parse code block
    else if(strncmp(&l->content[l->cursor], "```", 3) == 0){
        lexer_chop_char(l);
        lexer_chop_char(l);
        lexer_chop_char(l);
        token.type = TOKEN_BLOCK_CODE;
        token.text = &l->content[l->cursor];
        while (strncmp(&l->content[l->cursor], "\n```", 4)) {
            lexer_chop_char(l);
            token.text_len++;
        }
        lexer_chop_char(l);
        lexer_chop_char(l);
        lexer_chop_char(l);
        lexer_chop_char(l);
    }

    // Parse inline code
    else if(l->content[l->cursor] == '`'){
        lexer_chop_char(l);
        token.type = TOKEN_INLINE_CODE;
        token.text = &l->content[l->cursor];
        while (lexer_chop_char(l) != '`') {
            ASSERT(l->content[l->cursor] != '\n', "Found newline in inline code block");
            token.text_len++;
        }
    }

    // Parse alt text
    else if(strncmp(&l->content[l->cursor], "![", 2) == 0) {
        lexer_chop_char(l);
        lexer_chop_char(l);
        token.type = TOKEN_ALTTEXT;
        token.text = &l->content[l->cursor];
        while (lexer_chop_char(l) != ']') {
            ASSERT(l->content[l->cursor] != '\n', "Found newline in alt text");
            token.text_len++;
        }
    }

    // Parse link text
    else if(l->content[l->cursor] == '[') {
        lexer_chop_char(l);
        token.type = TOKEN_LINKTEXT;
        token.text = &l->content[l->cursor];
        while (lexer_chop_char(l) != ']') {
            ASSERT(l->content[l->cursor] != '\n', "Found newline in link text");
            token.text_len++;
        }
    }

    // Parse URL
    else if(l->content[l->cursor] == '(' && l->cursor > 0 && l->content[l->cursor-1] == ']') {
        lexer_chop_char(l);
        token.type = TOKEN_URL;
        token.text = &l->content[l->cursor];
        while (lexer_chop_char(l) != ')') {
            ASSERT(l->content[l->cursor] != '\n', "Found newline in URL");
            token.text_len++;
        }
    }

    // Other
    else {
        token.text = &l->content[l->cursor];
        if(token.text[0] == '\n'){
            token.type = TOKEN_NEWLINE;
            token.text_len = 0;
        }else if (token.text[0] == '\\'){
            lexer_chop_char(l);
            token.type = TOKEN_TEXT;
            token.text_len = 1;
            token.text++;
        }else {
            token.type = TOKEN_TEXT;
            token.text_len = 1;
        }
        lexer_chop_char(l);
    }

    return token;
}

void generate_start(MarkdownFunctionMap functionMap[], MarkdownDataType type, MarkdownData *data) {
    for (int i = 0; functionMap[i].type != TYPE_INVALID; i++) {
        if (functionMap[i].type == type) {
            functionMap[i].start(data);
            return;
        }
    }
    ASSERT(0, "No handler found");
}

void generate_end(MarkdownFunctionMap functionMap[], MarkdownDataType type, MarkdownData *data) {
    for (int i = 0; functionMap[i].type != TYPE_INVALID; i++) {
        if (functionMap[i].type == type) {
            functionMap[i].end(data);
            return;
        }
    }
    ASSERT(0, "No handler found");
}

int was_in_text = 0;

void generate(MarkdownFunctionMap fm[], Lexer *l, Token t, Token prev) {
    if(t.type == TOKEN_INVALID) {
        while(l->cursor < l->content_len) {
            Token next = lexer_next(l);
            generate(fm, l, next, t);
            t = next;
        }
        if(was_in_text) generate_end(fm, TYPE_TEXT, NULL);
    }

    else if(t.type == TOKEN_TEXT) {
        if(!was_in_text) {
            generate_start(fm, TYPE_TEXT, NULL);
            was_in_text = 1;
        }
        MarkdownData data = {&t, NULL};
        generate_start(fm, TYPE_RAW, &data);
    }

    else if(t.type == TOKEN_NEWLINE) {
        if(was_in_text) {
            Token tok = {.text = "<br>", .text_len = 4};
            MarkdownData data = {&tok, NULL};
            generate_start(fm, TYPE_RAW, &data);
        }
        else {
            Token tok = {.text = "\n", .text_len = 1};
            MarkdownData data = {&tok, NULL};
            generate_start(fm, TYPE_RAW, &data);
        }
    }

    else if(t.type >= TOKEN_TITLE1 && t.type <= TOKEN_TITLE4) {
        if(was_in_text) generate_end(fm, TYPE_TEXT, NULL);
        was_in_text = 1;
        Token to = t;
        MarkdownData data = {&to, NULL};
        generate_start(fm, TYPE_TITLE, &data);
        prev = t;
        while((t = lexer_next(l)).type != TOKEN_NEWLINE) {
            generate(fm, l, t, prev);
            prev = t;
        }
        generate_end(fm, TYPE_TITLE, &data);
        was_in_text = 0;
    }

    else if(t.type == TOKEN_LIST) {
        if(was_in_text) generate_end(fm, TYPE_TEXT, NULL);
        was_in_text = 1;
        generate_start(fm, TYPE_LIST, NULL);
        prev = t;
        while((t = lexer_next(l)).type != TOKEN_NEWLINE) {
            generate(fm, l, t, prev);
            prev = t;
        }
        was_in_text = 0;
        generate_end(fm, TYPE_LIST, NULL);
    }

    else if(t.type == TOKEN_BLOCK_CODE) {
        if(was_in_text) generate_end(fm, TYPE_TEXT, NULL);
        was_in_text = 0;
        generate_start(fm, TYPE_BLOCK, NULL);
        MarkdownData data = {&t, NULL};
        generate_start(fm, TYPE_RAW, &data);
        generate_end(fm, TYPE_BLOCK, NULL);
    }

    else if(t.type == TOKEN_INLINE_CODE) {
        generate_start(fm, TYPE_INLINE, NULL);
        MarkdownData data = {&t, NULL};
        generate_start(fm, TYPE_RAW, &data);
        generate_end(fm, TYPE_INLINE, NULL);
    }

    else if(t.type == TOKEN_ALTTEXT) {
        Token next = lexer_next(l);
        ASSERT(next.type == TOKEN_URL, "Alt text without URL")
        MarkdownData data = {
            &next, &t
        };
        generate_start(fm, TYPE_IMAGE, &data);
    }

    else if(t.type == TOKEN_LINKTEXT) {
        Token next = lexer_next(l);
        ASSERT(next.type == TOKEN_URL, "Alt text without URL")
        MarkdownData data = {
            &next, &t
        };
        generate_start(fm, TYPE_LINK, &data);
    }
}

#endif