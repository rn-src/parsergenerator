#include "parsecommon.h"
#include <ctype.h>

static ParseError g_err;
bool g_errInit = false;
bool g_errSet = false;

void setParseError(int line, int col, const char *file, const String *err) {
  if( ! g_errInit ) {
    String_init(&g_err.err,false);
    g_errInit = true;
  }
  clearParseError();
  g_err.line = line;
  g_err.col = col;
  String_AssignString(&g_err.err,err);
  String_AssignChars(&g_err.file,file);
  g_errSet = true;
  Scope_LongJmp(1);
}

bool getParseError(const ParseError **err) {
  if( ! g_errSet )
    return false;
  *err = &g_err;
  return true;
}

void clearParseError() {
  if( g_errInit )
    String_clear(&g_err.err);
  g_errSet = false;
}

extern void *zalloc(size_t len);

void CLanguageOutputter_outTop(const LanguageOutputter* This, FILE* out) {
  fprintf(out, "#ifndef __%s_h\n", This->options->name);
  fprintf(out, "#define __%s_h\n", This->options->name);
  ImportAs *extraImports = This->options->importHead;
  while( extraImports ) {
    if( extraImports->import[0] == '<' || extraImports->import[0] == '"' )
      fprintf(out, "#include %s\n", extraImports->import);
    else
      fprintf(out, "#include \"%s.h\"\n", extraImports->import);
    extraImports = extraImports->next;
  }
}

void CLanguageOutputter_outBottom(const LanguageOutputter* This, FILE *out) {
  fprintf(out, "#endif // __%s_h\n", This->options->name);
}

void CLanguageOutputter_outTypeDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs("typedef ", out); fputs(type, out); fputc(' ', out); fputs(name, out);
}

void CLanguageOutputter_outDecl(const LanguageOutputter *This, FILE *out, const char *type, const char *name) {
  if( This->options->prefix )
    fprintf(out, "%s %s%s", type, This->options->prefix, name);
  else
    fprintf(out, "%s %s", type, name);
}

void CLanguageOutputter_outIntDecl(const LanguageOutputter *This, FILE *out, const char *name, int i) {
  fprintf(out, "#define %s (%d)\n", name, i);
}
void CLanguageOutputter_outOptionalDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs(type, out);
  fputc(' ', out);
  fputs(name, out);
}

void CLanguageOutputter_outArrayDecl(const LanguageOutputter *This, FILE *out, const char *type, const char *name) {
  if( This->options->prefix )
    fprintf(out, "%s %s%s[]", type, This->options->prefix, name);
  else
    fprintf(out, "%s %s[]", type, name);
}

void CLanguageOutputter_outStartArray(const LanguageOutputter *This, FILE *out) {
  fputc('{',out);
}

void CLanguageOutputter_outEndArray(const LanguageOutputter *This, FILE *out) {
  fputc('}',out);
}

void CLanguageOutputter_outEndStmt(const LanguageOutputter *This, FILE *out) {
  fputc(';',out);
}

void CLanguageOutputter_outNull(const LanguageOutputter *This, FILE *out) {
  fputc('0',out);
}

void CLanguageOutputter_outBool(const LanguageOutputter *This, FILE *out, bool b) {
  fputs((b ? "true" : "false"),out);
}

void CLanguageOutputter_outStr(const LanguageOutputter *This, FILE *out, const char *str)  {
  fputc('"',out); fputs(str,out); fputc('"',out);
}

void CLanguageOutputter_outChar(const LanguageOutputter *This, FILE *out, int c)  {
  if(c == '\r' ) {
    fputs("'\\r'",out);
  } else if(c == '\n' ) {
    fputs("'\\n'",out);
  } else if(c == '\v' ) {
    fputs("'\\v'",out);
  } else if(c == ' ' ) {
    fputs("' '",out);
  } else if(c == '\t' ) {
    fputs("'\\t'",out);
  } else if(c == '\\' || c == '\'' ) {
    fprintf(out,"'\\%c'",c);
  } else if( c <= 126 && isgraph(c) ) {
    fprintf(out,"'%c'",c);
  } else
    fprintf(out,"%d",c);
}

void CLanguageOutputter_outInt(const LanguageOutputter *This, FILE *out, int i)  {
  fprintf(out,"%d",i);
}

void CLanguageOutputter_outFunctionStart(const LanguageOutputter *This, FILE *out, const char *rettype, const char *name) {
    fprintf(out, "%s %s", rettype, name);
}

