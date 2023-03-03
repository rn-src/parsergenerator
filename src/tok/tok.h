#ifndef __tok_h
#define __tok_h

#include <stdio.h>
#include <string.h>
#include "tinytemplates.h"

struct ParsePos;
typedef struct ParsePos ParsePos;

struct ParsePos {
  String filename;
  int line, col;
};

struct TokBuf;
typedef struct TokBuf TokBuf;

typedef int (*TokBuf_peekc)(TokBuf *This, int n);
typedef void (*TokBuf_discard)(TokBuf *This, int n);
typedef int (*TokBuf_line)(const TokBuf *This);
typedef int (*TokBuf_col)(const TokBuf *This);
typedef void (*TokBuf_filename)(const TokBuf *This, String *sout);
typedef void (*TokBuf_getParsePos)(const TokBuf *This, VectorAny /*<ParsePos>*/ *fNames);


struct TokBuf {
  TokBuf_peekc peekc;
  TokBuf_discard discard;
  TokBuf_line line;
  TokBuf_col col;
  TokBuf_filename filename;
  TokBuf_getParsePos getParsePos;
};

struct StackTokBufItem;
typedef struct StackTokBufItem StackTokBufItem;

void StackTokBufItem_init(StackTokBufItem *This, bool onstack);
void StackTokBufItem_destroy(StackTokBufItem *This);
void StackTokBufItem_Assign(StackTokBufItem *lhs, const StackTokBufItem *rhs);

struct StackTokBufItem {
  TokBuf *tokbuf;
  int startline;
  int lineno;
  String filename;
};

struct Token;
typedef struct Token Token;

void Token_init(Token *This, bool onstack);
void Token_set(Token *This, String *s, String *filename, int wslines, int wscols, int wslen, int line, int col, int sym);
const char *Token_Chars(const Token *This);
void Token_Assign(Token *lhs, const Token *rhs);

struct Token {
  String m_s;
  String m_filename;
  int m_wslines, m_wscols, m_wslen;
  int m_line, m_col;
  int m_sym;
};

struct StackTokBuf;
typedef struct StackTokBuf StackTokBuf;

void StackTokBuf_init(StackTokBuf *This, bool onstack);
void StackTokBuf_destroy(StackTokBuf *This);
void StackTokBuf_assign(StackTokBuf *lhs, const StackTokBuf *rhs);
void StackTokBuf_push(StackTokBuf *This, TokBuf *tokbuf);
void StackTokBuf_setline(StackTokBuf *This, int lineno, String *filename);

struct StackTokBuf {
  TokBuf m_tokbuf;
  VectorAny /*<StackTokBufItem>*/ m_stack;
  Token m_endtok;
  void (*startBuf)(TokBuf *This, TokBuf *tokBuf);
  void (*endBuf)(TokBuf *This, TokBuf *tokBuf);
};

struct FileTokBuf;
typedef struct FileTokBuf FileTokBuf;

void FileTokBuf_init(FileTokBuf *This, FILE *in, const char *fname, bool onstack);
void FileTokBuf_destroy(FileTokBuf *This);

struct FileTokBuf {
  TokBuf m_tokbuf;
  char *m_buf;
  int m_buflen, m_buffill, m_bufpos, m_pos, m_line, m_col;
  FILE *m_in;
  String m_filename;
};

struct TokenInfo;
typedef struct TokenInfo TokenInfo;

struct TokenInfo {
  int m_tokenCount;
  int m_sectionCount;
  const int *m_tokenaction;
  const char **m_tokenstr;
  const bool *m_isws;
  int m_stateCount;
  const int *m_transitions;
  const int *m_transitionOffset;
  const int *m_tokens;
};

struct Tokenizer;
typedef struct Tokenizer Tokenizer;

typedef int (*Tokenizer_peek)(const Tokenizer *This);
typedef void (*Tokenizer_discard)(const Tokenizer *This);
typedef int (*Tokenizer_length)(const Tokenizer *This);
typedef void (*Tokenizer_tokstr)(Tokenizer *This, String *s);
typedef int (*Tokenizer_wslines)(const Tokenizer *This);
typedef int (*Tokenizer_wscols)(const Tokenizer *This);
typedef int (*Tokenizer_wslen)(const Tokenizer *This);
typedef int (*Tokenizer_line)(const Tokenizer *This);
typedef int (*Tokenizer_col)(const Tokenizer *This);
typedef void (*Tokenizer_filename)(const Tokenizer *This, String *filename);
typedef const char *(*Tokenizer_tokid2str)(int tok);
typedef void (*Tokenizer_getParsePos)(VectorAny /*<ParsePos>*/ *parsepos);

struct Tokenizer {
  Tokenizer_peek peek;
  Tokenizer_discard discard;
  Tokenizer_length length;
  Tokenizer_tokstr tokstr;
  Tokenizer_wslines wslines;
  Tokenizer_wscols wscols;
  Tokenizer_wslen wslen;
  Tokenizer_line line;
  Tokenizer_col col;
  Tokenizer_filename filename;
  Tokenizer_tokid2str tokid2str;
  Tokenizer_getParsePos getParsePos;
};

struct TokBufTokenizer;
typedef struct TokBufTokenizer TokBufTokenizer;

void TokBufTokenizer_init(TokBufTokenizer *This, TokBuf *tokbuf, TokenInfo *tokinfo, bool onstack);
void TokBufTokenizer_destroy(TokBufTokenizer *This);

struct TokBufTokenizer {
  Tokenizer m_tokenizer;
  TokBuf *m_tokbuf;
  TokenInfo *m_tokinfo;
  int m_tok;
  int m_tokLen;
  char *m_s;
  int m_slen;
  int m_wslines, m_wscols, m_wslen;
  VectorAny /*<int>*/ m_stack;
};

#endif /* __tok_h */
