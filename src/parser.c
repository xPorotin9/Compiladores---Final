// parser.c
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==================== UTILIDADES ====================

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = 1;
    parser->had_error = 1;
    
    fprintf(stderr, "[Linea %d, Columna %d] Error", token->line, token->column);
    
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " al final del archivo");
    } else if (token->type == TOKEN_ERROR) {
        // El mensaje ya está en el lexema
    } else {
        fprintf(stderr, " en '%s'", token->lexeme);
    }
    
    fprintf(stderr, ": %s\n", message);
}

static void error_at_current(Parser* parser, const char* message) {
    error_at(parser, &parser->current, message);
}

static void advance(Parser* parser) {
    token_free(&parser->previous);
    parser->previous = parser->current;
    
    for (;;) {
        parser->current = lexer_next_token(parser->lexer);
        
        if (parser->current.type != TOKEN_ERROR) break;
        
        error_at_current(parser, parser->current.lexeme);
    }
}

static int check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static int match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return 0;
    advance(parser);
    return 1;
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    
    error_at_current(parser, message);
}

// Sincronización para recuperación de errores
static void synchronize(Parser* parser) {
    parser->panic_mode = 0;
    
    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_NL) return;
        
        switch (parser->current.type) {
            case TOKEN_FUN:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
            case TOKEN_END:
                return;
            default:
                ;
        }
        
        advance(parser);
    }
}

// ==================== DECLARACIONES FORWARD ====================

static void programa(Parser* parser);
static void decl(Parser* parser);
static void funcion(Parser* parser);
static void global_decl(Parser* parser);
static void bloque(Parser* parser);
static void params(Parser* parser);
static void parametro(Parser* parser);
static void tipo(Parser* parser);
static void tipobase(Parser* parser);
static void declvar(Parser* parser);
static void comando(Parser* parser);
static void cmdif(Parser* parser);
static void cmdwhile(Parser* parser);
static void cmdatrib_o_llamada(Parser* parser);
static void cmdreturn_stmt(Parser* parser);
static void listaexp(Parser* parser);

// Expresiones - renombradas para evitar conflicto con math.h
static void expression(Parser* parser);
static void expr_or(Parser* parser);
static void expr_and(Parser* parser);
static void expr_rel(Parser* parser);
static void expr_add(Parser* parser);
static void expr_mul(Parser* parser);
static void expr_unary(Parser* parser);
static void expr_postfix(Parser* parser);
static void expr_primary(Parser* parser);

static void nl(Parser* parser);

// ==================== IMPLEMENTACIÓN ====================

void parser_init(Parser* parser, Lexer* lexer) {
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    parser->previous.lexeme = NULL;
    parser->previous.string_value = NULL;
    parser->current.lexeme = NULL;
    parser->current.string_value = NULL;
    
    advance(parser);
}

int parser_parse(Parser* parser) {
    programa(parser);
    consume(parser, TOKEN_EOF, "Se esperaba fin de archivo");
    return !parser->had_error;
}

void parser_free(Parser* parser) {
    token_free(&parser->previous);
    token_free(&parser->current);
}

// programa -> { NL } decl { decl }
static void programa(Parser* parser) {
    while (match(parser, TOKEN_NL)) {
        // Consumir saltos de línea iniciales
    }
    
    if (check(parser, TOKEN_EOF)) {
        error_at_current(parser, "El programa esta vacio");
        return;
    }
    
    decl(parser);
    
    while (!check(parser, TOKEN_EOF) && !parser->had_error) {
        decl(parser);
    }
}

// decl -> funcion | global
static void decl(Parser* parser) {
    if (parser->panic_mode) synchronize(parser);
    
    if (check(parser, TOKEN_FUN)) {
        funcion(parser);
    } else if (check(parser, TOKEN_ID)) {
        global_decl(parser);
    } else {
        error_at_current(parser, "Se esperaba una declaracion (funcion o variable)");
        advance(parser);
    }
}

