#include "parser.h"

namespace pptok {
#include "parsertok.h"
}

static void error(const String &err) {
  throw ParserError(err);
}

void ComputeFirsts(const ParserDef &parser, ParserSolution &solution) {
  for( Map<int,SymbolDef>::const_iterator cur = parser.m_tokdefs.begin(), end = parser.m_tokdefs.end(); cur != end; ++cur ) {
    if( cur->second.m_symboltype == SymbolTypeTerminal )
      solution.m_firsts[cur->second.m_tokid].insert(cur->second.m_tokid);
  }
  bool added = true;
  while( added ) {
    added = false;
    for( Map<int,SymbolDef>::const_iterator cur = parser.m_tokdefs.begin(), end = parser.m_tokdefs.end(); cur != end; ++cur ) {
      if( cur->second.m_symboltype == SymbolTypeTerminal )
        continue;
      int tokid = cur->second.m_tokid;
      int beforecnt = solution.m_firsts[tokid].size();
      for( Vector<Production*>::const_iterator curp = parser.m_productions.begin(), endp = parser.m_productions.end(); curp != endp; ++curp ) {
        const Production *p = *curp;
        int s = p->m_symbols[0];
        if( p->m_nt != tokid || p->m_nt == s )
          continue;
        const Set<int> &sfirsts = solution.m_firsts[s];
        solution.m_firsts[cur->second.m_tokid].insert(sfirsts.begin(),sfirsts.end());
      }
      int aftercnt = solution.m_firsts[tokid].size();
      if( beforecnt != aftercnt )
        added = true;
    }
  }
}

void ComputeFollows(const ParserDef &parser, ParserSolution &solution) {
  solution.m_follows[parser.getStartNt()].insert(EOF_TOK);
  bool added = true;
  while( added ) {
    added = false;
    for( Map<int,SymbolDef>::const_iterator cur = parser.m_tokdefs.begin(), end = parser.m_tokdefs.end(); cur != end; ++cur ) {
      int tokid = cur->second.m_tokid;
      int beforecnt = 0;
      if( solution.m_follows.find(tokid) != solution.m_follows.end() )
        beforecnt = solution.m_follows[tokid].size();
      for( Vector<Production*>::const_iterator curp = parser.m_productions.begin(), endp = parser.m_productions.end(); curp != endp; ++curp ) {
        const Production *p = *curp;
        for( Vector<int>::const_iterator curs = p->m_symbols.begin(), ends = p->m_symbols.end(); curs != ends; ++curs ) {
          if( *curs != tokid )
            continue;
          Vector<int>::const_iterator nxts = curs+1;
          int s = (nxts==ends) ? p->m_nt : *nxts;
          const Set<int> &sfirsts = solution.m_firsts[s];
          solution.m_follows[cur->second.m_tokid].insert(sfirsts.begin(),sfirsts.end());
        }
      }
      int aftercnt = 0;
      if( solution.m_follows.find(tokid) != solution.m_follows.end() )
        aftercnt = solution.m_follows[tokid].size();
      if( beforecnt != aftercnt )
        added = true;
    }
  }
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

void ComputeStates(const ParserDef &parser, ParserSolution &solution) {
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
      }
    }
  }
}

void ComputeActions(const ParserDef &parser, ParserSolution &solution) {
}

void SolveParser(const ParserDef &parser, ParserSolution &solution) {
  if( ! parser.getStartProduction() )
    error("The grammar definition requires a START production");
  ComputeFirsts(parser,solution);
  ComputeFollows(parser,solution);
  ComputeStates(parser,solution);
  ComputeActions(parser,solution);
}
