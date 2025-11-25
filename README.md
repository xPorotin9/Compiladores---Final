
# Parser Mini-0

**Autor:** José Machaca  
**Curso:** Compiladores 

## Descripción

Este proyecto implementa un analizador sintáctico (parser) recursivo descendente para el lenguaje Mini-0. El parser analiza programas escritos en Mini-0 y determina si son sintácticamente correctos según la gramática del lenguaje.

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

El parser procesa este código y reporta si es sintácticamente válido o contiene errores.

## Estructura del Proyecto

```
mini0-parser/
├── src/
│   ├── tokens.h         # Definición de tipos de tokens
│   ├── lexer.h          # Interfaz del analizador léxico
│   ├── lexer.c          # Implementación del analizador léxico
│   ├── parser.h         # Interfaz del parser recursivo descendente
│   ├── parser.c         # Implementación del parser
│   └── main.c           # Programa principal
├── tests/
│   ├── valid/           # Programas Mini-0 válidos para prueba
│   └── invalid/         # Programas Mini-0 inválidos para prueba
└── README.md
```

## Descripción de Archivos

### Archivos Fuente

- **`main.c`**: Punto de entrada del programa. Lee archivos Mini-0 y coordina el análisis léxico y sintáctico.

- **`tokens.h`**: Define los tipos de tokens reconocidos (palabras reservadas, operadores, identificadores, literales) y la estructura Token.

- **`lexer.h/lexer.c`**: Analizador léxico que convierte el código fuente en una secuencia de tokens. Maneja comentarios, strings con escapes, números decimales y hexadecimales.

- **`parser.h/parser.c`**: Parser recursivo descendente que verifica si la secuencia de tokens cumple con la gramática Mini-0. Implementa precedencia de operadores y recuperación de errores.

### Archivos de Prueba

- **`tests/valid/`**: Contiene programas Mini-0 sintácticamente correctos que deben ser aceptados por el parser.

- **`tests/invalid/`**: Contiene programas Mini-0 con errores sintácticos que deben ser rechazados por el parser.

## Compilación y Ejecución

### Compilar el Proyecto

```bash
gcc -Wall -o mini0parser.exe src/main.c src/lexer.c src/parser.c
```

### Ejecutar con Archivo Individual

```bash
# Ejemplo con archivo válido
./mini0parser.exe tests/valid/01_hello.mini0

# Ejemplo con archivo inválido
./mini0parser.exe tests/invalid/01_sin_end.mini0
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

### Probar Archivos Válidos

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

### Probar Archivos Inválidos

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

- **Tipos básicos**: `int`, `bool`, `char`, `string`
- **Arreglos**: `[]int`, `[][]char`, etc.
- **Funciones**: Con parámetros y valor de retorno opcional
- **Variables**: Globales y locales
- **Estructuras de control**: `if/else/end`, `while/loop`
- **Expresiones**: Aritméticas, relacionales, lógicas con precedencia correcta
- **Literales**: Números (decimal/hex), strings, booleanos
- **Comentarios**: `//` y `/* */`

## Manejo de Errores

El parser detecta y reporta errores sintácticos con información de ubicación:

- Tokens no reconocidos
- Estructuras incompletas (funciones sin `end`, bucles sin `loop`)
- Expresiones mal formadas
- Tipos no válidos
- Paréntesis y corchetes no balanceados

## Transformaciones de Gramática

Para hacer la gramática compatible con análisis recursivo descendente se aplicaron:

- **Eliminación de recursión izquierda** en expresiones
- **Factorización común** para distinguir variables de llamadas
- **Precedencia explícita** de operadores mediante niveles jerárquicos
- **Manejo de ambigüedad** en declaraciones vs comandos

## Código de Salida

- **0**: Análisis exitoso
- **No 0**: Error léxico o sintáctico detectado
