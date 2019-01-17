#include "parser.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  TokBuf tokbuf(stdin);
  ParserDef parser;
  ParseParser(&tokbuf,parser);
  return 0;
}
