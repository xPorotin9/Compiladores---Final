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

// ... resto del código del parser que ya te di

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

// Verificar si estamos al final de un bloque
static int is_block_end(Parser* parser) {
    return check(parser, TOKEN_END) ||
           check(parser, TOKEN_ELSE) ||
           check(parser, TOKEN_LOOP) ||
           check(parser, TOKEN_EOF);
}

// ==================== DECLARACIONES FORWARD ====================

static void programa(Parser* parser);
static void decl(Parser* parser);
static void funcion(Parser* parser);
static void global_decl(Parser* parser);
static void bloque(Parser* parser);
static void statement(Parser* parser);          // NUEVA - LL1
static void statement_suffix(Parser* parser);   // NUEVA - LL1
static void params(Parser* parser);
static void parametro(Parser* parser);
static void tipo(Parser* parser);
static void tipobase(Parser* parser);
static void declvar(Parser* parser);
static void comando(Parser* parser);
static void cmdif(Parser* parser);
static void cmdwhile(Parser* parser);
static void cmdreturn_stmt(Parser* parser);
static void listaexp(Parser* parser);

// Expresiones con comentarios LL1
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

// ==================== IMPLEMENTACIÓN LL1 ====================

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

// ===== TABLA LL1: No-terminal 'programa' =====
// LL1[programa, NL] = programa → NL programa
// LL1[programa, fun] = programa → decl programa  
// LL1[programa, ID] = programa → decl programa
// LL1[programa, EOF] = programa → ε
static void programa(Parser* parser) {
    // Consumir NL opcionales al inicio
    while (match(parser, TOKEN_NL)) {
        // LL1[programa, NL] = NL programa
    }
    
    if (check(parser, TOKEN_EOF)) {
        // LL1[programa, EOF] = ε
        return;
    }
    
    // LL1[programa, fun|ID] = decl programa
    decl(parser);
    
    while (!check(parser, TOKEN_EOF) && !parser->had_error) {
        decl(parser);
    }
}

// ===== TABLA LL1: No-terminal 'decl' =====
// LL1[decl, fun] = decl → funcion
// LL1[decl, ID] = decl → global
static void decl(Parser* parser) {
    if (parser->panic_mode) synchronize(parser);
    
    if (check(parser, TOKEN_FUN)) {
        // LL1[decl, fun] = decl → funcion
        funcion(parser);
    } else if (check(parser, TOKEN_ID)) {
        // LL1[decl, ID] = decl → global
        global_decl(parser);
    } else {
        error_at_current(parser, "Se esperaba una declaracion (funcion o variable)");
        advance(parser);
    }
}

// ===== TABLA LL1: No-terminal 'global' =====
// LL1[global, ID] = global → declvar nl
static void global_decl(Parser* parser) {
    // LL1[global, ID] = declvar nl
    declvar(parser);
    nl(parser);
}

