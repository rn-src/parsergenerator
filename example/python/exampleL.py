from typing import Sequence
WS: int = 0
LPAREN: int = 1
RPAREN: int = 2
PLUS: int = 3
MINUS: int = 4
TIMES: int = 5
DIV: int = 6
NUMBER: int = 7
tokenCount: int = 8
sectionCount: int = 1
sectioninfo: Sequence[int] = []
sectioninfo_offset: Sequence[int] = [0,0]
tokenstr: Sequence[str] = ['WS','LPAREN','RPAREN','PLUS','MINUS','TIMES','DIV','NUMBER']
isws: Sequence[int] = [0]
stateCount: int = 14
stateinfo: Sequence[int] = [0,1,1,0,0,0,2,3,9,1,3,0,19,0,0,3,1,40,0,0,4,1,41,0,0,5,1,42,0,0,6,1,43,0,0,7,1,45,0,0,8,1,46,0,0,9,1,47,0,0,10,1,48,9,1,2,3,9,1,3,0,19,0,0,11,1,48,9,8,10,1,48,9,8,12,1,46,0,8,13,1,48,9,8,13,1,48,9]
stateinfo_offset: Sequence[int] = [0,5,54,63,63,63,63,63,63,68,68,78,78,83,88]


