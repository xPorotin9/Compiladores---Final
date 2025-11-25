# Parser Mini-0

Analizador sintáctico (parser) recursivo descendente para el lenguaje Mini-0.

## Autores
- José Carlos Machaca

## Descripción
Este proyecto implementa un parser recursivo descendente para el lenguaje Mini-0, 
desarrollado como Trabajo Práctico 2 de la materia Compiladores.

## Compilación

```bash
gcc -Wall -o mini0parser src/main.c src/lexer.c src/parser.c


./mini0parser <archivo.mini0>
```