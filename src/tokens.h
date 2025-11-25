// tokens.h
#ifndef TOKENS_H
#define TOKENS_H

typedef enum {
    // Fin de archivo y salto de línea
    TOKEN_EOF = 0,
    TOKEN_NL,           // Newline (relevante para la gramática)
    
    // Literales
    TOKEN_LITNUMERAL,   // Números enteros (decimal o hex)
    TOKEN_LITSTRING,    // Strings "..."
    TOKEN_ID,           // Identificadores
    
    // Palabras reservadas
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_END,
    TOKEN_WHILE,
    TOKEN_LOOP,
    TOKEN_FUN,
    TOKEN_RETURN,
    TOKEN_NEW,
    TOKEN_STRING,
    TOKEN_INT,
    TOKEN_CHAR,
    TOKEN_BOOL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    
    // Operadores aritméticos
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_STAR,         // *
    TOKEN_SLASH,        // /
    
    // Operadores relacionales
    TOKEN_GT,           // >
    TOKEN_LT,           // <
    TOKEN_GE,           // >=
    TOKEN_LE,           // <=
    TOKEN_EQ,           // =
    TOKEN_NE,           // <>
    
    // Signos de puntuación
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_LBRACKET,     // [
    TOKEN_RBRACKET,     // ]
    TOKEN_COMMA,        // ,
    TOKEN_COLON,        // :
    
    // Error
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char* lexeme;       // Texto del token
    int line;           // Línea donde aparece
    int column;         // Columna donde aparece
    
    // Para literales numéricos
    int int_value;
    
    // Para strings
    char* string_value;
} Token;

// Función para obtener el nombre del token (para debugging)
const char* token_type_name(TokenType type);

#endif