const int WS = 0;
const int LPAREN = 1;
const int RPAREN = 2;
const int PLUS = 3;
const int MINUS = 4;
const int TIMES = 5;
const int DIV = 6;
const int NUMBER = 7;
const int tokenCount = 8;
const int sectionCount = 1;
static const int tokenaction[] = {0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1};
static const char* tokenstr[] = {"WS","LPAREN","RPAREN","PLUS","MINUS","TIMES","DIV","NUMBER"};
static const bool isws[] = {true,false,false,false,false,false,false,false};
const int stateCount = 14;
static const int transitions[] = {1,1,0,0,2,3,'\t','\n','\r','\r',' ',' ',3,1,'(','(',4,1,')',')',5,1,'*','*',6,1,'+','+',7,1,'-','-',8,1,'.','.',9,1,'/','/',10,1,'0','9',2,3,'\t','\n','\r','\r',' ',' ',11,1,'0','9',10,1,'0','9',12,1,'.','.',13,1,'0','9',13,1,'0','9'};
static const int transitionOffset[] = {0,4,44,52,52,52,52,52,52,56,56,64,64,68,72};
static const int tokens[] = {-1,-1,0,1,2,5,3,4,-1,6,7,7,7,7};

