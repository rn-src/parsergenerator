#include "tok.h"

ElementOps StackTokBufItemElement = {sizeof(StackTokBufItem), false, false, (elementInit)StackTokBufItem_init, (elementDestroy)StackTokBufItem_destroy, 0, 0, (elementAssign)StackTokBufItem_Assign, 0};

void StackTokBufItem_init(StackTokBufItem *This, bool onstack) {
  This->tokbuf = 0;
  This->startline = 0;
  This->lineno = 0;
  String_init(&This->filename,false);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)StackTokBufItem_destroy);
}

void StackTokBufItem_destroy(StackTokBufItem *This) {
  String_clear(&This->filename);
}

void StackTokBufItem_Assign(StackTokBufItem *lhs, const StackTokBufItem *rhs) {
  lhs->tokbuf = rhs->tokbuf;
  lhs->startline = rhs->startline;
  lhs->lineno = rhs->lineno;
  String_AssignString(&lhs->filename,&rhs->filename);
}

void Tok_destroy(Tok *This);

void Tok_init(Tok *This, bool onstack) {
  String_init(&This->m_s,false);
  String_init(&This->m_filename,false);
  This->m_wslines = 0;
  This->m_wscols = 0;
  This->m_wslen = 0;
  This->m_line = -1;
  This->m_col = -1;
  This->m_sym = -1;
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)Tok_destroy);
}

void Tok_destroy(Tok *This) {
  String_clear(&This->m_s);
  String_clear(&This->m_filename);
}

void Tok_set(Tok *This, String *s, String *filename, int wslines, int wscols, int wslen, int line, int col, int sym) {
    String_AssignString(&This->m_s,s);
    String_AssignString(&This->m_filename,filename);
    This->m_wslines = wslines;
    This->m_wscols = wscols;
    This->m_wslen = wslen;
    This->m_line = line;
    This->m_col = col;
    This->m_sym = sym;
}

const char *Tok_Chars(const Tok *This) {
  return String_Chars(&This->m_s);
}

void Tok_Assign(Tok *lhs, const Tok *rhs) {
  String_AssignString(&lhs->m_s,&rhs->m_s);
  String_AssignString(&lhs->m_filename,&rhs->m_filename);
  lhs->m_wslines = rhs->m_wslines;
  lhs->m_wscols = rhs->m_wscols;
  lhs->m_wslen = rhs->m_wslen;
  lhs->m_line = rhs->m_line;
  lhs->m_col = rhs->m_col;
  lhs->m_sym = rhs->m_sym;
}

static int StackTokBuf_peekc(StackTokBuf *This, int n);
static void StackTokBuf_discard(StackTokBuf *This, int n);
static int StackTokBuf_line(StackTokBuf *This);
static int StackTokBuf_col(StackTokBuf *This);
static void StackTokBuf_filename(StackTokBuf *This, String *sout);
static void StackTokBuf_getParsePos(StackTokBuf *This, VectorAny /*<ParsePos>*/ *fPos);

void StackTokBuf_init(StackTokBuf *This, bool onstack) {
  VectorAny_init(&This->m_stack, &StackTokBufItemElement, false);
  Tok_init(&This->m_endtok, false);
  This->m_tokbuf.peekc = (TokBuf_peekc)StackTokBuf_peekc;
  This->m_tokbuf.discard = (TokBuf_discard)StackTokBuf_discard;
  This->m_tokbuf.line = (TokBuf_line)StackTokBuf_line;
  This->m_tokbuf.col = (TokBuf_col)StackTokBuf_col;
  This->m_tokbuf.filename = (TokBuf_filename)StackTokBuf_filename;
  This->m_tokbuf.getParsePos = (TokBuf_getParsePos)StackTokBuf_getParsePos;
  if (onstack)
    Push_Destroy(This, (vpstack_destroyer)StackTokBuf_destroy);
}

void StackTokBuf_destroy(StackTokBuf *This) {
  VectorAny_destroy(&This->m_stack);
  Tok_destroy(&This->m_endtok);
}

void StackTokBuf_assign(StackTokBuf *lhs, const StackTokBuf *rhs) {
  VectorAny_Assign(&lhs->m_stack, &rhs->m_stack);
  Tok_Assign(&lhs->m_endtok, &rhs->m_endtok);
}

