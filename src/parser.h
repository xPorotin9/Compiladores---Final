#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef struct {
    Lexer* lexer;
    Token current;
    Token previous;
    int had_error;
    int panic_mode;
} Parser;

// Inicializar el parser
void parser_init(Parser* parser, Lexer* lexer);

// Parsear el programa completo
int parser_parse(Parser* parser);

// Liberar recursos del parser
void parser_free(Parser* parser);

#endif