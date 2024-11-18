#include "generator.h"

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