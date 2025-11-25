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

static void error(Parser* parser, const char* message) {
    error_at(parser, &parser->previous, message);
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
        // Si encontramos un NL, podemos intentar continuar
        if (parser->previous.type == TOKEN_NL) return;
        
        // Si encontramos el inicio de una declaración, podemos continuar
        switch (parser->current.type) {
            case TOKEN_FUN:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
            case TOKEN_END:
                return;
            default:
                ; // Continuar avanzando
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
static void cmdreturn(Parser* parser);
static void listaexp(Parser* parser);
static void exp(Parser* parser);
static void exp_and(Parser* parser);
static void exp_rel(Parser* parser);
static void exp_add(Parser* parser);
static void exp_mul(Parser* parser);
static void exp_unary(Parser* parser);
static void exp_postfix(Parser* parser);
static void exp_primary(Parser* parser);
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
    
    // Leer el primer token
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
    // Consumir NL opcionales al inicio
    while (match(parser, TOKEN_NL)) {
        // Consumir saltos de línea iniciales
    }
    
    // Debe haber al menos una declaración
    if (check(parser, TOKEN_EOF)) {
        error_at_current(parser, "El programa esta vacio");
        return;
    }
    
    // Primera declaración obligatoria
    decl(parser);
    
    // Declaraciones adicionales
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
    
    // Tipo de retorno opcional
    if (match(parser, TOKEN_COLON)) {
        tipo(parser);
    }
    
    nl(parser);
    bloque(parser);
    consume(parser, TOKEN_END, "Se esperaba 'end' al final de la funcion");
    nl(parser);
}

// bloque -> { declvar nl } { comando nl }
static void bloque(Parser* parser) {
    // Declaraciones de variables locales
    while (check(parser, TOKEN_ID)) {
        // Necesitamos mirar adelante para distinguir declvar de comando
        // Una declvar tiene la forma ID ':' tipo
        // Un comando puede ser ID '=' exp o ID '(' listaexp ')'
        
        // Guardamos la posición actual
        Token saved = parser->current;
        advance(parser);
        
        if (check(parser, TOKEN_COLON)) {
            // Es una declaración de variable
            // Retrocedemos (simulado: usamos el token guardado)
            parser->current = saved;
            // Necesitamos re-parsear desde el ID
            // En realidad, como ya avanzamos, hacemos un truco:
            // Sabemos que el previous tiene el ID, así que parseamos desde ahí
            
            // Realmente, para este caso, vamos a manejar de forma especial
            // ya que ya consumimos el ID
            consume(parser, TOKEN_COLON, "Se esperaba ':' en declaracion de variable");
            tipo(parser);
            nl(parser);
        } else {
            // No es declaración, retroceder y salir del loop
            // Como no podemos retroceder fácilmente, usamos otro enfoque
            // Vamos a terminar las declaraciones y procesar como comando
            
            // Restaurar: simulamos poniendo current como si no hubiéramos avanzado
            Token temp = parser->current;
            parser->current = saved;
            token_free(&temp);
            break;
        }
    }
    
    // Comandos
    while (!check(parser, TOKEN_END) && 
           !check(parser, TOKEN_ELSE) && 
           !check(parser, TOKEN_LOOP) &&
           !check(parser, TOKEN_EOF)) {
        comando(parser);
        if (!parser->panic_mode) {
            nl(parser);
        }
    }
}

// nl -> NL { NL }
static void nl(Parser* parser) {
    consume(parser, TOKEN_NL, "Se esperaba salto de linea");
    while (match(parser, TOKEN_NL)) {
        // Consumir NL adicionales
    }
}

// params -> /* vacio */ | parametro { ',' parametro }
static void params(Parser* parser) {
    if (check(parser, TOKEN_RPAREN)) {
        return; // Lista vacía
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
        tipo(parser); // Recursivo para [][]int, etc.
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
        cmdreturn(parser);
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
    exp(parser);
    nl(parser);
    bloque(parser);
    
    // else if
    while (check(parser, TOKEN_ELSE)) {
        advance(parser); // consumir 'else'
        
        if (match(parser, TOKEN_IF)) {
            exp(parser);
            nl(parser);
            bloque(parser);
        } else {
            // else final
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
    exp(parser);
    nl(parser);
    bloque(parser);
    consume(parser, TOKEN_LOOP, "Se esperaba 'loop' al final del while");
}

// cmdreturn -> 'return' [exp]
static void cmdreturn(Parser* parser) {
    consume(parser, TOKEN_RETURN, "Se esperaba 'return'");
    
    // Expresión opcional (si no hay NL inmediatamente)
    if (!check(parser, TOKEN_NL) && !check(parser, TOKEN_EOF)) {
        exp(parser);
    }
}

// cmdatrib_o_llamada -> ID ( '(' listaexp ')' | var_suffix '=' exp )
static void cmdatrib_o_llamada(Parser* parser) {
    consume(parser, TOKEN_ID, "Se esperaba identificador");
    
    if (match(parser, TOKEN_LPAREN)) {
        // Es una llamada a función
        listaexp(parser);
        consume(parser, TOKEN_RPAREN, "Se esperaba ')' en llamada a funcion");
    } else {
        // Es una asignación: var_suffix '=' exp
        // var_suffix -> { '[' exp ']' }
        while (match(parser, TOKEN_LBRACKET)) {
            exp(parser);
            consume(parser, TOKEN_RBRACKET, "Se esperaba ']'");
        }
        
        consume(parser, TOKEN_EQ, "Se esperaba '=' en asignacion");
        exp(parser);
    }
}

// listaexp -> /* vacio */ | exp { ',' exp }
static void listaexp(Parser* parser) {
    if (check(parser, TOKEN_RPAREN)) {
        return; // Lista vacía
    }
    
    exp(parser);
    
    while (match(parser, TOKEN_COMMA)) {
        exp(parser);
    }
}

// ==================== EXPRESIONES CON PRECEDENCIA ====================

// exp -> exp_and { 'or' exp_and }
static void exp(Parser* parser) {
    exp_and(parser);
    
    while (match(parser, TOKEN_OR)) {
        exp_and(parser);
    }
}

// exp_and -> exp_rel { 'and' exp_rel }
static void exp_and(Parser* parser) {
    exp_rel(parser);
    
    while (match(parser, TOKEN_AND)) {
        exp_rel(parser);
    }
}

// exp_rel -> exp_add [ op_rel exp_add ]
static void exp_rel(Parser* parser) {
    exp_add(parser);
    
    // Operadores relacionales (no asociativos, solo uno)
    if (match(parser, TOKEN_GT) || 
        match(parser, TOKEN_LT) || 
        match(parser, TOKEN_GE) || 
        match(parser, TOKEN_LE) || 
        match(parser, TOKEN_EQ) || 
        match(parser, TOKEN_NE)) {
        exp_add(parser);
    }
}

// exp_add -> exp_mul { ('+' | '-') exp_mul }
static void exp_add(Parser* parser) {
    exp_mul(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        exp_mul(parser);
    }
}

// exp_mul -> exp_unary { ('*' | '/') exp_unary }
static void exp_mul(Parser* parser) {
    exp_unary(parser);
    
    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH)) {
        exp_unary(parser);
    }
}

// exp_unary -> 'not' exp_unary | '-' exp_unary | exp_postfix
static void exp_unary(Parser* parser) {
    if (match(parser, TOKEN_NOT) || match(parser, TOKEN_MINUS)) {
        exp_unary(parser); // Recursivo para not not x, --x, etc.
    } else {
        exp_postfix(parser);
    }
}

// exp_postfix -> exp_primary { '[' exp ']' }
static void exp_postfix(Parser* parser) {
    exp_primary(parser);
    
    while (match(parser, TOKEN_LBRACKET)) {
        exp(parser);
        consume(parser, TOKEN_RBRACKET, "Se esperaba ']'");
    }
}

// exp_primary -> LITNUMERAL | LITSTRING | 'true' | 'false' 
//              | 'new' '[' exp ']' tipo | '(' exp ')' | ID ['(' listaexp ')']
static void exp_primary(Parser* parser) {
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
        exp(parser);
        consume(parser, TOKEN_RBRACKET, "Se esperaba ']' en expresion new");
        tipo(parser);
        return;
    }
    
    if (match(parser, TOKEN_LPAREN)) {
        exp(parser);
        consume(parser, TOKEN_RPAREN, "Se esperaba ')'");
        return;
    }
    
    if (match(parser, TOKEN_ID)) {
        // Puede ser variable o llamada a función
        if (match(parser, TOKEN_LPAREN)) {
            listaexp(parser);
            consume(parser, TOKEN_RPAREN, "Se esperaba ')' en llamada a funcion");
        }
        return;
    }
    
    error_at_current(parser, "Se esperaba una expresion");
    advance(parser);
}