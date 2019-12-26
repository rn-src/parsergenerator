<start> : stmt ;

stmt : IF stmt
     | IF stmt ELSE stmt
     | ID ;

RESTRICT [stmt: IF stmt] @ [stmt : IF .stmt ELSE stmt][stmt : IF stmt ELSE.stmt]* ;

