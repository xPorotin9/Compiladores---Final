# Analizador Sintáctico Mini-0

**Autor:** José Machaca  
**Curso:** Compiladores  
**Universidad:** La Salle  
**Fecha:** Noviembre 2025

## Descripción

Este proyecto implementa un **analizador sintáctico (parser) recursivo descendente LL1** para el lenguaje Mini-0. El parser analiza programas escritos en Mini-0, determina si son sintácticamente correctos según la gramática transformada del lenguaje, y reporta errores específicos con información de línea y columna.

### Características Principales

-  **Parser LL1 puro**: Cada decisión basada en 1 token de lookahead
-  **Gramática transformada**: Eliminación de recursión izquierda y factorización común  
-  **Precedencia de operadores**: Jerarquía correcta de operadores aritméticos y lógicos
-  **Recuperación de errores**: Continúa análisis después de detectar errores
-  **Casos de prueba completos**: 20 archivos (10 válidos + 10 inválidos)

### Ejemplo Simple
```mini0
fun main()
    x: int
    x = 42
    if x > 10
        x = x + 1
    end
end
```

### Ejemplo Complejo
```mini0
/* Programa que demuestra todas las características */

// Variables globales
contador: int

fun factorial(n: int): int
    result: int
    if n <= 1
        result = 1
    else
        result = n * factorial(n - 1)
    end
    return result
end

fun main()
    arr: []int
    i: int
    
    arr = new [10] int
    i = 0
    
    while i < 10
        arr[i] = factorial(i + 1)
        i = i + 1
    loop
    
    if arr[5] > 100
        contador = contador + 1
    end
end
```

El parser procesa ambos códigos y reporta si son sintácticamente válidos o contienen errores específicos.

## Estructura del Proyecto

```
mini0-parser/
├── src/
│   ├── tokens.h         # Definición de tipos de tokens y estructura Token
│   ├── lexer.h          # Interfaz del analizador léxico
│   ├── lexer.c          # Implementación del analizador léxico
│   ├── parser.h         # Interfaz del parser recursivo descendente LL1
│   ├── parser.c         # Implementación del parser con transformaciones LL1
│   └── main.c           # Programa principal y coordinación
├── tests/
│   ├── valid/           # 10 programas Mini-0 válidos para prueba
│   │   ├── 01_hello.mini0           # Función básica
│   │   ├── 02_tipos.mini0           # Todos los tipos
│   │   ├── 03_funcion_completa.mini0 # Parámetros y retorno
│   │   ├── 04_if_else.mini0         # Estructuras condicionales
│   │   ├── 05_while.mini0           # Bucles
│   │   ├── 06_expresiones.mini0     # Precedencia de operadores
│   │   ├── 07_arreglos.mini0        # Arrays y new
│   │   ├── 08_strings.mini0         # Strings con escapes
│   │   ├── 09_llamadas.mini0        # Llamadas a funciones
│   │   └── 10_completo.mini0        # Programa integrado completo
│   └── invalid/         # 10 programas Mini-0 inválidos para prueba
│       ├── 01_sin_end.mini0         # Función sin cerrar
│       ├── 02_sin_loop.mini0        # While sin loop
│       ├── 03_tipo_invalido.mini0   # Tipo no reconocido
│       ├── 04_parentesis.mini0      # Paréntesis no balanceados
│       ├── 05_operador_invalido.mini0 # Operador inexistente
│       ├── 06_string_sin_cerrar.mini0 # String malformado
│       ├── 07_asignacion_invalida.mini0 # Sintaxis incorrecta
│       ├── 08_parametro_sin_tipo.mini0 # Parámetro malformado
│       ├── 09_expresion_incompleta.mini0 # Expresión incompleta
│       └── 10_token_invalido.mini0  # Carácter no reconocido
├── docs/
│   └── informe_tecnico.pdf  # Informe técnico completo LaTeX
├── Makefile
└── README.md
```

