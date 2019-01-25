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

#if 0
bool ComputeFirsts(ParserDef &parser) {
  Map<int,Set<int> > firsts;
  Map<int,Vector<Production*> > firstProductions;
  // A terminal is it's own first terminal.
  for( Map<int,SymbolDef>::iterator cur = parser.m_tokdefs.begin(), end = parser.m_tokdefs.end(); cur != end; ++cur ) {
    const SymbolDef &def = cur->second;
    if( def.m_symboltype == SymbolTypeTerminal )
      firsts[def.m_tokid].insert(def.m_tokid);
    else if( def.m_symboltype == SymbolTypeProduction )
      firstProductions[def.m_tokid] = parser.productionsAt(ProductionState(def.m_p,0));
  }
  // Nonterminals inheret the terminals from other symbols, repeat until closure.
  int added = 0;
  while( added ) {
    added = 0;
    for( Map<int,SymbolDef>::iterator cur = parser.m_tokdefs.begin(), end = parser.m_tokdefs.end(); cur != end; ++cur ) {
      const SymbolDef &def = cur->second;
      Vector<Production*> &productions = firstProductions[def.m_tokid];
      Set<int> &dstfirsts = firsts[def.m_tokid];
      for( Vector<Production*>::iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
        Production *p = *curp;
        const Set<int> &srcfirsts = firsts[p->m_symbolid];
        int before = dstfirsts.size();
        dstfirsts.insert(srcfirsts.begin(),srcfirsts.end());
        int after = dstfirsts.size();
        added += after-before;
      }
    }
  }
  return true;
}

bool ComputeFollows(ParserDef &parser) {
  // TODO
  return true;
}
#endif

typedef Set<ProductionState> state_t;

void closure(state_t &state, ParserDef &parser) {
  // TODO
}

void nexts(state_t &state, ParserDef &parser, Set<int> &nextSymbols) {
  // TODO
}

void advance(state_t &state, int symbol, ParserDef &parser, state_t &nextState) {
  // TODO
}

bool ComputeStates(ParserDef &parser) {
  Vector<state_t> states;
  Map<state_t,int> statemap;
  state_t state, nextState;
  Set<int> nextSymbols;
  state.insert(ProductionState(parser.getStartProduction(),0));
  closure(state,parser);
  states.push_back(state);
  statemap[state] = 0;
  for( int i = 0; i < states.size(); ++i ) {
    state = states[i];
    nexts(state,parser,nextSymbols);
    for( Set<int>::iterator cur = nextSymbols.begin(), end = nextSymbols.end(); cur != end; ++cur ) {
      advance(state,*cur,parser,nextState);
      closure(nextState,parser);
      if( statemap.find(nextState) == statemap.end() ) {
        statemap[nextState] = states.size();
        states.push_back(nextState);
      }
    }
  }
  return true;
}

bool SolveParser(ParserDef &parser) {
  if( ! parser.getStartProduction() )
    return error("The grammar definition requires a <start> production");
#if 0
  if( ! ComputeFirsts(parser) )
    return error("failed to compute firsts");
  if( ! ComputeFollows(parser) )
    return error("failed to compute follows");
#endif
  if( ! ComputeStates(parser) )
    return error("failed to compute states");
  return true;
}
