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

typedef Set<ProductionState> state_t;

void closure(state_t &state, ParserDef &parser) {
  int prevsize = 0;
  Set<ProductionState> newparts;
  Vector<Production*> productions;
  while( state.size() > prevsize ) {
    prevsize = state.size();
    newparts.clear();
    for( Set<ProductionState>::const_iterator cur = state.begin(), end = state.end(); cur != end; ++cur ) {
      productions = parser.productionsAt(*cur);
      for( Vector<Production*>::const_iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
        ProductionState ps(*curp,0);
        if( parser.isPartialReject(*cur) )
          ps.m_items.insert(ps.m_items.end(),cur->m_items.begin(),cur->m_items.end());
        newparts.insert(ps);
      }
    }
    for( Set<ProductionState>::const_iterator cur = newparts.begin(), end = newparts.end(); cur != end; ++cur )
      state.insert(*cur);
  }
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
  statemap[state] = states.size();
  states.push_back(state);
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
  if( ! ComputeStates(parser) )
    return error("failed to compute states");
  return true;
}
