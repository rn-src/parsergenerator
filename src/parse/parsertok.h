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
const int NOTARROW = 18;
const int GT = 19;
const int DOT = 20;
const int STAR = 21;
const int VSEP = 22;
const int SRC = 23;
const int tokenCount = 24;
const int sectionCount = 2;
static const int tokenaction[] = {0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,1,1,1,1,0,-1,2,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1};
static const char* tokenstr[] = {"WS","START","ERROR","DISALLOW","PRECEDENCE","REJECTABLE","TYPEDEF","LEFTASSOC","RIGHTASSOC","NONASSOC","ID","COLON","SEMI","LBRACE","RBRACE","LBRKT","RBRKT","ARROW","NOTARROW","GT","DOT","STAR","VSEP","SRC"};
static const bool isws[] = {true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false};
const int stateCount = 140;
static const int transitions[] = {1,1,0,0,2,1,1,1,3,3,'\t','\n','\r','\r',' ',' ',4,1,'#','#',5,1,'*','*',6,1,'-','-',7,1,'.','.',8,1,':',':',9,1,';',';',10,1,'<','<',11,1,'>','>',12,9,'A','C','E','K','M','M','O','O','Q','Q','S','S','U','Z','_','_','a','z',13,1,'D','D',14,1,'L','L',15,1,'N','N',16,1,'P','P',17,1,'R','R',18,1,'T','T',19,1,'[','[',20,1,']',']',21,1,'{','{',22,1,'|','|',23,1,'}','}',24,5,0,'!','#','&','(','z','|','|','~',2147483647,25,1,'"','"',26,1,'\'','\'',27,1,'{','{',28,1,'}','}',3,3,'\t','\n','\r','\r',' ',' ',4,1,'#','#',29,2,0,'\t','\v',2147483647,30,1,'\n','\n',6,1,'-','-',31,1,'/','/',32,1,'>','>',33,1,'e','e',34,1,'s','s',35,4,'0','9','A','Z','_','_','a','z',35,5,'0','9','A','H','J','Z','_','_','a','z',36,1,'I','I',35,5,'0','9','A','D','F','Z','_','_','a','z',37,1,'E','E',35,5,'0','9','A','N','P','Z','_','_','a','z',38,1,'O','O',35,5,'0','9','A','Q','S','Z','_','_','a','z',39,1,'R','R',35,6,'0','9','A','D','F','H','J','Z','_','_','a','z',40,1,'E','E',41,1,'I','I',35,5,'0','9','A','X','Z','Z','_','_','a','z',42,1,'Y','Y',24,5,0,'!','#','&','(','z','|','|','~',2147483647,43,3,0,'!','#','[',']',2147483647,44,1,'"','"',45,1,'\\','\\',46,3,0,'&','(','[',']',2147483647,47,1,'\'','\'',48,1,'\\','\\',29,2,0,'\t','\v',2147483647,30,1,'\n','\n',3,3,'\t','\n','\r','\r',' ',' ',4,1,'#','#',49,1,'-','-',50,1,'r','r',51,1,'t','t',35,4,'0','9','A','Z','_','_','a','z',35,5,'0','9','A','R','T','Z','_','_','a','z',52,1,'S','S',35,5,'0','9','A','E','G','Z','_','_','a','z',53,1,'F','F',35,5,'0','9','A','M','O','Z','_','_','a','z',54,1,'N','N',35,5,'0','9','A','D','F','Z','_','_','a','z',55,1,'E','E',35,5,'0','9','A','I','K','Z','_','_','a','z',56,1,'J','J',35,5,'0','9','A','F','H','Z','_','_','a','z',57,1,'G','G',35,5,'0','9','A','O','Q','Z','_','_','a','z',58,1,'P','P',43,3,0,'!','#','[',']',2147483647,44,1,'"','"',45,1,'\\','\\',59,4,0,'!','#','[',']','w','y',2147483647,60,1,'"','"',61,1,'\\','\\',62,1,'x','x',46,3,0,'&','(','[',']',2147483647,47,1,'\'','\'',48,1,'\\','\\',63,4,0,'&','(','[',']','w','y',2147483647,64,1,'\'','\'',65,1,'\\','\\',66,1,'x','x',49,1,'-','-',67,1,'>','>',68,1,'r','r',69,1,'a','a',35,4,'0','9','B','Z','_','_','a','z',70,1,'A','A',35,5,'0','9','A','S','U','Z','_','_','a','z',71,1,'T','T',35,4,'0','9','A','Z','_','_','a','z',72,1,'-','-',35,5,'0','9','A','B','D','Z','_','_','a','z',73,1,'C','C',35,5,'0','9','A','D','F','Z','_','_','a','z',74,1,'E','E',35,5,'0','9','A','G','I','Z','_','_','a','z',75,1,'H','H',35,5,'0','9','A','D','F','Z','_','_','a','z',76,1,'E','E',43,3,0,'!','#','[',']',2147483647,44,1,'"','"',45,1,'\\','\\',43,3,0,'!','#','[',']',2147483647,44,1,'"','"',45,1,'\\','\\',59,4,0,'!','#','[',']','w','y',2147483647,60,1,'"','"',61,1,'\\','\\',62,1,'x','x',43,6,0,'!','#','/',':','@','G','[',']','`','g',2147483647,44,1,'"','"',45,1,'\\','\\',77,3,'0','9','A','F','a','f',46,3,0,'&','(','[',']',2147483647,47,1,'\'','\'',48,1,'\\','\\',46,3,0,'&','(','[',']',2147483647,47,1,'\'','\'',48,1,'\\','\\',63,4,0,'&','(','[',']','w','y',2147483647,64,1,'\'','\'',65,1,'\\','\\',66,1,'x','x',46,6,0,'&','(','/',':','@','G','[',']','`','g',2147483647,47,1,'\'','\'',48,1,'\\','\\',78,3,'0','9','A','F','a','f',79,1,'o','o',80,1,'r','r',35,5,'0','9','A','K','M','Z','_','_','a','z',81,1,'L','L',35,4,'0','9','A','Z','_','_','a','z',82,1,'-','-',83,1,'A','A',35,5,'0','9','A','D','F','Z','_','_','a','z',84,1,'E','E',35,5,'0','9','A','B','D','Z','_','_','a','z',85,1,'C','C',35,5,'0','9','A','S','U','Z','_','_','a','z',86,1,'T','T',35,5,'0','9','A','C','E','Z','_','_','a','z',87,1,'D','D',43,6,0,'!','#','/',':','@','G','[',']','`','g',2147483647,44,1,'"','"',45,1,'\\','\\',77,3,'0','9','A','F','a','f',46,6,0,'&','(','/',':','@','G','[',']','`','g',2147483647,47,1,'\'','\'',48,1,'\\','\\',78,3,'0','9','A','F','a','f',88,1,'r','r',89,1,'t','t',35,5,'0','9','A','K','M','Z','_','_','a','z',90,1,'L','L',91,1,'A','A',92,1,'S','S',35,5,'0','9','A','C','E','Z','_','_','a','z',93,1,'D','D',35,5,'0','9','A','S','U','Z','_','_','a','z',94,1,'T','T',35,4,'0','9','A','Z','_','_','a','z',95,1,'-','-',35,5,'0','9','A','D','F','Z','_','_','a','z',96,1,'E','E',97,1,'>','>',98,1,'>','>',35,5,'0','9','A','N','P','Z','_','_','a','z',99,1,'O','O',100,1,'S','S',101,1,'S','S',35,5,'0','9','A','D','F','Z','_','_','a','z',102,1,'E','E',35,4,'0','9','B','Z','_','_','a','z',103,1,'A','A',104,1,'A','A',35,5,'0','9','A','E','G','Z','_','_','a','z',105,1,'F','F',35,5,'0','9','A','V','X','Z','_','_','a','z',106,1,'W','W',107,1,'S','S',108,1,'O','O',35,5,'0','9','A','M','O','Z','_','_','a','z',109,1,'N','N',35,5,'0','9','A','A','C','Z','_','_','a','z',110,1,'B','B',111,1,'S','S',35,4,'0','9','A','Z','_','_','a','z',35,4,'0','9','A','Z','_','_','a','z',112,1,'O','O',113,1,'C','C',35,5,'0','9','A','B','D','Z','_','_','a','z',114,1,'C','C',35,5,'0','9','A','K','M','Z','_','_','a','z',115,1,'L','L',116,1,'S','S',117,1,'C','C',118,1,'I','I',35,5,'0','9','A','D','F','Z','_','_','a','z',119,1,'E','E',35,5,'0','9','A','D','F','Z','_','_','a','z',120,1,'E','E',121,1,'O','O',122,1,'I','I',123,1,'A','A',35,4,'0','9','A','Z','_','_','a','z',35,4,'0','9','A','Z','_','_','a','z',124,1,'C','C',125,1,'A','A',126,1,'T','T',127,1,'I','I',128,1,'T','T',129,1,'I','I',130,1,'A','A',131,1,'I','I',132,1,'V','V',133,1,'T','T',134,1,'V','V',135,1,'E','E',136,1,'I','I',137,1,'E','E',138,1,'V','V',139,1,'E','E'};
static const int transitionOffset[] = {0,8,112,140,152,162,162,174,174,174,174,182,182,192,208,224,240,256,278,294,294,294,294,294,294,306,322,338,338,338,348,360,364,364,368,372,382,398,414,430,446,462,478,494,510,510,532,548,548,570,578,582,586,600,616,630,646,662,678,694,710,726,748,778,794,810,832,862,862,866,870,886,900,904,920,936,952,968,998,1028,1032,1036,1052,1056,1060,1076,1092,1106,1122,1126,1130,1146,1150,1154,1170,1184,1188,1204,1204,1204,1220,1224,1228,1244,1260,1264,1274,1284,1288,1292,1308,1324,1328,1332,1336,1352,1368,1372,1376,1380,1390,1400,1404,1408,1412,1416,1420,1424,1428,1432,1436,1440,1444,1448,1452,1456,1456,1460,1460,1464,1464};
static const int tokens[] = {-1,-1,-1,0,-1,21,-1,20,11,12,-1,19,10,10,10,10,10,10,10,15,16,13,22,14,23,-1,-1,13,14,-1,0,-1,17,-1,-1,10,10,10,10,10,10,10,10,-1,23,-1,-1,23,-1,-1,-1,-1,10,10,10,10,10,10,10,-1,23,-1,-1,-1,23,-1,-1,18,-1,-1,10,10,-1,10,10,10,10,-1,-1,-1,-1,10,-1,-1,10,10,10,10,-1,-1,10,-1,-1,10,10,-1,10,2,1,10,-1,-1,10,10,-1,6,3,-1,-1,10,10,-1,-1,-1,10,10,-1,-1,-1,4,5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,9,-1,7,-1,8};

