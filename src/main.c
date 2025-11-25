// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"

// Leer archivo completo a un string
static char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    
    if (file == NULL) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", path);
        return NULL;
    }
    
    // Obtener tamaño del archivo
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    
    // Reservar memoria
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error: No hay memoria suficiente para leer '%s'\n", path);
        fclose(file);
        return NULL;
    }
    
    // Leer archivo
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Error: No se pudo leer el archivo '%s'\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    buffer[bytes_read] = '\0';
    fclose(file);
    
    return buffer;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <archivo.mini0>\n", argv[0]);
        return 1;
    }
    
    // Leer archivo fuente
    char* source = read_file(argv[1]);
    if (source == NULL) {
        return 1;
    }
    
    // Inicializar lexer
    Lexer lexer;
    lexer_init(&lexer, source);
    
    // Inicializar parser
    Parser parser;
    parser_init(&parser, &lexer);
    
    // Parsear
    int success = parser_parse(&parser);
    
    // Liberar recursos
    parser_free(&parser);
    free(source);
    
    if (success) {
        printf("Analisis sintactico exitoso!\n");
        return 0;
    } else {
        return 1;
    }
}