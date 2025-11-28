1.  programa → { NL } decl { decl }
2.  decl → funcion | global
3.  funcion → 'fun' ID '(' params ')' [':' tipo] nl bloque 'end' nl
4.  global → declvar nl
5.  bloque → statement nl bloque | comando nl bloque | ε
6.  statement → ID statement_suffix
7.  statement_suffix → ':' tipo | '=' expression | '[' expression ']' '=' expression | '(' listaexp ')'
8.  comando → cmdif | cmdwhile | cmdreturn
9.  cmdif → 'if' expression nl bloque {'else' 'if' expression nl bloque} ['else' nl bloque] 'end'
10. cmdwhile → 'while' expression nl bloque 'loop'
11. cmdreturn → 'return' [expression]
12. params → parametro {',' parametro} | ε
13. parametro → ID ':' tipo
14. tipo → '[' ']' tipo | tipobase
15. tipobase → 'int' | 'bool' | 'char' | 'string'
16. declvar → ID ':' tipo
17. listaexp → expression {',' expression} | ε
18. expression → expr_or
19. expr_or → expr_and {'or' expr_and}
20. expr_and → expr_rel {'and' expr_rel}
21. expr_rel → expr_add [op_rel expr_add]
22. expr_add → expr_mul {('+' | '-') expr_mul}
23. expr_mul → expr_unary {('*' | '/') expr_unary}
24. expr_unary → ('not' | '-') expr_unary | expr_postfix
25. expr_postfix → expr_primary {'[' expression ']'}
26. expr_primary → LITNUMERAL | LITSTRING | 'true' | 'false' | 'new' '[' expression ']' tipo | '(' expression ')' | ID ['(' listaexp ')']
27. nl → NL {NL} | ε (en contextos específicos)