<start> : E ;

E : E PLUS E
  | E TIMES E
  | ID
  ;

LEFT-ASSOCIATIVE [E: E PLUS E] ;
LEFT-ASSOCIATIVE [E: E TIMES E] ;
PRECEDENCE [E: E TIMES E] > [E: E PLUS E] ;

