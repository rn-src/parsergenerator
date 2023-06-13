#!python3
import sys
import exampleL, exampleG
from lrparse import ParseInfo, Parser
from tok import  TokenInfo, TokBufTokenizer

parser = Parser(ParseInfo(exampleG),0)
tokinfo = TokenInfo(exampleL)
tokenizer = TokBufTokenizer(FileTokBuf(sys.stdin,'<stdin>'),tokinfo)
parser.parse(tokenizer)