void CLanguageOutputter_outStartParameters(const LanguageOutputter *This, FILE *out) {
  fputs("(", out);
}

void CLanguageOutputter_outEndParameters(const LanguageOutputter *This, FILE *out) {
  fputs(")", out);
}

void CLanguageOutputter_outStartFunctionCode(const LanguageOutputter *This, FILE *out) {
  fputs("{", out);
}

void CLanguageOutputter_outEndFunctionCode(const LanguageOutputter *This, FILE *out) {
  fputs("}", out);
}

void CLanguageOutputter_outStartLineComment(const LanguageOutputter* This, FILE* out) {
  fputs("//", out);
}


static const char* pytype(const char* type) {
  if (strncmp(type, "static", 6) == 0)
    type += 6;
  while (*type == ' ')
    ++type;
  if( strncmp(type,"const",5) == 0)
    type += 5;
  while( *type == ' ')
    ++type;
  if( strncmp(type,"char*",5) == 0 )
    return "str";
  if( strcmp(type,"unsigned char") == 0 )
    return "int";
  if( strcmp(type,"double") == 0 )
    return "float";
  return type;
}

void PyLanguageOutputter_outTop(const LanguageOutputter* This, FILE* out) {
  ImportAs *import = This->options->importHead;
  while( import ) {
    if( import->itemHead ) {
      ImportAs *item = import->itemHead;
      fprintf(out, "from %s import ", import->import);
      bool first = true;
      while( item ) {
        if( first )
          first = false;
        else
          fputs(",", out);
        if( item->as )
          fprintf(out, "%s as %s", item->import, item->as);
        else
          fputs(item->import, out);
        item = item->next;
      }
      fputc('\n',out);
    } else {
      if( import->as )
        fprintf(out, "import %s as %s\n", import->import, import->as);
      else
        fprintf(out, "import %s\n", import->import);
    }
    import = import->next;
  }
}
void PyLanguageOutputter_outBottom(const LanguageOutputter* This, FILE* out) {}

void PyLanguageOutputter_outDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs(name,out);
  fputs(": ",out);
  fputs(pytype(type), out);
}

void PyLanguageOutputter_outIntDecl(const LanguageOutputter* This, FILE* out, const char* name, int i) {
  fprintf(out, "%s: int = %d\n", name, i);
}

void PyLanguageOutputter_outOptionalDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs(name, out);
  fputs(": Optional[", out);
  fputs(pytype(type), out);
  fputs("]", out);
}

void PyLanguageOutputter_outTypeDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs(name, out);
  fputs(" = ", out);
  fputs(pytype(type), out);
}

void PyLanguageOutputter_outArrayDecl(const LanguageOutputter* This, FILE* out, const char* type, const char* name) {
  fputs(name,out);
  fputs(": ", out);
  fputs("Sequence[",out);
  fputs(pytype(type), out);
  fputs("]",out);
}

void PyLanguageOutputter_outStartArray(const LanguageOutputter* This, FILE* out) {
  fputc('[', out);
}

void PyLanguageOutputter_outEndArray(const LanguageOutputter* This, FILE* out) {
  fputc(']', out);
}

void PyLanguageOutputter_outEndStmt(const LanguageOutputter* This, FILE* out) {
}

void PyLanguageOutputter_outNull(const LanguageOutputter* This, FILE* out) {
  fputs("None", out);
}

void PyLanguageOutputter_outBool(const LanguageOutputter* This, FILE* out, bool b) {
  fputs((b ? "True" : "False"), out);
}

void PyLanguageOutputter_outStr(const LanguageOutputter* This, FILE* out, const char* str) {
  fputc('\'', out); fputs(str, out); fputc('\'', out);
}

void PyLanguageOutputter_outChar(const LanguageOutputter* This, FILE* out, int c) {
  if (c == '\r') {
    fputs("ord('\\r')", out);
  }
  else if (c == '\n') {
    fputs("ord('\\n')", out);
  }
  else if (c == '\v') {
    fputs("ord('\\v')", out);
  }
  else if (c == ' ') {
    fputs("ord(' ')", out);
  }
  else if (c == '\t') {
    fputs("ord('\\t')", out);
  }
  else if (c == '\\' || c == '\'') {
    fprintf(out, "ord('\\%c')", c);
  }
  else if (c <= 126 && isgraph(c)) {
    fprintf(out, "ord('%c')", c);
  }
  else
    fprintf(out, "%d", c);
}