## Descripción de Archivos

### Archivos Fuente

- **`main.c`**: Punto de entrada del programa. Lee archivos Mini-0 usando `argv[1]`, coordina el análisis léxico y sintáctico, y maneja códigos de salida (0=éxito, ≠0=error).

- **`tokens.h`**: Define los 40+ tipos de tokens reconocidos (palabras reservadas, operadores aritméticos, relacionales, signos de puntuación) y la estructura Token con información de ubicación.

- **`lexer.h/lexer.c`**: Analizador léxico que convierte código fuente en secuencia de tokens. Maneja:
  - Comentarios `//` y `/* */`
  - Strings con escapes `\n`, `\t`, `\"`, `\\`
  - Números decimales y hexadecimales (`0xFF`)
  - Identificadores y palabras reservadas
  - Manejo de errores léxicos

- **`parser.h/parser.c`**: **Parser recursivo descendente LL1** que verifica sintaxis. Implementa:
  - **Transformaciones de gramática**: Eliminación de recursión izquierda
  - **Factorización común**: Funciones `statement` y `statement_suffix` para resolver ambigüedad con `ID`
  - **Precedencia de operadores**: Jerarquía de funciones (`expr_or` → `expr_and` → `expr_rel` → `expr_add` → `expr_mul` → `expr_unary` → `expr_primary`)
  - **Recuperación de errores**: Función `synchronize` para continuar después de errores
  - **Correspondencia LL1**: Cada función implementa entradas específicas de tabla LL1

### Archivos de Prueba

- **`tests/valid/`**: Contiene programas Mini-0 sintácticamente correctos que ejercitan todas las reglas gramaticales principales.

- **`tests/invalid/`**: Contiene programas Mini-0 con errores sintácticos específicos para validar la detección de errores.

## Compilación y Ejecución

### Compilar el Proyecto

```bash
# Compilación básica
gcc -Wall -o mini0parser.exe src/main.c src/lexer.c src/parser.c

# Con Makefile (si está disponible)
make

# Compilación con debugging
gcc -Wall -g -o mini0parser.exe src/main.c src/lexer.c src/parser.c
```

### Ejecutar con Archivo Individual

```bash
# Ejemplo con archivo válido
./mini0parser.exe tests/valid/01_hello.mini0

# Ejemplo con archivo inválido  
./mini0parser.exe tests/invalid/01_sin_end.mini0

# Ejemplo con archivo propio
./mini0parser.exe mi_programa.mini0
```

### Salida Esperada

**Archivo válido:**
```
Analisis sintactico exitoso!
```

**Archivo inválido:**
```
[Linea 4, Columna 1] Error al final del archivo: Se esperaba 'end' al final de la funcion
```

## Ejecutar Todas las Pruebas

### Probar Archivos Válidos (PowerShell)