void StackTokBuf_push(StackTokBuf *This, TokBuf *tokbuf) {
  StackTokBufItem item;
  item.tokbuf = tokbuf;
  item.startline = -1;
  item.lineno = -1;
  VectorAny_push_back(&This->m_stack, &item);
  This->startBuf(&This->m_tokbuf, tokbuf);
  // if empty file, pop immediately
  if (tokbuf->peekc(tokbuf, 0) == -1)
    StackTokBuf_discard(This, 0);
}

static int StackTokBuf_peekc(StackTokBuf *This, int n) {
    if( VectorAny_size(&This->m_stack) ) {
      StackTokBufItem *item = &VectorAny_ArrayOpT(&This->m_stack,-1,StackTokBufItem);
      return item->tokbuf->peekc(item->tokbuf,n);
    }
    return -1;
}

static void StackTokBuf_discard(StackTokBuf *This, int n) {
  if( VectorAny_size(&This->m_stack) ) {
    StackTokBufItem *item = &VectorAny_ArrayOpT(&This->m_stack,-1,StackTokBufItem);
    TokBuf *tokbuf = item->tokbuf;
    tokbuf->discard(tokbuf,n);
    if( tokbuf->peekc(tokbuf,0) == -1 ) {
      VectorAny_pop_back(&This->m_stack);
      This->endBuf(&This->m_tokbuf,tokbuf);
      if( VectorAny_size(&This->m_stack) == 0 ) {
        tokbuf->filename(tokbuf,&This->m_endtok.m_filename);
        This->m_endtok.m_line = tokbuf->line(tokbuf);
        This->m_endtok.m_col = tokbuf->col(tokbuf);
      }
      //free(tokbuf);
    }
  }
}

static int StackTokBuf_line(StackTokBuf *This) {
  if( VectorAny_size(&This->m_stack) ) {
    StackTokBufItem *item = &VectorAny_ArrayOpT(&This->m_stack,-1,StackTokBufItem);
    TokBuf *tokbuf = item->tokbuf;
    if( item->lineno > 0 )
      return item->lineno + (tokbuf->line(tokbuf)-item->startline);
    return tokbuf->line(tokbuf);
  }
  return This->m_endtok.m_line;
}

static int StackTokBuf_col(StackTokBuf *This) {
  if( VectorAny_size(&This->m_stack) ) {
    StackTokBufItem *item = &VectorAny_ArrayOpT(&This->m_stack,-1,StackTokBufItem);
    TokBuf *tokbuf = item->tokbuf;
    return tokbuf->col(tokbuf);
  }
  return This->m_endtok.m_col;
}

static void StackTokBuf_filename(StackTokBuf *This, String *sout) {
  if( VectorAny_size(&This->m_stack) ) {
    StackTokBufItem *item = &VectorAny_ArrayOpT(&This->m_stack,-1,StackTokBufItem);
    TokBuf *tokbuf = item->tokbuf;
    if( String_length(&item->filename) ) {
      String_AssignString(sout,&item->filename);
      return;
    }
    tokbuf->filename(tokbuf,sout);
    return;
  }
  String_AssignString(sout,&This->m_endtok.m_filename);
}

void StackTokBuf_setline(StackTokBuf *This, int lineno, String *filename) {
  if (VectorAny_size(&This->m_stack)) {
    StackTokBufItem *item = &VectorAny_ArrayOpT(&This->m_stack, -1, StackTokBufItem);
    TokBuf *tokbuf = item->tokbuf;
    item->startline = tokbuf->line(tokbuf);
    item->lineno = lineno;
    String_AssignString(&item->filename, filename);
  }
}

static void StackTokBuf_getParsePos(StackTokBuf *This, VectorAny /*<ParsePos>*/ *fPos) {
  for( int i = VectorAny_size(&This->m_stack)-1; i >= 0; --i ) {
    StackTokBufItem *item = &VectorAny_ArrayOpT(&This->m_stack,i,StackTokBufItem);
    item->tokbuf->getParsePos(item->tokbuf,fPos);
  }
}

static int FileTokBuf_peekc(FileTokBuf *This, int n);
static void FileTokBuf_discard(FileTokBuf *This, int n);
static int FileTokBuf_line(const FileTokBuf *This);
static int FileTokBuf_col(const FileTokBuf *This);
static void FileTokBuf_filename(const FileTokBuf *This, String *sout);
static void FileTokBuf_getParsePos(FileTokBuf *This, VectorAny /*<ParsePos>*/ *fNames);

