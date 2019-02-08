const int WS = 0;
const int START = 1;
const int ERROR = 2;
const int DISALLOW = 3;
const int PRECEDENCE = 4;
const int REJECTABLE = 5;
const int TYPEDEF = 6;
const int LEFTASSOC = 7;
const int RIGHTASSOC = 8;
const int NONASSOC = 9;
const int ID = 10;
const int COLON = 11;
const int SEMI = 12;
const int LBRACE = 13;
const int RBRACE = 14;
const int LBRKT = 15;
const int RBRKT = 16;
const int ARROW = 17;
const int GT = 18;
const int DOT = 19;
const int ELIPSIS = 20;
const int STAR = 21;
const int VSEP = 22;
const int SRC = 23;
const int tokenCount = 24;
const int sectionCount = 2;
static const int tokenaction[] = {0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,1,1,1,1,0,-1,2,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1};
static const char* tokenstr[] = {"WS","START","ERROR","DISALLOW","PRECEDENCE","REJECTABLE","TYPEDEF","LEFTASSOC","RIGHTASSOC","NONASSOC","ID","COLON","SEMI","LBRACE","RBRACE","LBRKT","RBRKT","ARROW","GT","DOT","ELIPSIS","STAR","VSEP","SRC"};
static const bool isws[] = {true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false};
const int stateCount = 139;
static const int transitions[] = {1,1,0,0,2,1,1,1,3,3,'\t','\n','\r','\r',' ',' ',4,1,'#','#',5,1,'*','*',6,1,'-','-',7,1,'.','.',8,1,':',':',9,1,';',';',10,1,'<','<',11,1,'>','>',12,9,'A','C','E','K','M','M','O','O','Q','Q','S','S','U','Z','_','_','a','z',13,1,'D','D',14,1,'L','L',15,1,'N','N',16,1,'P','P',17,1,'R','R',18,1,'T','T',19,1,'[','[',20,1,']',']',21,1,'{','{',22,1,'|','|',23,1,'}','}',24,5,0,'!','#','&','(','z','|','|','~',2147483647,25,1,'"','"',26,1,'\'','\'',27,1,'{','{',28,1,'}','}',3,3,'\t','\n','\r','\r',' ',' ',4,1,'#','#',29,2,0,'\t','\v',2147483647,30,1,'\n','\n',6,1,'-','-',31,1,'>','>',32,1,'.','.',33,1,'e','e',34,1,'s','s',35,4,'0','9','A','Z','_','_','a','z',35,5,'0','9','A','H','J','Z','_','_','a','z',36,1,'I','I',35,5,'0','9','A','D','F','Z','_','_','a','z',37,1,'E','E',35,5,'0','9','A','N','P','Z','_','_','a','z',38,1,'O','O',35,5,'0','9','A','Q','S','Z','_','_','a','z',39,1,'R','R',35,6,'0','9','A','D','F','H','J','Z','_','_','a','z',40,1,'E','E',41,1,'I','I',35,5,'0','9','A','X','Z','Z','_','_','a','z',42,1,'Y','Y',24,5,0,'!','#','&','(','z','|','|','~',2147483647,43,3,0,'!','#','[',']',2147483647,44,1,'"','"',45,1,'\\','\\',46,3,0,'&','(','[',']',2147483647,47,1,'\'','\'',48,1,'\\','\\',29,2,0,'\t','\v',2147483647,30,1,'\n','\n',3,3,'\t','\n','\r','\r',' ',' ',4,1,'#','#',49,1,'.','.',50,1,'r','r',51,1,'t','t',35,4,'0','9','A','Z','_','_','a','z',35,5,'0','9','A','R','T','Z','_','_','a','z',52,1,'S','S',35,5,'0','9','A','E','G','Z','_','_','a','z',53,1,'F','F',35,5,'0','9','A','M','O','Z','_','_','a','z',54,1,'N','N',35,5,'0','9','A','D','F','Z','_','_','a','z',55,1,'E','E',35,5,'0','9','A','I','K','Z','_','_','a','z',56,1,'J','J',35,5,'0','9','A','F','H','Z','_','_','a','z',57,1,'G','G',35,5,'0','9','A','O','Q','Z','_','_','a','z',58,1,'P','P',43,3,0,'!','#','[',']',2147483647,44,1,'"','"',45,1,'\\','\\',59,4,0,'!','#','[',']','w','y',2147483647,60,1,'"','"',61,1,'\\','\\',62,1,'x','x',46,3,0,'&','(','[',']',2147483647,47,1,'\'','\'',48,1,'\\','\\',63,4,0,'&','(','[',']','w','y',2147483647,64,1,'\'','\'',65,1,'\\','\\',66,1,'x','x',67,1,'r','r',68,1,'a','a',35,4,'0','9','B','Z','_','_','a','z',69,1,'A','A',35,5,'0','9','A','S','U','Z','_','_','a','z',70,1,'T','T',35,4,'0','9','A','Z','_','_','a','z',71,1,'-','-',35,5,'0','9','A','B','D','Z','_','_','a','z',72,1,'C','C',35,5,'0','9','A','D','F','Z','_','_','a','z',73,1,'E','E',35,5,'0','9','A','G','I','Z','_','_','a','z',74,1,'H','H',35,5,'0','9','A','D','F','Z','_','_','a','z',75,1,'E','E',43,3,0,'!','#','[',']',2147483647,44,1,'"','"',45,1,'\\','\\',43,3,0,'!','#','[',']',2147483647,44,1,'"','"',45,1,'\\','\\',59,4,0,'!','#','[',']','w','y',2147483647,60,1,'"','"',61,1,'\\','\\',62,1,'x','x',43,6,0,'!','#','/',':','@','G','[',']','`','g',2147483647,44,1,'"','"',45,1,'\\','\\',76,3,'0','9','A','F','a','f',46,3,0,'&','(','[',']',2147483647,47,1,'\'','\'',48,1,'\\','\\',46,3,0,'&','(','[',']',2147483647,47,1,'\'','\'',48,1,'\\','\\',63,4,0,'&','(','[',']','w','y',2147483647,64,1,'\'','\'',65,1,'\\','\\',66,1,'x','x',46,6,0,'&','(','/',':','@','G','[',']','`','g',2147483647,47,1,'\'','\'',48,1,'\\','\\',77,3,'0','9','A','F','a','f',78,1,'o','o',79,1,'r','r',35,5,'0','9','A','K','M','Z','_','_','a','z',80,1,'L','L',35,4,'0','9','A','Z','_','_','a','z',81,1,'-','-',82,1,'A','A',35,5,'0','9','A','D','F','Z','_','_','a','z',83,1,'E','E',35,5,'0','9','A','B','D','Z','_','_','a','z',84,1,'C','C',35,5,'0','9','A','S','U','Z','_','_','a','z',85,1,'T','T',35,5,'0','9','A','C','E','Z','_','_','a','z',86,1,'D','D',43,6,0,'!','#','/',':','@','G','[',']','`','g',2147483647,44,1,'"','"',45,1,'\\','\\',76,3,'0','9','A','F','a','f',46,6,0,'&','(','/',':','@','G','[',']','`','g',2147483647,47,1,'\'','\'',48,1,'\\','\\',77,3,'0','9','A','F','a','f',87,1,'r','r',88,1,'t','t',35,5,'0','9','A','K','M','Z','_','_','a','z',89,1,'L','L',90,1,'A','A',91,1,'S','S',35,5,'0','9','A','C','E','Z','_','_','a','z',92,1,'D','D',35,5,'0','9','A','S','U','Z','_','_','a','z',93,1,'T','T',35,4,'0','9','A','Z','_','_','a','z',94,1,'-','-',35,5,'0','9','A','D','F','Z','_','_','a','z',95,1,'E','E',96,1,'>','>',97,1,'>','>',35,5,'0','9','A','N','P','Z','_','_','a','z',98,1,'O','O',99,1,'S','S',100,1,'S','S',35,5,'0','9','A','D','F','Z','_','_','a','z',101,1,'E','E',35,4,'0','9','B','Z','_','_','a','z',102,1,'A','A',103,1,'A','A',35,5,'0','9','A','E','G','Z','_','_','a','z',104,1,'F','F',35,5,'0','9','A','V','X','Z','_','_','a','z',105,1,'W','W',106,1,'S','S',107,1,'O','O',35,5,'0','9','A','M','O','Z','_','_','a','z',108,1,'N','N',35,5,'0','9','A','A','C','Z','_','_','a','z',109,1,'B','B',110,1,'S','S',35,4,'0','9','A','Z','_','_','a','z',35,4,'0','9','A','Z','_','_','a','z',111,1,'O','O',112,1,'C','C',35,5,'0','9','A','B','D','Z','_','_','a','z',113,1,'C','C',35,5,'0','9','A','K','M','Z','_','_','a','z',114,1,'L','L',115,1,'S','S',116,1,'C','C',117,1,'I','I',35,5,'0','9','A','D','F','Z','_','_','a','z',118,1,'E','E',35,5,'0','9','A','D','F','Z','_','_','a','z',119,1,'E','E',120,1,'O','O',121,1,'I','I',122,1,'A','A',35,4,'0','9','A','Z','_','_','a','z',35,4,'0','9','A','Z','_','_','a','z',123,1,'C','C',124,1,'A','A',125,1,'T','T',126,1,'I','I',127,1,'T','T',128,1,'I','I',129,1,'A','A',130,1,'I','I',131,1,'V','V',132,1,'T','T',133,1,'V','V',134,1,'E','E',135,1,'I','I',136,1,'E','E',137,1,'V','V',138,1,'E','E'};
static const int transitionOffset[] = {0,8,112,140,152,162,162,170,174,174,174,182,182,192,208,224,240,256,278,294,294,294,294,294,294,306,322,338,338,338,348,360,360,364,368,372,382,398,414,430,446,462,478,494,510,510,532,548,548,570,570,574,578,592,608,622,638,654,670,686,702,718,740,770,786,802,824,854,858,862,878,892,896,912,928,944,960,990,1020,1024,1028,1044,1048,1052,1068,1084,1098,1114,1118,1122,1138,1142,1146,1162,1176,1180,1196,1196,1196,1212,1216,1220,1236,1252,1256,1266,1276,1280,1284,1300,1316,1320,1324,1328,1344,1360,1364,1368,1372,1382,1392,1396,1400,1404,1408,1412,1416,1420,1424,1428,1432,1436,1440,1444,1448,1448,1452,1452,1456,1456};
static const int tokens[] = {-1,-1,-1,0,-1,21,-1,19,11,12,-1,18,10,10,10,10,10,10,10,15,16,13,22,14,23,-1,-1,13,14,-1,0,17,-1,-1,-1,10,10,10,10,10,10,10,10,-1,23,-1,-1,23,-1,20,-1,-1,10,10,10,10,10,10,10,-1,23,-1,-1,-1,23,-1,-1,-1,-1,10,10,-1,10,10,10,10,-1,-1,-1,-1,10,-1,-1,10,10,10,10,-1,-1,10,-1,-1,10,10,-1,10,2,1,10,-1,-1,10,10,-1,6,3,-1,-1,10,10,-1,-1,-1,10,10,-1,-1,-1,4,5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,9,-1,7,-1,8};

