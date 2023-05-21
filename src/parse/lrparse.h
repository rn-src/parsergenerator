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
  const int prod0;
  const int nproductions;
  const int *productions;
  const int *productionstart;
  const int start;
  const char **nonterminals;
};

template<class E, class T>
class Parser {
public:
  ParseInfo *m_parseinfo;
  E m_extra;
  typedef bool (*reducefnct)(E &extra, int productionidx, T *inputs, T &ntout, const char **err);
  reducefnct reduce;
  int m_verbosity;
  FILE *m_vpout;

  Parser(ParseInfo *parseinfo, reducefnct rfnct, int verbosity, FILE *vpout)
    : m_parseinfo(parseinfo), reduce(rfnct), m_verbosity(verbosity), m_vpout(vpout) {}

  bool findsymbol(int tok, const int *symbols, int nsymbols) {
    for( int i = 0; i < nsymbols; ++i )
      if( symbols[i] == tok )
        return true;
    return false;
  }

  bool parse(Tokenizer *ptoks) {
    Tokenizer &toks = *ptoks;
    Vector<int> states;
    Vector<T> values;
    Vector< Pair<int,T> > inputqueue;
    states.push_back(0);
    values.push_back(T());
    int tok = -1;
    int inputnum = 0;
    const char *err = 0;
    while( states.size() > 0 ) {
      if( m_verbosity ) {
        fputs("lrstates = { ", m_vpout);
        for( int i = 0; i < states.size(); ++i )
          fprintf(m_vpout, "%d ", states[i]);
        fputs("}\n", m_vpout);
      }
      int stateno = states.back();
      if( ! inputqueue.size() ) {
        inputnum += 1;
        int nxt = toks.peek();
        T t;
        t.tok.set(toks.tokstr(),toks.filename(),toks.wslines(),toks.wscols(),toks.wslen(),toks.line(),toks.col(),nxt);
        toks.discard();
        inputqueue.push_back(Pair<int,T>(nxt,t));
      }
      tok = inputqueue.back().first;
      if( m_verbosity ) {
        const T &t = inputqueue.back().second;
        if( t.tok.m_s.c_str() )
          fprintf(m_vpout, "input (# %d) = %d %s = \"%s\" - %s(%d:%d)\n", inputnum, tok, ptoks->tokid2str(tok), t.tok.m_s.c_str(), t.tok.m_filename.c_str(), t.tok.m_line, t.tok.m_col);
      }
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
            if( m_verbosity )
              fprintf(m_vpout, "shift to %d\n", shiftto);
            states.push_back(shiftto);
            values.push_back(inputqueue.back().second);
            inputqueue.pop_back();
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
            if( m_verbosity ) {
              int reducebyp = reduceby-m_parseinfo->prod0;
              fprintf(m_vpout, "reduce %d states by rule %d [", reducecount, reducebyp+1);
              const int *pproduction = m_parseinfo->productions+m_parseinfo->productionstart[reducebyp];
              int pcount = m_parseinfo->productionstart[reducebyp+1]-m_parseinfo->productionstart[reducebyp];
              bool first = true;
              while( pcount-- ) {
                int s = *pproduction++;
                fputs(" ", m_vpout);
                fputs(s >= m_parseinfo->start ? m_parseinfo->nonterminals[s-m_parseinfo->start] : ptoks->tokstr(s), m_vpout);
                if( first ) {
                  first = false;
                  fputs(" :", m_vpout);
                }
              }
              fputs(" ] ? ... ", m_vpout);
            }
            err = 0;
            if( reduce(m_extra,reduceby,inputs,output,&err) ) {
              if( m_verbosity )
                fputs("YES\n", m_vpout);
              states.resize(states.size()-reducecount);
              values.resize(values.size()-reducecount);
              inputqueue.push_back( Pair<int,T>(reduceby,output) );
              didaction = true;
            } else {
              if( m_verbosity )
                fputs("NO\n", m_vpout);
            }
          }
          if( action == ACTION_STOP && didaction ) {
            if( m_verbosity )
              fputs("ACCEPT\n", m_vpout);
            return true;
          }
        }
        if( didaction )
          break;
        firstaction = nextaction;
      }
      if( firstaction == lastaction ) {
        if( tok == PARSE_ERROR ) {
          // Input is error and no handler, keep popping states until we find a handler or run out.
          if( m_vpout ) {
            fprintf(m_vpout, "during error handling, popping lr state %d\n", states.back())
          }
          states.pop_back();
          if( states.size() == 0 ) {
            if( m_verbosity )
              fputs("NO ERROR HANDLER AVAILABLE, FAIL\n", m_vpout);
            return false;
          }
        }
        else {
          const T &t = inputqueue.back().second;
          // report the error for verbose output, but add error to the input, hoping for a handler, and carry on
          if( m_vpout ) {
            Vector<ParsePos> posstack;
            ptoks->getParsePos(posstack);
            if( err ) {
              fprintf(m_vpout, "%s(%d:%d) : error : %s\n", t.tok.m_filename.c_str(), t.tok.m_line, t.tok.m_col, err);
              for( Vector<ParsePos>::const_iterator pos = posstack.begin(), endPos = posstack.end(); pos != endPos; ++pos )
                fprintf(m_vpout, "  at %s(%d:%d)\n", pos->filename.c_str(), pos->line, pos->col);
            } else {
              fprintf(m_vpout, "%s(%d:%d) : parse error : input (# %d) = %d %s = \"%s\" - \n", t.tok.m_filename.c_str(), t.tok.m_line, t.tok.m_col, inputnum, tok, ptoks->tokid2str(tok), t.tok.m_s.c_str());
              for( Vector<ParsePos>::const_iterator pos = posstack.begin(), endPos = posstack.end(); pos != endPos; ++pos )
                fprintf(m_vpout, "  at %s(%d:%d)\n", pos->filename.c_str(), pos->line, pos->col);
            }
          }
          T t_err;
          t_err.tok.set("",t.tok.m_filename.c_str(),t.tok.m_wslines,t.tok.m_wscols,t.tok.m_wslen,t.tok.m_line,t.tok.m_col,PARSE_ERROR);
          inputqueue.push_back(Pair<int,T>(PARSE_ERROR,t_err));
        }
      }
    }
    return true;
  }
};

#endif /* __lrrparse_h */