void FileTokBuf_init(FileTokBuf *This, FILE *in, const char *fname, bool onstack) {
  This->m_tokbuf.peekc = (TokBuf_peekc)FileTokBuf_peekc;
  This->m_tokbuf.discard = (TokBuf_discard)FileTokBuf_discard;
  This->m_tokbuf.line = (TokBuf_line)FileTokBuf_line;
  This->m_tokbuf.col = (TokBuf_col)FileTokBuf_col;
  This->m_tokbuf.filename = (TokBuf_filename)FileTokBuf_filename;
  This->m_tokbuf.getParsePos = (TokBuf_getParsePos)FileTokBuf_getParsePos;
  This->m_buf = 0;
  This->m_buffill = 0;
  This->m_buflen = 0;
  This->m_bufpos = 0;
  This->m_in = in;
  This->m_pos = 0;
  This->m_line = This->m_col = 1;
  String_init(&This->m_filename, false);
  String_AssignChars(&This->m_filename, fname);
  if (onstack)
    Push_Destroy(This, (vpstack_destroyer)FileTokBuf_destroy);
}

void FileTokBuf_destroy(FileTokBuf *This) {
  String_clear(&This->m_filename);
  if (This->m_buf)
    free(This->m_buf);
}

static bool FileTokBuf_addc(FileTokBuf *This) {
  if( feof(This->m_in) )
    return false;
  char c;
  if( fread(&c,1,1,This->m_in) != 1 )
    return false;
  if( This->m_buffill == This->m_buflen ) {
    int newlen = 0;
    if( This->m_buflen == 0 ) {
      newlen = 1024;
      This->m_buf = (char*)malloc(newlen);
      memset(This->m_buf,0,newlen);
    } else {
      newlen = This->m_buflen*2;
      This->m_buf = (char*)realloc(This->m_buf,newlen);
      memset(This->m_buf+This->m_buflen,0,newlen-This->m_buflen);
      for( int i = 0; i < This->m_buffill; ++i ) {
        int oldpos = (This->m_bufpos+i)%This->m_buflen;
        int newpos = (This->m_bufpos+i)%newlen;
        if( oldpos == newpos ) continue;
        This->m_buf[newpos] = This->m_buf[oldpos];
        This->m_buf[oldpos] = 0;
      }
    }
    This->m_buflen = newlen;
  }
  This->m_buf[(This->m_bufpos+This->m_buffill)%This->m_buflen] = c;
  ++This->m_buffill;
  return true;
}

static int FileTokBuf_peekc(FileTokBuf *This, int n) {
  while( n >= This->m_buffill ) {
    if( ! FileTokBuf_addc(This) )
      return -1;
  }
  return This->m_buf[(This->m_bufpos+n)%This->m_buflen];
}

static void FileTokBuf_discard(FileTokBuf *This, int n) {
  for( int i = 0; i < n; ++i ) {
    if( This->m_buffill == 0 )
      if( ! FileTokBuf_addc(This) )
        return;
    int c = This->m_buf[This->m_bufpos];
    This->m_bufpos = (This->m_bufpos+1)%This->m_buflen;
    --This->m_buffill;
    if( c == '\n' ) {
      ++This->m_line;
      This->m_col = 1;
    } else
      ++This->m_col;
  }
}

static int FileTokBuf_line(const FileTokBuf *This) {
  return This->m_line;
}

static int FileTokBuf_col(const FileTokBuf *This)  {
  return This->m_col;
}

static void FileTokBuf_filename(const FileTokBuf *This, String *sout) {
  String_AssignString(sout, &This->m_filename);
}

static void FileTokBuf_getParsePos(FileTokBuf *This, VectorAny /*<ParsePos>*/ *fNames) {
  ParsePos pos;
  String_AssignString(&pos.filename,&This->m_filename);
  pos.line = This->m_line;
  pos.col = This->m_col;
  VectorAny_push_back(fNames,&pos);
}

static int TokBufTokenizer_peek(TokBufTokenizer *This);
static void TokBufTokenizer_discard(TokBufTokenizer *This);
static int TokBufTokenizer_length(const TokBufTokenizer *This);
static void TokBufTokenizer_tokstr(TokBufTokenizer *This, String *sout);
static int TokBufTokenizer_wslines(const TokBufTokenizer *This);
static int TokBufTokenizer_wscols(const TokBufTokenizer *This);
static int TokBufTokenizer_wslen(const TokBufTokenizer *This);
static int TokBufTokenizer_line(const TokBufTokenizer *This);
static int TokBufTokenizer_col(const TokBufTokenizer *This);
static void TokBufTokenizer_filename(const TokBufTokenizer *This, String *filename);
static const char *TokBufTokenizer_tokid2str(const TokBufTokenizer *This, int tok);
static void TokBufTokenizer_getParsePos(const TokBufTokenizer *This, VectorAny /*<ParsePos>*/ *parsepos);