// ===== TABLA LL1: No-terminal 'funcion' =====
// LL1[funcion, fun] = funcion → 'fun' ID '(' params ')' [':' tipo] nl bloque 'end' nl
static void funcion(Parser* parser) {
    // LL1[funcion, fun] = 'fun' ID '(' params ')' [':' tipo] nl bloque 'end' nl
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

// ===== TABLA LL1: No-terminal 'bloque' (REFACTORIZADO) =====
// LL1[bloque, ID] = bloque → statement nl bloque
// LL1[bloque, if] = bloque → comando nl bloque  
// LL1[bloque, while] = bloque → comando nl bloque
// LL1[bloque, return] = bloque → comando nl bloque
// LL1[bloque, end|else|loop] = bloque → ε
static void bloque(Parser* parser) {
    while (!is_block_end(parser) && !parser->had_error) {
        if (check(parser, TOKEN_ID)) {
            // LL1[bloque, ID] = statement nl bloque
            statement(parser);
            nl(parser);
        } else if (check(parser, TOKEN_IF) || 
                   check(parser, TOKEN_WHILE) || 
                   check(parser, TOKEN_RETURN)) {
            // LL1[bloque, if|while|return] = comando nl bloque
            comando(parser);
            nl(parser);
        } else {
            // LL1[bloque, end|else|loop] = ε
            break;
        }
    }
}

// ===== NUEVA FUNCIÓN LL1: No-terminal 'statement' =====
// LL1[statement, ID] = statement → ID statement_suffix
static void statement(Parser* parser) {
    // LL1[statement, ID] = ID statement_suffix
    consume(parser, TOKEN_ID, "Se esperaba identificador");
    statement_suffix(parser);
}

// ===== NUEVA FUNCIÓN LL1: No-terminal 'statement_suffix' =====
// LL1[statement_suffix, :] = statement_suffix → ':' tipo
// LL1[statement_suffix, =] = statement_suffix → array_indices '=' expression
// LL1[statement_suffix, [] = statement_suffix → array_indices '=' expression  
// LL1[statement_suffix, (] = statement_suffix → '(' listaexp ')'
static void statement_suffix(Parser* parser) {
    if (match(parser, TOKEN_COLON)) {
        // LL1[statement_suffix, :] = ':' tipo (declaración de variable)
        tipo(parser);
    } else if (match(parser, TOKEN_LBRACKET)) {
        // LL1[statement_suffix, [] = '[' expression ']' ... '=' expression (asignación array)
        expression(parser);
        consume(parser, TOKEN_RBRACKET, "Se esperaba ']'");
        
        // Múltiples índices: arr[i][j] = valor
        while (match(parser, TOKEN_LBRACKET)) {
            expression(parser);
            consume(parser, TOKEN_RBRACKET, "Se esperaba ']'");
        }
        
        consume(parser, TOKEN_EQ, "Se esperaba '=' en asignacion");
        expression(parser);
    } else if (match(parser, TOKEN_EQ)) {
        // LL1[statement_suffix, =] = '=' expression (asignación simple)
        expression(parser);
    } else if (match(parser, TOKEN_LPAREN)) {
        // LL1[statement_suffix, (] = '(' listaexp ')' (llamada función)
        listaexp(parser);
        consume(parser, TOKEN_RPAREN, "Se esperaba ')' en llamada a funcion");
    } else {
        error_at_current(parser, "Se esperaba ':', '=', '[' o '(' despues del identificador");
    }
}

// Función auxiliar para verificar si un salto de línea es opcional
static int nl_is_optional_here(Parser* parser) {
    if (check(parser, TOKEN_EOF)) return 1;
    if (check(parser, TOKEN_END) || 
        check(parser, TOKEN_ELSE) || 
        check(parser, TOKEN_LOOP)) return 1;
    return 0;
}

// ===== TABLA LL1: No-terminal 'nl' =====
// LL1[nl, NL] = nl → NL { NL }
// LL1[nl, EOF|end|else|loop] = nl → ε (opcional)
static void nl(Parser* parser) {
    if (nl_is_optional_here(parser)) {
        // LL1[nl, EOF|end|else|loop] = ε
        while (match(parser, TOKEN_NL)) {
            // Consumir si están presentes
        }
        return;
    }
    
    // LL1[nl, NL] = NL { NL }
    consume(parser, TOKEN_NL, "Se esperaba salto de linea");
    while (match(parser, TOKEN_NL)) {
        // Consumir NL adicionales
    }
}

// ===== TABLA LL1: No-terminal 'params' =====
// LL1[params, )] = params → ε  
// LL1[params, ID] = params → parametro { ',' parametro }
static void params(Parser* parser) {
    if (check(parser, TOKEN_RPAREN)) {
        // LL1[params, )] = ε
        return;
    }
    
    // LL1[params, ID] = parametro { ',' parametro }
    parametro(parser);
    
    while (match(parser, TOKEN_COMMA)) {
        parametro(parser);
    }
}

// ===== TABLA LL1: No-terminal 'parametro' =====
// LL1[parametro, ID] = parametro → ID ':' tipo
static void parametro(Parser* parser) {
    // LL1[parametro, ID] = ID ':' tipo
    consume(parser, TOKEN_ID, "Se esperaba nombre de parametro");
    consume(parser, TOKEN_COLON, "Se esperaba ':' despues del nombre de parametro");
    tipo(parser);
}

// ===== TABLA LL1: No-terminal 'tipo' =====
// LL1[tipo, [] = tipo → '[' ']' tipo
// LL1[tipo, int|bool|char|string] = tipo → tipobase
static void tipo(Parser* parser) {
    if (match(parser, TOKEN_LBRACKET)) {
        // LL1[tipo, [] = '[' ']' tipo
        consume(parser, TOKEN_RBRACKET, "Se esperaba ']' para tipo arreglo");
        tipo(parser);
    } else {
        // LL1[tipo, int|bool|char|string] = tipobase
        tipobase(parser);
    }
}

// ===== TABLA LL1: No-terminal 'tipobase' =====
// LL1[tipobase, int] = tipobase → 'int'
// LL1[tipobase, bool] = tipobase → 'bool'  
// LL1[tipobase, char] = tipobase → 'char'
// LL1[tipobase, string] = tipobase → 'string'
static void tipobase(Parser* parser) {
    if (match(parser, TOKEN_INT) || 
        match(parser, TOKEN_BOOL) || 
        match(parser, TOKEN_CHAR) || 
        match(parser, TOKEN_STRING)) {
        // LL1[tipobase, int|bool|char|string] = respectivo token
        return;
    }
    
    error_at_current(parser, "Se esperaba un tipo (int, bool, char, string)");
    advance(parser);
}

// ===== TABLA LL1: No-terminal 'declvar' =====
// LL1[declvar, ID] = declvar → ID ':' tipo
static void declvar(Parser* parser) {
    // LL1[declvar, ID] = ID ':' tipo
    consume(parser, TOKEN_ID, "Se esperaba nombre de variable");
    consume(parser, TOKEN_COLON, "Se esperaba ':' en declaracion");
    tipo(parser);
}

// ===== TABLA LL1: No-terminal 'comando' =====
// LL1[comando, if] = comando → cmdif
// LL1[comando, while] = comando → cmdwhile
// LL1[comando, return] = comando → cmdreturn
static void comando(Parser* parser) {
    if (check(parser, TOKEN_IF)) {
        // LL1[comando, if] = cmdif
        cmdif(parser);
    } else if (check(parser, TOKEN_WHILE)) {
        // LL1[comando, while] = cmdwhile
        cmdwhile(parser);
    } else if (check(parser, TOKEN_RETURN)) {
        // LL1[comando, return] = cmdreturn
        cmdreturn_stmt(parser);
    } else {
        error_at_current(parser, "Se esperaba un comando");
        advance(parser);
    }
}

// ===== TABLA LL1: No-terminal 'cmdif' =====
// LL1[cmdif, if] = cmdif → 'if' expression nl bloque { 'else' 'if' expression nl bloque } ['else' nl bloque] 'end'
static void cmdif(Parser* parser) {
    // LL1[cmdif, if] = 'if' expression nl bloque ...
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

// ===== TABLA LL1: No-terminal 'cmdwhile' =====
// LL1[cmdwhile, while] = cmdwhile → 'while' expression nl bloque 'loop'
static void cmdwhile(Parser* parser) {
    // LL1[cmdwhile, while] = 'while' expression nl bloque 'loop'
    consume(parser, TOKEN_WHILE, "Se esperaba 'while'");
    expression(parser);
    nl(parser);
    bloque(parser);
    consume(parser, TOKEN_LOOP, "Se esperaba 'loop' al final del while");
}

// ===== TABLA LL1: No-terminal 'cmdreturn' =====
// LL1[cmdreturn, return] = cmdreturn → 'return' [expression]
static void cmdreturn_stmt(Parser* parser) {
    // LL1[cmdreturn, return] = 'return' [expression]
    consume(parser, TOKEN_RETURN, "Se esperaba 'return'");
    
    if (!check(parser, TOKEN_NL) && !check(parser, TOKEN_EOF)) {
        expression(parser);
    }
}

// ===== TABLA LL1: No-terminal 'listaexp' =====
// LL1[listaexp, )] = listaexp → ε
// LL1[listaexp, FIRST(expression)] = listaexp → expression { ',' expression }
static void listaexp(Parser* parser) {
    if (check(parser, TOKEN_RPAREN)) {
        // LL1[listaexp, )] = ε
        return;
    }
    
    // LL1[listaexp, FIRST(expression)] = expression { ',' expression }
    expression(parser);
    
    while (match(parser, TOKEN_COMMA)) {
        expression(parser);
    }
}

// ==================== EXPRESIONES CON PRECEDENCIA LL1 ====================

// ===== TABLA LL1: No-terminal 'expression' =====
// LL1[expression, FIRST(expr_or)] = expression → expr_or
static void expression(Parser* parser) {
    // LL1[expression, ID|LITNUM|LITSTR|true|false|(|new|not|-] = expr_or
    expr_or(parser);
}

// ===== TABLA LL1: No-terminal 'expr_or' =====
// LL1[expr_or, FIRST(expr_and)] = expr_or → expr_and { 'or' expr_and }
static void expr_or(Parser* parser) {
    // LL1[expr_or, FIRST(expr_and)] = expr_and { 'or' expr_and }
    expr_and(parser);
    
    while (match(parser, TOKEN_OR)) {
        expr_and(parser);
    }
}

// ===== TABLA LL1: No-terminal 'expr_and' =====
// LL1[expr_and, FIRST(expr_rel)] = expr_and → expr_rel { 'and' expr_rel }
static void expr_and(Parser* parser) {
    // LL1[expr_and, FIRST(expr_rel)] = expr_rel { 'and' expr_rel }
    expr_rel(parser);
    
    while (match(parser, TOKEN_AND)) {
        expr_rel(parser);
    }
}

// ===== TABLA LL1: No-terminal 'expr_rel' =====
// LL1[expr_rel, FIRST(expr_add)] = expr_rel → expr_add [op_rel expr_add]
static void expr_rel(Parser* parser) {
    // LL1[expr_rel, FIRST(expr_add)] = expr_add [op_rel expr_add]
    expr_add(parser);
    
    // Operadores relacionales (no asociativos)
    if (match(parser, TOKEN_GT) || 
        match(parser, TOKEN_LT) || 
        match(parser, TOKEN_GE) || 
        match(parser, TOKEN_LE) || 
        match(parser, TOKEN_EQ) || 
        match(parser, TOKEN_NE)) {
        expr_add(parser);
    }
}

// ===== TABLA LL1: No-terminal 'expr_add' =====
// LL1[expr_add, FIRST(expr_mul)] = expr_add → expr_mul { ('+' | '-') expr_mul }
static void expr_add(Parser* parser) {
    // LL1[expr_add, FIRST(expr_mul)] = expr_mul { ('+' | '-') expr_mul }
    expr_mul(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        expr_mul(parser);
    }
}

// ===== TABLA LL1: No-terminal 'expr_mul' =====
// LL1[expr_mul, FIRST(expr_unary)] = expr_mul → expr_unary { ('*' | '/') expr_unary }
static void expr_mul(Parser* parser) {
    // LL1[expr_mul, FIRST(expr_unary)] = expr_unary { ('*' | '/') expr_unary }
    expr_unary(parser);
    
    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH)) {
        expr_unary(parser);
    }
}

