TYPEDEF double e ;

LEFT-ASSOCIATIVE [e : e TIMES e][e : e DIV  e] ;
LEFT-ASSOCIATIVE [e : e TIMES e][e : e DIV  e] ;
PRECEDENCE [e : * e] > [e : e TIMES e][e : e DIV  e] > [e : e MINUS e][e : e PLUS e] ;

<start> : e { fprintf(stdout, "%f",$1); }
        ;

e : LPAREN e RPAREN { $$ = $2; }
  | e TIMES e       { $$ = $1 * $3; }
  | e DIV e         { $$ = $1 / $3; }
  | e PLUS e        { $$ = $1 + $3; }
  | e MINUS e       { $$ = $1 - $3; }
  | PLUS e          { $$ = $2; }
  | MINUS e         { $$ = -$2; }
  | NUMBER          { $$ = atof($1); }
  ;

