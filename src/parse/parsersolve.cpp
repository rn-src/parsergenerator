#include "parser.h"

namespace pptok {
#include "parsertok.h"
}

static bool error(const String &err) {
  fputs(err.c_str(),stderr);
  fputc('\n',stderr);
  fflush(stderr);
  return false;
}

bool SolveParser(ParserDef &parser) {
  if( ! parser.getStartProduction() )
    return error("The grammar definition requires a <start> production");
  return false;
}