void PyLanguageOutputter_outInt(const LanguageOutputter* This, FILE* out, int i) {
  fprintf(out, "%d", i);
}

void PyLanguageOutputter_outFunctionStart(const LanguageOutputter* This, FILE* out, const char* rettype, const char* name) {
  fputs("def ", out);
  fputs(name, out);
}

void PyLanguageOutputter_outStartParameters(const LanguageOutputter* This, FILE* out) {
  fputs("(", out);
}

void PyLanguageOutputter_outEndParameters(const LanguageOutputter* This, FILE* out) {
  fputs(")", out);
}

void PyLanguageOutputter_outStartFunctionCode(const LanguageOutputter* This, FILE* out) {
  fputs(":", out);
}

void PyLanguageOutputter_outEndFunctionCode(const LanguageOutputter* This, FILE* out) {}

void PyLanguageOutputter_outStartLineComment(const LanguageOutputter* This, FILE* out) {
  fputs("#", out);
}

void LanguageOutputter_init(LanguageOutputter *outputter, LanguageOutputOptions *options) {
  outputter->options = options;
  if( options->outputLanguage == OutputLanguage_C ) {
    outputter->outTop = CLanguageOutputter_outTop;
    outputter->outBottom = CLanguageOutputter_outBottom;
    outputter->outTypeDecl = CLanguageOutputter_outTypeDecl;
    outputter->outDecl = CLanguageOutputter_outDecl;
    outputter->outIntDecl = CLanguageOutputter_outIntDecl;
    outputter->outTypeDecl = CLanguageOutputter_outTypeDecl;
    outputter->outArrayDecl = CLanguageOutputter_outArrayDecl;
    outputter->outStartArray = CLanguageOutputter_outStartArray;
    outputter->outEndArray = CLanguageOutputter_outEndArray;
    outputter->outEndStmt = CLanguageOutputter_outEndStmt;
    outputter->outNull = CLanguageOutputter_outNull;
    outputter->outBool = CLanguageOutputter_outBool;
    outputter->outStr = CLanguageOutputter_outStr;
    outputter->outChar = CLanguageOutputter_outChar;
    outputter->outInt = CLanguageOutputter_outInt;
    outputter->outFunctionStart = CLanguageOutputter_outFunctionStart;
    outputter->outStartParameters = CLanguageOutputter_outStartParameters;
    outputter->outEndParameters = CLanguageOutputter_outEndParameters;
    outputter->outStartFunctionCode = CLanguageOutputter_outStartFunctionCode;
    outputter->outEndFunctionCode = CLanguageOutputter_outEndFunctionCode;
    outputter->outStartLineComment = CLanguageOutputter_outStartLineComment;
  }
  else if( options->outputLanguage == OutputLanguage_Python ) {
    outputter->outTop = PyLanguageOutputter_outTop;
    outputter->outBottom = PyLanguageOutputter_outBottom;
    outputter->outTypeDecl = PyLanguageOutputter_outTypeDecl;
    outputter->outDecl = PyLanguageOutputter_outDecl;
    outputter->outIntDecl = PyLanguageOutputter_outIntDecl;
    outputter->outTypeDecl = PyLanguageOutputter_outTypeDecl;
    outputter->outArrayDecl = PyLanguageOutputter_outArrayDecl;
    outputter->outStartArray = PyLanguageOutputter_outStartArray;
    outputter->outEndArray = PyLanguageOutputter_outEndArray;
    outputter->outEndStmt = PyLanguageOutputter_outEndStmt;
    outputter->outNull = PyLanguageOutputter_outNull;
    outputter->outBool = PyLanguageOutputter_outBool;
    outputter->outStr = PyLanguageOutputter_outStr;
    outputter->outChar = PyLanguageOutputter_outChar;
    outputter->outInt = PyLanguageOutputter_outInt;
    outputter->outFunctionStart = PyLanguageOutputter_outFunctionStart;
    outputter->outStartParameters = PyLanguageOutputter_outStartParameters;
    outputter->outEndParameters = PyLanguageOutputter_outEndParameters;
    outputter->outStartFunctionCode = PyLanguageOutputter_outStartFunctionCode;
    outputter->outEndFunctionCode = PyLanguageOutputter_outEndFunctionCode;
    outputter->outStartLineComment = PyLanguageOutputter_outStartLineComment;
  }
}


static int modup(int lhs, int rhs) {
  return lhs/rhs+((lhs%rhs)?1:0);
}

