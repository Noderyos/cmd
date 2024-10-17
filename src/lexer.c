#include "lexer.h"

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

    else if(l->content[l->cursor] == '`'){
        lexer_chop_char(l);
        token.type = TOKEN_INLINE_CODE;
        token.text = &l->content[l->cursor];
        while (lexer_chop_char(l) != '`') {
            ASSERT(l->content[l->cursor] != '\n', "Found newline in inline code block");
            token.text_len++;
        }
    }

    // Other
    else {
        token.text = &l->content[l->cursor];
        if(token.text[0] == '\n'){
            token.type = TOKEN_NEWLINE;
            token.text_len = 0;
        }else {
            token.type = TOKEN_TEXT;
            token.text_len = 1;
        }
        lexer_chop_char(l);
    }

    return token;
}
