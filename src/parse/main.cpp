#include "parser.h"
#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
  TokBuf tokbuf(&cin);
  ParserDef parser;
  ParseParser(&tokbuf,parser);
  return 0;
}
