const int tokenCount = 9;
const int WS = 0;
const int START = 1;
const int ID = 2;
const int COLON = 3;
const int SEMI = 4;
const int LBRACE = 5;
const int RBRACE = 6;
const int LBRKT = 7;
const int RBRKT = 8;
const char* tokenstr[] = {"WS","START","ID","COLON","SEMI","LBRACE","RBRACE","LBRKT","RBRKT"};
bool isws[] = {true,false,false,false,false,false,false,false,false};
int ranges[] = {'\t','\n','\r','\r',' ',' ','0','9',':',':',';',';','<','<','>','>','A','Z','[','[',']',']','a','a','b','q','r','r','s','s','t','t','u','z','{','{','}','}'};
const int stateCount = 17;
int transitions[] = {0,1,1,1,2,1,4,2,5,3,6,4,8,5,9,6,10,7,11,5,12,5,13,5,14,5,15,5,16,5,17,8,18,9,0,1,1,1,2,1,14,10,3,11,8,11,11,11,12,11,13,11,14,11,15,11,16,11,15,12,3,11,8,11,11,11,12,11,13,11,14,11,15,11,16,11,11,13,13,14,15,15,7,16};
int transitionOffset[] = {0,17,20,20,20,21,29,29,29,29,29,30,38,39,40,41,42,42};
int tokens[] = {-1,0,3,4,-1,2,7,8,5,6,-1,2,-1,-1,-1,-1,1};