```powershell
Write-Host "=== VALIDOS (deben funcionar) ===" -ForegroundColor Green
Get-ChildItem tests/valid/*.mini0 | ForEach-Object { 
    Write-Host "`n$($_.Name):" -ForegroundColor Yellow
    $result = ./mini0parser.exe $_.FullName 2>&1
    if($LASTEXITCODE -eq 0) { 
        Write-Host "  CORRECTO: $result" -ForegroundColor Green 
    } else { 
        Write-Host "  ERROR: $result" -ForegroundColor Red 
    }
}
```

### Probar Archivos Inválidos (PowerShell)

```powershell
Write-Host "`n=== INVALIDOS (deben fallar) ===" -ForegroundColor Red
Get-ChildItem tests/invalid/*.mini0 | ForEach-Object { 
    Write-Host "`n$($_.Name):" -ForegroundColor Yellow
    $result = ./mini0parser.exe $_.FullName 2>&1
    if($LASTEXITCODE -ne 0) { 
        Write-Host "  CORRECTO - Error detectado: $result" -ForegroundColor Green 
    } else { 
        Write-Host "  ERROR - No detecto problema: $result" -ForegroundColor Red 
    }
}
```

## Características del Lenguaje Mini-0

El parser reconoce las siguientes construcciones del lenguaje Mini-0:

### Tipos de Datos
- **Tipos básicos**: `int`, `bool`, `char`, `string`
- **Arreglos**: `[]int`, `[][]char`, `[][][]bool` (multidimensionales)
- **Declaraciones**: `variable: tipo`

### Funciones
- **Definición**: `fun nombre(param1: tipo1, param2: tipo2): tipo_retorno`
- **Sin parámetros**: `fun main()`
- **Sin retorno**: `fun procedimiento(x: int)`
- **Con retorno**: `return expresion` o `return`

### Estructuras de Control
- **Condicionales**: `if condicion ... else if condicion ... else ... end`
- **Bucles**: `while condicion ... loop`

### Expresiones y Operadores
- **Aritméticas**: `+`, `-`, `*`, `/` con precedencia correcta
- **Relacionales**: `>`, `<`, `>=`, `<=`, `=`, `<>`
- **Lógicas**: `and`, `or`, `not` con evaluación por cortocircuito
- **Unarias**: `-expresion`, `not expresion`
- **Agrupación**: `(expresion)`
- **Acceso arrays**: `array[indice]`, `matriz[i][j]`

### Literales
- **Números enteros**: `42`, `-15`, `0xFF`, `0x1A2B`
- **Strings**: `"texto"`, `"con\nescapes\t\"comillas\""`
- **Booleanos**: `true`, `false`
- **Arrays dinámicos**: `new [tamaño] tipo`

### Comentarios
- **Línea**: `// comentario hasta fin de línea`
- **Bloque**: `/* comentario multilínea */`

### Variables
- **Globales**: Declaradas fuera de funciones
- **Locales**: Declaradas dentro de funciones

## Transformaciones de Gramática LL1

Para hacer la gramática original compatible con análisis recursivo descendente LL1 se aplicaron:

### 1. Eliminación de Recursión Izquierda

**ANTES (problemático):**
```
exp -> exp '+' exp | exp '*' exp | LITNUMERAL
```

**DESPUÉS (LL1):**
```
expression -> expr_or
expr_or -> expr_and { 'or' expr_and }
expr_and -> expr_rel { 'and' expr_rel }  
expr_rel -> expr_add [op_rel expr_add]
expr_add -> expr_mul { ('+' | '-') expr_mul }
expr_mul -> expr_unary { ('*' | '/') expr_unary }
expr_unary -> ('not' | '-') expr_unary | expr_postfix
expr_postfix -> expr_primary { '[' expression ']' }
```

### 2. Factorización Común

**Problema:** `ID` podía ser declaración, asignación o llamada función

**Solución LL1:**
```c
// Nueva función que factoriza el ID común
static void statement(Parser* parser) {
    consume(parser, TOKEN_ID, "Se esperaba identificador");
    statement_suffix(parser);  // Decide después de ver el siguiente token
}

// Función que resuelve la ambigüedad
static void statement_suffix(Parser* parser) {
    if (match(parser, TOKEN_COLON)) {        // ID ':' -> declaración
        tipo(parser);
    } else if (match(parser, TOKEN_EQ)) {    // ID '=' -> asignación
        expression(parser);
    } else if (match(parser, TOKEN_LPAREN)) { // ID '(' -> llamada
        listaexp(parser);
        consume(parser, TOKEN_RPAREN, "Se esperaba ')'");
    }
}
```

### 3. Precedencia Explícita

| Nivel | Operadores | Asociatividad | Precedencia |
|-------|------------|---------------|-------------|
| 1 | `or` | Izquierda | Menor |
| 2 | `and` | Izquierda | |
| 3 | `>`, `<`, `>=`, `<=`, `=`, `<>` | No asociativo | |
| 4 | `+`, `-` | Izquierda | |
| 5 | `*`, `/` | Izquierda | |
| 6 | `not`, `-` (unario) | Derecha | |
| 7 | `[` `]` (acceso array) | Izquierda | |
| 8 | `(` `)`, literales | --- | Mayor |

**Resultado:** `2 + 3 * 4` = `2 + (3 * 4)` = `14` ✓

## Manejo de Errores

### Tipos de Errores Detectados

#### Errores Léxicos
- **Caracteres no reconocidos**: `$`, `@`, `%`
- **Strings sin cerrar**: `"hola mundo`
- **Números malformados**: `0xG42`
- **Escapes inválidos**: `"\z"`

