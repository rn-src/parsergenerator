#ifndef __parsecommon_h
#define __parsecommon_h
#include "tinytemplates.h"

struct ParseError;
typedef struct ParseError ParseError;

void setParseError(int line, int col, const char *file, const String *err);
bool getParseError(const ParseError **err);
void clearParseError();

struct ParseError
{
  int line, col;
  String file;
  String err;
};

enum RxType {
  RxType_None = 0,
  RxType_Production = 1,
  RxType_CharSet = 2,
  RxType_BinaryOp = 3,
  RxType_Many = 4
};
typedef enum RxType RxType;

enum BinaryOp {
  BinaryOp_None = 0,
  BinaryOp_Or = 1,
  BinaryOp_Concat = 2
};
typedef enum BinaryOp BinaryOp;

struct ImportAs;
typedef struct ImportAs ImportAs;

struct ImportAs {
  const char *import;
  const char *as;
  ImportAs *next;
  ImportAs *itemHead;
  ImportAs *itemTail;
};

struct LanguageOutputOptions;
typedef struct LanguageOutputOptions LanguageOutputOptions;

enum OutputLanguage {
  OutputLanguage_C,
  OutputLanguage_Python
};
typedef enum OutputLanguage OutputLanguage;

struct LanguageOutputOptions {
  int min_nt_value;
  bool do_pound_line;
  OutputLanguage outputLanguage;
  const char *lexerName;
  ImportAs *importHead;
  ImportAs *importTail;
  const char *name;
  bool encode;
  bool compress;
  bool show_uncompressed_data;
};

void LanguageOutputOptions_import(LanguageOutputOptions *options, const char *name, const char *as);
void LanguageOutputOptions_importitem(LanguageOutputOptions *options, const char *name, const char *as);

struct LanguageOutputter;
typedef struct LanguageOutputter LanguageOutputter;

struct LanguageOutputter {
  void (*outTop)(const LanguageOutputter *This, FILE *out);
  void (*outBottom)(const LanguageOutputter *This, FILE *out);
  void (*outTypeDecl)(const LanguageOutputter* This, FILE* out, const char* type, const char* name);
  void (*outDecl)(const LanguageOutputter *This, FILE *out, const char *type, const char *name);
  void (*outIntDecl)(const LanguageOutputter *This, FILE *out, const char *name, int i);
  void (*outOptionalDecl)(const LanguageOutputter* This, FILE* out, const char* type, const char* name);
  void (*outArrayDecl)(const LanguageOutputter *This, FILE *out, const char *type, const char *name);
  void (*outStartArray)(const LanguageOutputter *This, FILE *out);
  void (*outEndArray)(const LanguageOutputter *This, FILE *out);
  void (*outEndStmt)(const LanguageOutputter *This, FILE *out);
  void (*outNull)(const LanguageOutputter *This, FILE *out);
  void (*outBool)(const LanguageOutputter *This, FILE *out, bool b);
  void (*outStr)(const LanguageOutputter *This, FILE *out, const char *str);
  void (*outChar)(const LanguageOutputter *This, FILE *out, int c);
  void (*outInt)(const LanguageOutputter *This, FILE *out, int i);
  void (*outFunctionStart)(const LanguageOutputter *This, FILE *out, const char *rettype, const char *name);
  void (*outStartParameters)(const LanguageOutputter *This, FILE *out);
  void (*outEndParameters)(const LanguageOutputter *This, FILE *out);
  void (*outStartFunctionCode)(const LanguageOutputter *This, FILE *out);
  void (*outEndFunctionCode)(const LanguageOutputter *This, FILE *out);
  void (*outStartLineComment)(const LanguageOutputter* This, FILE* out);
  LanguageOutputOptions *options;
};

void LanguageOutputter_init(LanguageOutputter *outputter, LanguageOutputOptions *options);

// Language agnostic functions
int encodeuint(const LanguageOutputter *lang, FILE *out, unsigned int i);

void LongestMatch(const int *p,
                int testoff,
                int testlen,
                int *pmatchoff,
                int *pmatchlen
                );

void WriteIndexedArray(const LanguageOutputter *lang,
    FILE *out,
    VectorAny /*<int>*/ *values,
    const char *values_type,
    const char *values_name,
    VectorAny /*<int>*/ *counts,
    const char *counts_type,
    const char *counts_name,
    const char *index_type,
    const char *index_name);

#endif //__parsecommon_h