void TokBufTokenizer_init(TokBufTokenizer *This, TokBuf *tokbuf, TokenInfo *tokinfo, bool onstack) {
  This->m_tokenizer.peek = (Tokenizer_peek)TokBufTokenizer_peek;
  This->m_tokenizer.discard = (Tokenizer_discard)TokBufTokenizer_discard;
  This->m_tokenizer.length = (Tokenizer_length)TokBufTokenizer_length;
  This->m_tokenizer.tokstr = (Tokenizer_tokstr)TokBufTokenizer_tokstr;
  This->m_tokenizer.wslines = (Tokenizer_wslines)TokBufTokenizer_wslines;
  This->m_tokenizer.wscols = (Tokenizer_wscols)TokBufTokenizer_wscols;
  This->m_tokenizer.wslen = (Tokenizer_wslen)TokBufTokenizer_wslen;
  This->m_tokenizer.line = (Tokenizer_line)TokBufTokenizer_line;
  This->m_tokenizer.col = (Tokenizer_col)TokBufTokenizer_col;
  This->m_tokenizer.filename = (Tokenizer_filename)TokBufTokenizer_filename;
  This->m_tokenizer.tokid2str = (Tokenizer_tokid2str)TokBufTokenizer_tokid2str;
  This->m_tokenizer.getParsePos = (Tokenizer_getParsePos)TokBufTokenizer_getParsePos;
  This->m_tokbuf = tokbuf;
  This->m_tokinfo = tokinfo;
  This->m_tok = -1;
  This->m_tokLen = 0;
  This->m_s = 0;
  This->m_slen = 0;
  This->m_wslines = 0;
  This->m_wscols = 0;
  This->m_wslen = 0;
  VectorAny_init(&This->m_stack, getIntElement(), false);
  int zero = 0;
  VectorAny_push_back(&This->m_stack, &zero);
  if (onstack)
    Push_Destroy(This, (vpstack_destroyer)TokBufTokenizer_destroy);
}

void TokBufTokenizer_destroy(TokBufTokenizer *This) {
  VectorAny_destroy(&This->m_stack);
}

static int TokBufTokenizer_getTok(const TokBufTokenizer *This, int state) {
  return This->m_tokinfo->m_tokens[state];
}

static bool TokBufTokenizer_isWs(const TokBufTokenizer *This, int tok) {
  return This->m_tokinfo->m_isws[tok];
}

static int TokBufTokenizer_nextState(TokBufTokenizer *This, int state, int c) {
  const int *curTransition = This->m_tokinfo->m_transitions + This->m_tokinfo->m_transitionOffset[state];
  const int *endTransition = This->m_tokinfo->m_transitions + This->m_tokinfo->m_transitionOffset[state + 1];
  while (curTransition != endTransition) {
    int to = *curTransition++;
    int pairCount = *curTransition++;
    for (int i = 0; i < pairCount; ++i) {
      int low = *curTransition++;
      int high = *curTransition++;
      if (c >= low && c <= high)
        return to;
    }
  }
  return -1;
}

static void TokBufTokenizer_push(TokBufTokenizer *This, int tokset) {
  VectorAny_push_back(&This->m_stack,&tokset);
}

static void TokBufTokenizer_pop(TokBufTokenizer *This) {
  int len = VectorAny_size(&This->m_stack);
  if (len == 0)
    return;
  VectorAny_resize(&This->m_stack, len - 1);
}

static void TokBufTokenizer_gotots(TokBufTokenizer *This, int tokset) {
  VectorAny_ArrayOpT(&This->m_stack,-1,int) = tokset;
}