// global -> declvar nl
static void global_decl(Parser* parser) {
    declvar(parser);
    nl(parser);
}

// funcion -> 'fun' ID '(' params ')' [':' tipo] nl bloque 'end' nl
static void funcion(Parser* parser) {
    consume(parser, TOKEN_FUN, "Se esperaba 'fun'");
    consume(parser, TOKEN_ID, "Se esperaba nombre de funcion");
    consume(parser, TOKEN_LPAREN, "Se esperaba '(' despues del nombre de funcion");
    params(parser);
    consume(parser, TOKEN_RPAREN, "Se esperaba ')' despues de los parametros");
    
    if (match(parser, TOKEN_COLON)) {
        tipo(parser);
    }
    
    nl(parser);
    bloque(parser);
    consume(parser, TOKEN_END, "Se esperaba 'end' al final de la funcion");
    nl(parser);
}


// Función auxiliar para verificar si estamos al final de un bloque
static int is_block_end(Parser* parser) {
    return check(parser, TOKEN_END) ||
           check(parser, TOKEN_ELSE) ||
           check(parser, TOKEN_LOOP) ||
           check(parser, TOKEN_EOF);
}

// bloque -> { declvar nl } { comando nl }
static void bloque(Parser* parser) {
    // Primero procesamos declaraciones de variables
    // Una declaración de variable es: ID ':' tipo
    // Un comando que empieza con ID puede ser: ID '=' exp o ID '(' listaexp ')'
    
    while (check(parser, TOKEN_ID) && !is_block_end(parser)) {
        // Guardamos información del token actual para lookahead
        // Necesitamos ver si después del ID viene ':'
        
        // Avanzamos para ver qué sigue
        advance(parser); // Consumimos el ID, ahora está en previous
        
        if (check(parser, TOKEN_COLON)) {
            // Es una declaración de variable: ID ':' tipo
            advance(parser); // Consumir ':'
            tipo(parser);
            nl(parser);
        } else {
            // No es declaración, es un comando
            // El ID ya está en previous, necesitamos procesarlo como comando
            // Pero ya consumimos el ID, así que procesamos el resto aquí
            
            if (match(parser, TOKEN_LPAREN)) {
                // Es una llamada a función: ID '(' listaexp ')'
                listaexp(parser);
                consume(parser, TOKEN_RPAREN, "Se esperaba ')' en llamada a funcion");
            } else {
                // Es una asignación: ID var_suffix '=' exp
                // var_suffix -> { '[' exp ']' }
                while (match(parser, TOKEN_LBRACKET)) {
                    expression(parser);
                    consume(parser, TOKEN_RBRACKET, "Se esperaba ']'");
                }
                consume(parser, TOKEN_EQ, "Se esperaba '=' en asignacion");
                expression(parser);
            }
            nl(parser);
            
            // Ahora procesamos el resto de los comandos normalmente
            break;
        }
    }
    
    // Procesamos los comandos restantes
    while (!is_block_end(parser) && !parser->had_error) {
        comando(parser);
        if (!parser->panic_mode) {
            nl(parser);
        }
    }
}

// nl -> NL { NL } | EOF
static void nl(Parser* parser) {
    // Si estamos al final del archivo, no exigir salto de línea
    if (check(parser, TOKEN_EOF)) {
        return;
    }
    
    // Si hay salto de línea, consumirlo
    if (match(parser, TOKEN_NL)) {
        // Consumir saltos de línea adicionales
        while (match(parser, TOKEN_NL)) {
            // Continuar
        }
        return;
    }
    
    // Si no hay salto de línea ni EOF, es un error
    error_at_current(parser, "Se esperaba salto de linea");
}
// params -> /* vacio */ | parametro { ',' parametro }
static void params(Parser* parser) {
    if (check(parser, TOKEN_RPAREN)) {
        return;
    }
    
    parametro(parser);
    
    while (match(parser, TOKEN_COMMA)) {
        parametro(parser);
    }
}