// ===== TABLA LL1: No-terminal 'expr_unary' =====
// LL1[expr_unary, not] = expr_unary → 'not' expr_unary
// LL1[expr_unary, -] = expr_unary → '-' expr_unary
// LL1[expr_unary, FIRST(expr_postfix)] = expr_unary → expr_postfix
static void expr_unary(Parser* parser) {
    if (match(parser, TOKEN_NOT) || match(parser, TOKEN_MINUS)) {
        // LL1[expr_unary, not|-] = ('not' | '-') expr_unary
        expr_unary(parser);
    } else {
        // LL1[expr_unary, FIRST(expr_postfix)] = expr_postfix
        expr_postfix(parser);
    }
}

// ===== TABLA LL1: No-terminal 'expr_postfix' =====
// LL1[expr_postfix, FIRST(expr_primary)] = expr_postfix → expr_primary { '[' expression ']' }
static void expr_postfix(Parser* parser) {
    // LL1[expr_postfix, FIRST(expr_primary)] = expr_primary { '[' expression ']' }
    expr_primary(parser);
    
    while (match(parser, TOKEN_LBRACKET)) {
        expression(parser);
        consume(parser, TOKEN_RBRACKET, "Se esperaba ']'");
    }
}

// ===== TABLA LL1: No-terminal 'expr_primary' =====
// LL1[expr_primary, LITNUM] = expr_primary → LITNUMERAL
// LL1[expr_primary, LITSTR] = expr_primary → LITSTRING
// LL1[expr_primary, true] = expr_primary → 'true'
// LL1[expr_primary, false] = expr_primary → 'false'
// LL1[expr_primary, new] = expr_primary → 'new' '[' expression ']' tipo
// LL1[expr_primary, (] = expr_primary → '(' expression ')'
// LL1[expr_primary, ID] = expr_primary → ID ['(' listaexp ')']
static void expr_primary(Parser* parser) {
    if (match(parser, TOKEN_LITNUMERAL)) {
        // LL1[expr_primary, LITNUM] = LITNUMERAL
        return;
    }
    
    if (match(parser, TOKEN_LITSTRING)) {
        // LL1[expr_primary, LITSTR] = LITSTRING
        return;
    }
    
    if (match(parser, TOKEN_TRUE) || match(parser, TOKEN_FALSE)) {
        // LL1[expr_primary, true|false] = TRUE | FALSE
        return;
    }
    
    if (match(parser, TOKEN_NEW)) {
        // LL1[expr_primary, new] = 'new' '[' expression ']' tipo
        consume(parser, TOKEN_LBRACKET, "Se esperaba '[' despues de 'new'");
        expression(parser);
        consume(parser, TOKEN_RBRACKET, "Se esperaba ']' en expresion new");
        tipo(parser);
        return;
    }
    
    if (match(parser, TOKEN_LPAREN)) {
        // LL1[expr_primary, (] = '(' expression ')'
        expression(parser);
        consume(parser, TOKEN_RPAREN, "Se esperaba ')'");
        return;
    }
    
    if (match(parser, TOKEN_ID)) {
        // LL1[expr_primary, ID] = ID ['(' listaexp ')']
        if (match(parser, TOKEN_LPAREN)) {
            listaexp(parser);
            consume(parser, TOKEN_RPAREN, "Se esperaba ')' en llamada a funcion");
        }
        return;
    }
    
    error_at_current(parser, "Se esperaba una expresion");
    advance(parser);
}