<start> : stmt ;

stmt : IF stmt
     | IF stmt ELSE stmt
     | OTHER ;

RESTRICT [stmt : IF .stmt ELSE stmt][stmt : IF stmt ELSE.stmt]*[stmt: IF stmt] ;

