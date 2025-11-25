// lexer.h
#ifndef LEXER_H
#define LEXER_H

#include "tokens.h"
#include <stdio.h>

typedef struct {
    const char* source;     // Código fuente completo
    const char* start;      // Inicio del token actual
    const char* current;    // Posición actual
    int line;               // Línea actual
    int column;             // Columna actual
    int start_column;       // Columna del inicio del token
    
    // Para manejo de errores
    int had_error;
    char error_message[256];
} Lexer;

// Inicializar el lexer con el código fuente
void lexer_init(Lexer* lexer, const char* source);

// Obtener el siguiente token
Token lexer_next_token(Lexer* lexer);

// Liberar memoria de un token
void token_free(Token* token);

// Verificar si hubo error léxico
int lexer_had_error(Lexer* lexer);

#endif