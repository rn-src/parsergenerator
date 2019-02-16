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

void closure(state_t &state, const ParserDef &parser) {
  int prevsize = 0;
  Set<ProductionState> newparts;
  Vector<Production*> productions;
  while( state.size() > prevsize ) {
    prevsize = state.size();
    newparts.clear();
    for( Set<ProductionState>::const_iterator cur = state.begin(), end = state.end(); cur != end; ++cur ) {
      productions = parser.productionsAt(cur->m_p,cur->m_idx);
      for( Vector<Production*>::const_iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
        ProductionState ps(*curp,0);
        newparts.insert(ps);
      }
    }
    for( Set<ProductionState>::const_iterator cur = newparts.begin(), end = newparts.end(); cur != end; ++cur )
      state.insert(*cur);
  }
}

void nexts(const state_t &state, const ParserDef &parser, Set<int> &nextSymbols) {
  for( state_t::const_iterator curs = state.begin(), ends = state.end(); curs != ends; ++curs ) {
    int symbol = curs->symbol();
    if( symbol == -1 )
      continue;
    const SymbolDef &def = parser.m_tokdefs[symbol];
    if( def.m_symboltype == SymbolTypeTerminal )
      nextSymbols.insert(symbol);
    else if( def.m_symboltype == SymbolTypeNonterminal ) {
      //nextSymbols.insert(firsts(symbol));
    }
  }
}

void advance(state_t &state, int tsymbol, const ParserDef &parser, state_t &nextState) {
  nextState.clear();
  for( state_t::const_iterator curs = state.begin(), ends = state.end(); curs != ends; ++curs ) {
    if( curs->symbol() != tsymbol )
      continue;
    ProductionState ps(*curs);
    ps.m_idx++;
    nextState.insert(ps);
  }
}

bool ComputeStates(const ParserDef &parser, ParserSolution &solution) {
  Map<state_t,int> statemap;
  state_t state, nextState;
  Set<int> nextSymbols;
  state.insert(ProductionState(parser.getStartProduction(),0));
  closure(state,parser);
  statemap[state] = solution.m_states.size();
  solution.m_states.push_back(state);
  for( int i = 0; i < solution.m_states.size(); ++i ) {
    state = solution.m_states[i];
    int stateIdx = i;
    nexts(state,parser,nextSymbols);
    for( Set<int>::iterator cur = nextSymbols.begin(), end = nextSymbols.end(); cur != end; ++cur ) {
      int symbol = *cur;
      advance(state,symbol,parser,nextState);
      closure(nextState,parser);
      if( nextState.empty() )
        continue;
      if( statemap.find(nextState) == statemap.end() ) {
        int nextStateIdx = solution.m_states.size();
        statemap[nextState] = nextStateIdx;
        solution.m_states.push_back(nextState);
        solution.m_shifts[Pair<int,int>(stateIdx,nextStateIdx)].insert(symbol);
      }
    }
  }
  return true;
}

bool SolveParser(const ParserDef &parser, ParserSolution &solution) {
  if( ! parser.getStartProduction() )
    return error("The grammar definition requires a <start> production");
  if( ! ComputeStates(parser,solution) )
    return error("failed to compute states");
  return true;
}