static unsigned int lshift(unsigned int i, unsigned int b) {
  // workaround bit-shift quirks
  if( b==0 )
    return i;
  else if( b >= 32 )
    return 0;
  return i<<b;
}

static unsigned int rshift(unsigned int i, unsigned int b) {
  // workaround bit-shift quirks
  if( b==0 )
    return i;
  else if( b >= 32 )
    return 0;
  return i>>b;
}

// A simple encoding scheme for unsigned 4 byte integers.
// In practice, almost all integers where we use this end
// up being 1 byte encodings, but we have to handle up to
// 4 byte integers, so we save 75% typically.
// First bytes contains...
// 1 byte encoding 0xxxxxxx
// 2 byte encoding 10xxxxxx xxxxxxxx
// 3 byte encoding 110xxxxx xxxxxxxx xxxxxxxx
// 4 byte encoding 1110xxxx xxxxxxxx xxxxxxxx xxxxxxxx
// 5 byte encoding 111110xx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
// Note: first byte of 5 byte encoding will be 11111000 due to
// all 32 bits being consumed in the final 4 bytes.
int encodeuint(const LanguageOutputter *lang, FILE *out, unsigned int i) {
  if( i == 0 ) {
    lang->outInt(lang,out,0);
    return 1;
  }
  unsigned int nbits = 0;
  unsigned char c = 0;
  while( (lshift(0xffffffff,nbits)&i) != 0 ) ++nbits;
  unsigned int bytes = modup(nbits,7);
  unsigned int mask = 0;
  unsigned int first = 8-bytes;
  for( unsigned int b = bytes; b > 0; --b ) {
    if( b == bytes ) {
      mask = 0xff & ~lshift(0xff,first);
      c = 0xff & lshift(0xff,first+1);
    } else {
      mask = 0xff;
      fputc(',',out);
    }
    c |= mask & rshift(i,(b-1)*8);
    lang->outInt(lang,out,c);
  } 
  return bytes;
}

static void options_import(ImportAs **head, ImportAs **tail, const char *name, const char *as) {
  if( name ) {
    ImportAs *node = (ImportAs*)zalloc(sizeof(ImportAs));
    node->import = name;
    node->as = as;
    if( *tail ) {
      (*tail)->next = node;
      *tail = node;
    } else {
      *head = node;
      *tail = node;
    }
  } else if( as && *tail ) {
    (*tail)->as = as;
  }
}

void LanguageOutputOptions_import(LanguageOutputOptions *options, const char *name, const char *as) {
  options_import(&options->importHead, &options->importTail, name, as);
}

void LanguageOutputOptions_importitem(LanguageOutputOptions *options, const char *name, const char *as) {
  if( ! options->importTail )
    return;
  options_import(&options->importTail->itemHead, &options->importTail->itemTail, name, as);
}

void WriteIndexedArray(const LanguageOutputter *lang,
		FILE *out,
		bool encode,
		bool compress,
		VectorAny /*<int>*/ *values,
		const char *values_type,
		const char *values_name,
		VectorAny /*<int>*/ *counts,
		const char *counts_type,
		const char *counts_name) {
  VectorAny /*<int>*/ offsets;
  Scope_Push();
  VectorAny_init(&offsets,getIntElement(),true);
  lang->outArrayDecl(lang,out,values_name,values_type);
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  int offset = 0;
  int idx = 0;
  VectorAny_push_back(&offsets,&offset);
  for( int i = 0, n = VectorAny_size(counts); i < n; ++i ) {
    for( int j = 0, m = VectorAny_ArrayOpConstT(counts,i,int); j < m; ++j ) {
      if( idx > 0 )
        fputs(",",out);
      int value = VectorAny_ArrayOpConstT(values,idx,int);
      idx++;
      if( encode )
        offset += encodeuint(lang,out,value);
      else {
	++offset;
	lang->outInt(lang,out,value);
      }
    }
    VectorAny_push_back(&offsets,&offset);
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  lang->outArrayDecl(lang,out,counts_type,counts_name);
  fputs(" = ",out);
  lang->outStartArray(lang,out);
  for( int i = 0; i < VectorAny_size(&offsets); ++i ) {
    if( i > 0 )
      fputc(',',out);
    lang->outInt(lang,out,VectorAny_ArrayOpT(&offsets,i,int));
  }
  lang->outEndArray(lang,out);
  lang->outEndStmt(lang,out);
  fputc('\n',out);

  Scope_Pop();
}


