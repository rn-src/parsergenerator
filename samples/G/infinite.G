START : stmt ;

expr : ID ;

stmt : IF expr stmt
     | IF expr stmt ELSE stmt ;

DISALLOW [stmt : IF expr .stmt ELSE stmt]
  -->
              [stmt : IF expr stmt ELSE.stmt]
  *
  -/-> [stmt: IF expr stmt] ;

