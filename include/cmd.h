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
} cmd_token_type;

typedef struct {
    cmd_token_type type;
    const char *text;
    size_t text_len;
    size_t row;
    size_t col;
} cmd_token;

typedef struct {
    char *content;
    size_t content_len;
    size_t cursor;
    size_t line;
    size_t bol;
} cmd_lexer;

cmd_lexer lexer_init(char *filename);
cmd_token lexer_next(cmd_lexer *l);

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
} cmd_md_dt;

typedef struct {
    cmd_token *content;
    cmd_token *alt;
} cmd_md_data;

typedef void (*cmd_md_handler)(const cmd_md_data *data);

typedef struct {
    const cmd_md_dt type;
    cmd_md_handler start;
    cmd_md_handler end;
} cmd_md_func_map;

void cmd_generate(cmd_md_func_map fm[], cmd_lexer *l);

#endif

#ifdef CMD_IMPLEMENTATION
#undef CMD_IMPLEMENTATION

void cmd_generate_internal(cmd_md_func_map fm[], cmd_lexer *l, cmd_token t, cmd_token prev);

void cmd_generate(cmd_md_func_map fm[], cmd_lexer *l) {
    cmd_token t = {.type = TOKEN_INVALID};
    cmd_generate_internal(fm, l, t, t);
}


char *read_file(const char *filename){
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;

    return string;
}

cmd_lexer lexer_init(char *filename){
    cmd_lexer l = {0};
    l.content = read_file(filename);
    l.content_len = strlen(l.content);
    return l;
}

char lexer_chop_char(cmd_lexer *l){
    ASSERT(l->cursor < l->content_len, "Unexpected EOF")

    char x = l->content[l->cursor];
    l->cursor++;
    if(x == '\n'){
        l->line++;
        l->bol = l->cursor;
    }
    return x;
}

cmd_token lexer_next(cmd_lexer *l) {
    cmd_token token = {
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

void generate_start(cmd_md_func_map functionMap[], cmd_md_dt type, cmd_md_data *data) {
    for (int i = 0; functionMap[i].type != TYPE_INVALID; i++) {
        if (functionMap[i].type == type) {
            functionMap[i].start(data);
            return;
        }
    }
    ASSERT(0, "No handler found");
}

void generate_end(cmd_md_func_map functionMap[], cmd_md_dt type, cmd_md_data *data) {
    for (int i = 0; functionMap[i].type != TYPE_INVALID; i++) {
        if (functionMap[i].type == type) {
            functionMap[i].end(data);
            return;
        }
    }
    ASSERT(0, "No handler found");
}

int was_in_text = 0;

void cmd_generate_internal(cmd_md_func_map fm[], cmd_lexer *l, cmd_token t, cmd_token prev) {
    if(t.type == TOKEN_INVALID) {
        while(l->cursor < l->content_len) {
            cmd_token next = lexer_next(l);
            cmd_generate_internal(fm, l, next, t);
            t = next;
        }
        if(was_in_text) generate_end(fm, TYPE_TEXT, NULL);
    }

    else if(t.type == TOKEN_TEXT) {
        if(!was_in_text) {
            generate_start(fm, TYPE_TEXT, NULL);
            was_in_text = 1;
        }
        cmd_md_data data = {&t, NULL};
        generate_start(fm, TYPE_RAW, &data);
    }

    else if(t.type == TOKEN_NEWLINE) {
        if(was_in_text) {
            cmd_token tok = {.text = "<br>", .text_len = 4};
            cmd_md_data data = {&tok, NULL};
            generate_start(fm, TYPE_RAW, &data);
        }
        else {
            cmd_token tok = {.text = "\n", .text_len = 1};
            cmd_md_data data = {&tok, NULL};
            generate_start(fm, TYPE_RAW, &data);
        }
    }

    else if(t.type >= TOKEN_TITLE1 && t.type <= TOKEN_TITLE4) {
        if(was_in_text) generate_end(fm, TYPE_TEXT, NULL);
        was_in_text = 1;
        cmd_token to = t;
        cmd_md_data data = {&to, NULL};
        generate_start(fm, TYPE_TITLE, &data);
        prev = t;
        while((t = lexer_next(l)).type != TOKEN_NEWLINE) {
            cmd_generate_internal(fm, l, t, prev);
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
            cmd_generate_internal(fm, l, t, prev);
            prev = t;
        }
        was_in_text = 0;
        generate_end(fm, TYPE_LIST, NULL);
    }

    else if(t.type == TOKEN_BLOCK_CODE) {
        if(was_in_text) generate_end(fm, TYPE_TEXT, NULL);
        was_in_text = 0;
        generate_start(fm, TYPE_BLOCK, NULL);
        cmd_md_data data = {&t, NULL};
        generate_start(fm, TYPE_RAW, &data);
        generate_end(fm, TYPE_BLOCK, NULL);
    }

    else if(t.type == TOKEN_INLINE_CODE) {
        generate_start(fm, TYPE_INLINE, NULL);
        cmd_md_data data = {&t, NULL};
        generate_start(fm, TYPE_RAW, &data);
        generate_end(fm, TYPE_INLINE, NULL);
    }

    else if(t.type == TOKEN_ALTTEXT) {
        cmd_token next = lexer_next(l);
        ASSERT(next.type == TOKEN_URL, "Alt text without URL")
        cmd_md_data data = {
            &next, &t
        };
        generate_start(fm, TYPE_IMAGE, &data);
    }

    else if(t.type == TOKEN_LINKTEXT) {
        cmd_token next = lexer_next(l);
        ASSERT(next.type == TOKEN_URL, "Alt text without URL")
        cmd_md_data data = {
            &next, &t
        };
        generate_start(fm, TYPE_LINK, &data);
    }
}

#endif