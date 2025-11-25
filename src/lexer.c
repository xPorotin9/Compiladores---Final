// lexer.c
#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Tabla de palabras reservadas
typedef struct {
    const char* keyword;
    TokenType type;
} Keyword;

static Keyword keywords[] = {
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"end", TOKEN_END},
    {"while", TOKEN_WHILE},
    {"loop", TOKEN_LOOP},
    {"fun", TOKEN_FUN},
    {"return", TOKEN_RETURN},
    {"new", TOKEN_NEW},
    {"string", TOKEN_STRING},
    {"int", TOKEN_INT},
    {"char", TOKEN_CHAR},
    {"bool", TOKEN_BOOL},
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
    {"and", TOKEN_AND},
    {"or", TOKEN_OR},
    {"not", TOKEN_NOT},
    {NULL, TOKEN_ERROR}
};

const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_NL: return "NL";
        case TOKEN_LITNUMERAL: return "LITNUMERAL";
        case TOKEN_LITSTRING: return "LITSTRING";
        case TOKEN_ID: return "ID";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_END: return "END";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_LOOP: return "LOOP";
        case TOKEN_FUN: return "FUN";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_NEW: return "NEW";
        case TOKEN_STRING: return "STRING";
        case TOKEN_INT: return "INT";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_BOOL: return "BOOL";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_GT: return "GT";
        case TOKEN_LT: return "LT";
        case TOKEN_GE: return "GE";
        case TOKEN_LE: return "LE";
        case TOKEN_EQ: return "EQ";
        case TOKEN_NE: return "NE";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_COLON: return "COLON";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void lexer_init(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->start_column = 1;
    lexer->had_error = 0;
    lexer->error_message[0] = '\0';
}

static int is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    lexer->column++;
    return *lexer->current++;
}

static char peek(Lexer* lexer) {
    return *lexer->current;
}

static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static int match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return 0;
    if (*lexer->current != expected) return 0;
    lexer->current++;
    lexer->column++;
    return 1;
}

static Token make_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.line = lexer->line;
    token.column = lexer->start_column;
    token.int_value = 0;
    token.string_value = NULL;
    
    int length = (int)(lexer->current - lexer->start);
    token.lexeme = (char*)malloc(length + 1);
    memcpy(token.lexeme, lexer->start, length);
    token.lexeme[length] = '\0';
    
    return token;
}

static Token error_token(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.line = lexer->line;
    token.column = lexer->start_column;
    token.int_value = 0;
    token.string_value = NULL;
    
    token.lexeme = (char*)malloc(strlen(message) + 1);
    strcpy(token.lexeme, message);
    
    lexer->had_error = 1;
    strncpy(lexer->error_message, message, 255);
    lexer->error_message[255] = '\0';
    
    return token;
}

static void skip_whitespace(Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '/':
                if (peek_next(lexer) == '/') {
                    // Comentario de línea //
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                        advance(lexer);
                    }
                } else if (peek_next(lexer) == '*') {
                    // Comentario de bloque /* */
                    advance(lexer); // /
                    advance(lexer); // *
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer); // *
                            advance(lexer); // /
                            break;
                        }
                        if (peek(lexer) == '\n') {
                            lexer->line++;
                            lexer->column = 0;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token scan_string(Lexer* lexer) {
    // Buffer para el string procesado (sin escapes)
    char buffer[4096];
    int buf_pos = 0;
    
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            return error_token(lexer, "String sin terminar");
        }
        
        if (peek(lexer) == '\\') {
            advance(lexer); // consumir '\'
            switch (peek(lexer)) {
                case '\\': buffer[buf_pos++] = '\\'; break;
                case 'n':  buffer[buf_pos++] = '\n'; break;
                case 't':  buffer[buf_pos++] = '\t'; break;
                case '"':  buffer[buf_pos++] = '"';  break;
                default:
                    return error_token(lexer, "Secuencia de escape invalida");
            }
            advance(lexer);
        } else {
            buffer[buf_pos++] = advance(lexer);
        }
    }
    
    if (is_at_end(lexer)) {
        return error_token(lexer, "String sin terminar");
    }
    
    advance(lexer); // Cerrar comilla "
    buffer[buf_pos] = '\0';
    
    Token token = make_token(lexer, TOKEN_LITSTRING);
    token.string_value = (char*)malloc(buf_pos + 1);
    strcpy(token.string_value, buffer);
    
    return token;
}

static Token scan_number(Lexer* lexer) {
    // Verificar si es hexadecimal
    if (lexer->start[0] == '0' && (peek(lexer) == 'x' || peek(lexer) == 'X')) {
        advance(lexer); // consumir 'x'
        
        if (!isxdigit(peek(lexer))) {
            return error_token(lexer, "Numero hexadecimal invalido");
        }
        
        while (isxdigit(peek(lexer))) {
            advance(lexer);
        }
        
        Token token = make_token(lexer, TOKEN_LITNUMERAL);
        token.int_value = (int)strtol(lexer->start, NULL, 16);
        return token;
    }
    
    // Número decimal
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }
    
    Token token = make_token(lexer, TOKEN_LITNUMERAL);
    token.int_value = (int)strtol(lexer->start, NULL, 10);
    return token;
}

static TokenType check_keyword(Lexer* lexer) {
    int length = (int)(lexer->current - lexer->start);
    
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if ((int)strlen(keywords[i].keyword) == length &&
            memcmp(lexer->start, keywords[i].keyword, length) == 0) {
            return keywords[i].type;
        }
    }
    
    return TOKEN_ID;
}

static Token scan_identifier(Lexer* lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }
    
    return make_token(lexer, check_keyword(lexer));
}

Token lexer_next_token(Lexer* lexer) {
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    lexer->start_column = lexer->column;
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }
    
    char c = advance(lexer);
    
    // Salto de línea
    if (c == '\n') {
        // Consumir múltiples saltos de línea consecutivos
        while (peek(lexer) == '\n' || peek(lexer) == '\r' || 
               peek(lexer) == ' ' || peek(lexer) == '\t') {
            if (peek(lexer) == '\n') {
                lexer->line++;
                lexer->column = 0;
            }
            advance(lexer);
        }
        lexer->line++;
        lexer->column = 1;
        return make_token(lexer, TOKEN_NL);
    }
    
    // Identificadores
    if (isalpha(c) || c == '_') {
        return scan_identifier(lexer);
    }
    
    // Números
    if (isdigit(c)) {
        return scan_number(lexer);
    }
    
    // Operadores y puntuación
    switch (c) {
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case ':': return make_token(lexer, TOKEN_COLON);
        case '+': return make_token(lexer, TOKEN_PLUS);
        case '-': return make_token(lexer, TOKEN_MINUS);
        case '*': return make_token(lexer, TOKEN_STAR);
        case '/': return make_token(lexer, TOKEN_SLASH);
        case '=': return make_token(lexer, TOKEN_EQ);
        case '>':
            return make_token(lexer, match(lexer, '=') ? TOKEN_GE : TOKEN_GT);
        case '<':
            if (match(lexer, '=')) return make_token(lexer, TOKEN_LE);
            if (match(lexer, '>')) return make_token(lexer, TOKEN_NE);
            return make_token(lexer, TOKEN_LT);
        case '"':
            return scan_string(lexer);
    }
    
    return error_token(lexer, "Caracter inesperado");
}

void token_free(Token* token) {
    if (token->lexeme != NULL) {
        free(token->lexeme);
        token->lexeme = NULL;
    }
    if (token->string_value != NULL) {
        free(token->string_value);
        token->string_value = NULL;
    }
}

int lexer_had_error(Lexer* lexer) {
    return lexer->had_error;
}