static int TokBufTokenizer_peek(TokBufTokenizer *This) {
  if (This->m_tok != -1)
    return This->m_tok;
  do
  {
    int curSection = VectorAny_ArrayOpConstT(&This->m_stack,-1,int);
    if (This->m_tok != -1) {
      if (TokBufTokenizer_isWs(This,This->m_tok)) {
        int tokstartline = This->m_tokbuf->line(This->m_tokbuf);
        int tokstartcol = This->m_tokbuf->col(This->m_tokbuf);
        This->m_tokbuf->discard(This->m_tokbuf,This->m_tokLen);
        int tokendline = This->m_tokbuf->line(This->m_tokbuf);
        int tokendcol = This->m_tokbuf->col(This->m_tokbuf);
        if (tokendline > tokstartline) {
          This->m_wslines += (tokendline - tokstartline);
          This->m_wscols = tokendcol - 1;
        }
        else {
          This->m_wscols += (tokendcol - tokstartcol);
        }
        This->m_wslen += This->m_tokLen;
      }
      else {
        This->m_tokbuf->discard(This->m_tokbuf,This->m_tokLen);
        This->m_wslines = 0;
        This->m_wscols = 0;
        This->m_wslen = 0;
      }
      This->m_tok = -1;
      This->m_tokLen = 0;
    }
    int state = TokBufTokenizer_nextState(This, 0, curSection);
    int tok = -1;
    int tokLen = -1;
    int len = 0;
    while (state != -1) {
      int t = TokBufTokenizer_getTok(This,state);
      if (t != -1) {
        tok = t;
        tokLen = len;
      }
      int c = This->m_tokbuf->peekc(This->m_tokbuf,len);
      state = TokBufTokenizer_nextState(This, state, c);
      ++len;
    }
    This->m_tok = tok;
    This->m_tokLen = tokLen;
    if (This->m_tok != -1) {
      const int *tokActions = This->m_tokinfo->m_tokenaction + (This->m_tok*This->m_tokinfo->m_sectionCount * 2) + curSection * 2;
      int tokenaction = tokActions[0];
      int actionarg = tokActions[1];
      if (tokenaction == 1)
        TokBufTokenizer_push(This,actionarg);
      else if (tokenaction == 2)
        TokBufTokenizer_pop(This);
      else if (tokenaction == 3)
        TokBufTokenizer_gotots(This,actionarg);
    }
  } while (This->m_tok != -1 && TokBufTokenizer_isWs(This,This->m_tok));
  return This->m_tok;
}

void TokBufTokenizer_discard(TokBufTokenizer *This) {
  if (This->m_tok == -1)
    TokBufTokenizer_peek(This);
  if (This->m_tok != -1) {
    This->m_tokbuf->discard(This->m_tokbuf, This->m_tokLen);
    This->m_tok = -1;
    This->m_tokLen = 0;
    This->m_wslines = 0;
    This->m_wscols = 0;
    This->m_wslen = 0;
  }
}

static int TokBufTokenizer_length(const TokBufTokenizer *This) {
  return This->m_tokLen;
}

static void TokBufTokenizer_tokstr(TokBufTokenizer *This, String *sout) {
  if (This->m_tok == -1) {
    String_AssignChars(sout, "");
    return;
  }
  if (This->m_slen < This->m_tokLen + 1) {
    This->m_s = (char*)realloc(This->m_s, This->m_tokLen + 1);
    This->m_slen = This->m_tokLen + 1;
  }
  *This->m_s = 0;
  if (This->m_tok != -1) {
    for (int i = 0, n = This->m_tokLen; i < n; ++i)
      This->m_s[i] = This->m_tokbuf->peekc(This->m_tokbuf,i);
    This->m_s[This->m_tokLen] = 0;
  }
  String_AssignChars(sout,This->m_s);
}

static int TokBufTokenizer_wslines(const TokBufTokenizer *This) {
  return This->m_wslines;
}

static int TokBufTokenizer_wscols(const TokBufTokenizer *This) {
  return This->m_wscols;
}

static int TokBufTokenizer_wslen(const TokBufTokenizer *This) {
  return This->m_wslen;
}

static int TokBufTokenizer_line(const TokBufTokenizer *This) {
  return This->m_tokbuf->line(This->m_tokbuf);
}

static int TokBufTokenizer_col(const TokBufTokenizer *This) {
  return This->m_tokbuf->col(This->m_tokbuf);
}

static void TokBufTokenizer_filename(const TokBufTokenizer *This, String *filename) {
  This->m_tokbuf->filename(This->m_tokbuf,filename);
}

static const char *TokBufTokenizer_tokid2str(const TokBufTokenizer *This, int tok) {
  if (tok < 0 || tok >= This->m_tokinfo->m_tokenCount)
    return  "?";
  return This->m_tokinfo->m_tokenstr[tok];
}

static void TokBufTokenizer_getParsePos(const TokBufTokenizer *This, VectorAny /*<ParsePos>*/ *parsepos) {
  This->m_tokbuf->getParsePos(This->m_tokbuf,parsepos);
}