// parametro -> ID ':' tipo
static void parametro(Parser* parser) {
    consume(parser, TOKEN_ID, "Se esperaba nombre de parametro");
    consume(parser, TOKEN_COLON, "Se esperaba ':' despues del nombre de parametro");
    tipo(parser);
}

// tipo -> tipobase | '[' ']' tipo
static void tipo(Parser* parser) {
    if (match(parser, TOKEN_LBRACKET)) {
        consume(parser, TOKEN_RBRACKET, "Se esperaba ']' para tipo arreglo");
        tipo(parser);
    } else {
        tipobase(parser);
    }
}

// tipobase -> 'int' | 'bool' | 'char' | 'string'
static void tipobase(Parser* parser) {
    if (match(parser, TOKEN_INT) || 
        match(parser, TOKEN_BOOL) || 
        match(parser, TOKEN_CHAR) || 
        match(parser, TOKEN_STRING)) {
        return;
    }
    
    error_at_current(parser, "Se esperaba un tipo (int, bool, char, string)");
    advance(parser);
}

// declvar -> ID ':' tipo
static void declvar(Parser* parser) {
    consume(parser, TOKEN_ID, "Se esperaba nombre de variable");
    consume(parser, TOKEN_COLON, "Se esperaba ':' en declaracion");
    tipo(parser);
}

// comando -> cmdif | cmdwhile | cmdatrib_o_llamada | cmdreturn
static void comando(Parser* parser) {
    if (check(parser, TOKEN_IF)) {
        cmdif(parser);
    } else if (check(parser, TOKEN_WHILE)) {
        cmdwhile(parser);
    } else if (check(parser, TOKEN_RETURN)) {
        cmdreturn_stmt(parser);
    } else if (check(parser, TOKEN_ID)) {
        cmdatrib_o_llamada(parser);
    } else {
        error_at_current(parser, "Se esperaba un comando");
        advance(parser);
    }
}

// cmdif -> 'if' exp nl bloque { 'else' 'if' exp nl bloque } ['else' nl bloque] 'end'
static void cmdif(Parser* parser) {
    consume(parser, TOKEN_IF, "Se esperaba 'if'");
    expression(parser);
    nl(parser);
    bloque(parser);
    
    while (check(parser, TOKEN_ELSE)) {
        advance(parser); // consumir 'else'
        
        if (match(parser, TOKEN_IF)) {
            expression(parser);
            nl(parser);
            bloque(parser);
        } else {
            nl(parser);
            bloque(parser);
            break;
        }
    }
    
    consume(parser, TOKEN_END, "Se esperaba 'end' al final del if");
}

// cmdwhile -> 'while' exp nl bloque 'loop'
static void cmdwhile(Parser* parser) {
    consume(parser, TOKEN_WHILE, "Se esperaba 'while'");
    expression(parser);
    nl(parser);
    bloque(parser);
    consume(parser, TOKEN_LOOP, "Se esperaba 'loop' al final del while");
}

// cmdreturn -> 'return' [exp]
static void cmdreturn_stmt(Parser* parser) {
    consume(parser, TOKEN_RETURN, "Se esperaba 'return'");
    
    if (!check(parser, TOKEN_NL) && !check(parser, TOKEN_EOF)) {
        expression(parser);
    }
}

// cmdatrib_o_llamada -> ID ( '(' listaexp ')' | var_suffix '=' exp )
static void cmdatrib_o_llamada(Parser* parser) {
    consume(parser, TOKEN_ID, "Se esperaba identificador");
    
    if (match(parser, TOKEN_LPAREN)) {
        listaexp(parser);
        consume(parser, TOKEN_RPAREN, "Se esperaba ')' en llamada a funcion");
    } else {
        while (match(parser, TOKEN_LBRACKET)) {
            expression(parser);
            consume(parser, TOKEN_RBRACKET, "Se esperaba ']'");
        }
        
        consume(parser, TOKEN_EQ, "Se esperaba '=' en asignacion");
        expression(parser);
    }
}

