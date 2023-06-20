# Grammar File Contents - Python Version
TYPEDEFTOK float e;
TYPEDEFTOK list[float] <extra>;

LEFT-ASSOCIATIVE [e : e TIMES e][e : e DIV  e] ;
LEFT-ASSOCIATIVE [e : e MINUS e][e : e PLUS  e] ;
PRECEDENCE [e : PLUS e][e : MINUS e] > [e : e TIMES e][e : e DIV  e] > [e : e MINUS e][e : e PLUS e] ;

<start> : e { $extra.append($1); }
        ;

e : LPAREN e RPAREN { $$ = $2 }
  | PLUS e          { $$ = $2 }
  | MINUS e         { $$ = -$2 }
  | e TIMES e       { $$ = $1 * $3 }
  | e DIV e         { $$ = $1 / $3 }
  | e PLUS e        { $$ = $1 + $3 }
  | e MINUS e       { $$ = $1 - $3 }
  | NUMBER          { $$ = float($1.s) }
  ;

