<start> : stmt ;

expr : ID ;

stmt : IF expr stmt
     | IF expr stmt ELSE stmt ;

RESTRICT [stmt: IF expr stmt] @ [stmt : IF expr .stmt ELSE stmt][stmt : IF expr stmt ELSE.stmt]* ;