#### Errores Sintácticos
- **Estructuras incompletas**: Función sin `end`, bucle sin `loop`
- **Tokens no balanceados**: Paréntesis, corchetes sin cerrar
- **Tipos no válidos**: `float` (no existe en Mini-0)
- **Expresiones malformadas**: `x = 2 +` (operando faltante)
- **Sintaxis incorrecta**: `x == 5` (usa `==` en lugar de `=`)

### Recuperación de Errores

El parser **NO** se detiene abruptamente. Implementa recuperación mediante:

1. **Detección específica**: Reporta línea y columna exacta del error
2. **Modo pánico**: Evita cascada de errores falsos
3. **Puntos de sincronización**: Busca tokens conocidos (`fun`, `if`, `while`, `end`)
4. **Continuación**: Sigue analizando para detectar múltiples errores

**Ejemplo:**
```mini0
fun main()
    x: float     // Error 1: tipo no válido
    y: int
    y = 42 +     // Error 2: expresión incompleta
end
```

**Salida:**
```
[Linea 2, Columna 8] Error en 'float': Se esperaba un tipo (int, bool, char, string)
[Linea 4, Columna 12] Error al final del archivo: Se esperaba una expresion
```

## Tabla LL1 y Verificación

El parser implementa una **tabla LL1 completa** que garantiza:

-  **Determinismo**: Cada celda tiene máximo una producción
-  **Lookahead 1**: Decisiones basadas en 1 token únicamente  
-  **Sin conflictos**: No hay ambigüedad en las entradas

### Ejemplo de Correspondencia Código-Tabla

```c
// LL1[expr_primary, LITNUM] = expr_primary -> LITNUMERAL
// LL1[expr_primary, ID] = expr_primary -> ID ['(' listaexp ')']
// LL1[expr_primary, (] = expr_primary -> '(' expression ')'
static void expr_primary(Parser* parser) {
    if (match(parser, TOKEN_LITNUMERAL)) {    // Implementa entrada LL1
        return;
    }
    if (match(parser, TOKEN_ID)) {            // Implementa entrada LL1
        if (match(parser, TOKEN_LPAREN)) {
            listaexp(parser);
            consume(parser, TOKEN_RPAREN, "Se esperaba ')'");
        }
        return;
    }
    // ... más entradas LL1
}
```

## Requisitos Técnicos Cumplidos

| Requisito | Estado | Evidencia |
|-----------|--------|-----------|
|  Parser acepta/rechaza según gramática Mini-0 | Cumplido | 20 casos de prueba funcionando |
|  Gramática descendente (top-down) | Cumplido | Parser recursivo descendente |  
|  Primer argumento = nombre archivo | Cumplido | `main(argc, argv[1])` |
|  Errores → stderr + código ≠ 0 | Cumplido | `fprintf(stderr, ...)` + `return 1` |
|  Éxito → stdout + código = 0 | Cumplido | `printf(...)` + `return 0` |
|  Casos de prueba completos | Cumplido | 10 válidos + 10 inválidos |
|  Gramática compatible análisis descendente | Cumplido | Transformaciones LL1 aplicadas |
|  Manejo de errores especificado | Cumplido | Recuperación y sincronización |

## Código de Salida

- **0**: Análisis sintáctico exitoso
- **No 0**: Error léxico o sintáctico detectado
