TYPEDEF double e ;

PRECEDENCE [e : * e] >
           [e : e TIMES e] left-assoc = [e : e DIV  e] left-assoc >
           [e : e MINUS e] left-assoc = [e : e PLUS e] left-assoc ;

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

