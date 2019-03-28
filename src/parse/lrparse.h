#ifndef __lrparse_h
#define __lrparse_h
#include "../tok/tinytemplates.h"
#include "../tok/tok.h"

#define ACTION_SHIFT (0)
#define ACTION_REDUCE (1)
#define ACTION_STOP   (2)

struct ParseInfo {
  const int nstates;
  const int *actions;
  const int *actionstart;
};

template<class T>
class Parser {
public:
  TokenInfo *m_tokinfo;
  ParseInfo *m_parseinfo;
  typedef bool (*reducefnct)(int productionidx, T *inputs, T &ntout);
  reducefnct reduce;

  Parser(TokenInfo *tokinfo, ParseInfo *parseinfo, reducefnct rfnct) : m_tokinfo(tokinfo), m_parseinfo(parseinfo), reduce(rfnct) {}

  bool findsymbol(int tok, const int *symbols, int nsymbols) {
    for( int i = 0; i < nsymbols; ++i )
      if( symbols[i] == tok )
        return true;
    return false;
  }

  bool parse(TokBuf *tokbuf) {
    Tokenizer toks(tokbuf,m_tokinfo);
    Vector<int> states;
    Vector<T> values;
    Vector< Pair<int,T> > inputqueue;
    states.push_back(0);
    values.push_back(T());
    int tok = -1;
    while( states.size() > 0 ) {
      int stateno = states.back();
      if( inputqueue.size() )
        tok = inputqueue.back().first;
      else
        tok = toks.peek();
      const int *firstaction = m_parseinfo->actions+m_parseinfo->actionstart[stateno];
      const int *lastaction = m_parseinfo->actions+m_parseinfo->actionstart[stateno+1];
      while( firstaction != lastaction ) {
        const int *nextaction;
        int action = firstaction[0];
        bool didaction = false;
        if( action == ACTION_SHIFT ) {
          int shiftto = firstaction[1];
          int nsymbols = firstaction[2];
          nextaction = firstaction+3+nsymbols;
          if( findsymbol(tok,firstaction+3,nsymbols) ) {
            if( inputqueue.size() ) {
              states.push_back(shiftto);
              values.push_back(inputqueue.back().second);
              inputqueue.pop_back();
            } else {
              states.push_back(shiftto);
              T t;
              t.str = toks.tokstr();
              values.push_back(t);
              toks.discard();
            }
            didaction = true;
          }
        } else if( action == ACTION_REDUCE || action == ACTION_STOP ) {
          int reduceby = firstaction[1];
          int reducecount = firstaction[2];
          int nsymbols = firstaction[3];
          nextaction = firstaction+4+nsymbols;
          if( findsymbol(tok,firstaction+4,nsymbols) ) {
            T output;
            T *inputs = values.begin()+(values.size()-reducecount);
            if( reduce(reduceby,inputs,output) ) {
              states.resize(states.size()-reducecount);
              values.resize(values.size()-reducecount);
              inputqueue.push_back( Pair<int,T>(reduceby,output) );
              didaction = true;
            }
          }
          if( action == ACTION_STOP && didaction )
            return true;
        }
        if( didaction )
          break;
        firstaction = nextaction;
      }
      if( firstaction == lastaction )
        return false;
    }
    return true;
  }
};

#endif /* __lrrparse_h */