// listaexp -> /* vacio */ | exp { ',' exp }
static void listaexp(Parser* parser) {
    if (check(parser, TOKEN_RPAREN)) {
        return;
    }
    
    expression(parser);
    
    while (match(parser, TOKEN_COMMA)) {
        expression(parser);
    }
}

// ==================== EXPRESIONES CON PRECEDENCIA ====================

// expression -> expr_or
static void expression(Parser* parser) {
    expr_or(parser);
}

// expr_or -> expr_and { 'or' expr_and }
static void expr_or(Parser* parser) {
    expr_and(parser);
    
    while (match(parser, TOKEN_OR)) {
        expr_and(parser);
    }
}

// expr_and -> expr_rel { 'and' expr_rel }
static void expr_and(Parser* parser) {
    expr_rel(parser);
    
    while (match(parser, TOKEN_AND)) {
        expr_rel(parser);
    }
}

// expr_rel -> expr_add [ op_rel expr_add ]
static void expr_rel(Parser* parser) {
    expr_add(parser);
    
    if (match(parser, TOKEN_GT) || 
        match(parser, TOKEN_LT) || 
        match(parser, TOKEN_GE) || 
        match(parser, TOKEN_LE) || 
        match(parser, TOKEN_EQ) || 
        match(parser, TOKEN_NE)) {
        expr_add(parser);
    }
}

// expr_add -> expr_mul { ('+' | '-') expr_mul }
static void expr_add(Parser* parser) {
    expr_mul(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        expr_mul(parser);
    }
}

// expr_mul -> expr_unary { ('*' | '/') expr_unary }
static void expr_mul(Parser* parser) {
    expr_unary(parser);
    
    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH)) {
        expr_unary(parser);
    }
}

// expr_unary -> 'not' expr_unary | '-' expr_unary | expr_postfix
static void expr_unary(Parser* parser) {
    if (match(parser, TOKEN_NOT) || match(parser, TOKEN_MINUS)) {
        expr_unary(parser);
    } else {
        expr_postfix(parser);
    }
}

// expr_postfix -> expr_primary { '[' expression ']' }
static void expr_postfix(Parser* parser) {
    expr_primary(parser);
    
    while (match(parser, TOKEN_LBRACKET)) {
        expression(parser);
        consume(parser, TOKEN_RBRACKET, "Se esperaba ']'");
    }
}

// expr_primary -> LITNUMERAL | LITSTRING | 'true' | 'false' 
//               | 'new' '[' exp ']' tipo | '(' exp ')' | ID ['(' listaexp ')']
static void expr_primary(Parser* parser) {
    if (match(parser, TOKEN_LITNUMERAL)) {
        return;
    }
    
    if (match(parser, TOKEN_LITSTRING)) {
        return;
    }
    
    if (match(parser, TOKEN_TRUE) || match(parser, TOKEN_FALSE)) {
        return;
    }
    
    if (match(parser, TOKEN_NEW)) {
        consume(parser, TOKEN_LBRACKET, "Se esperaba '[' despues de 'new'");
        expression(parser);
        consume(parser, TOKEN_RBRACKET, "Se esperaba ']' en expresion new");
        tipo(parser);
        return;
    }
    
    if (match(parser, TOKEN_LPAREN)) {
        expression(parser);
        consume(parser, TOKEN_RPAREN, "Se esperaba ')'");
        return;
    }
    
    if (match(parser, TOKEN_ID)) {
        if (match(parser, TOKEN_LPAREN)) {
            listaexp(parser);
            consume(parser, TOKEN_RPAREN, "Se esperaba ')' en llamada a funcion");
        }
        return;
    }
    
    error_at_current(parser, "Se esperaba una expresion");
    advance(parser);
}