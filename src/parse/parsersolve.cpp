#include "parser.h"

namespace pptok {
#include "parsertok.h"
}

static void error(const String &err) {
  throw ParserError(err);
}

void ComputeFirsts(const ParserDef &parser, ParserSolution &solution, FILE *out, int verbosity) {
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

void ComputeFollows(const ParserDef &parser, ParserSolution &solution, FILE *out, int verbosity) {
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
    for( Set<ProductionState>::const_iterator curpart = newparts.begin(), endpart = newparts.end(); curpart != endpart; ++curpart )
      state.insert(*curpart);
  }
}

void nexts(const state_t &state, const ParserDef &parser, Set<int> &nextSymbols) {
  for( state_t::const_iterator curs = state.begin(), ends = state.end(); curs != ends; ++curs ) {
    int symbol = curs->symbol();
    if( symbol == -1 )
      continue;
    nextSymbols.insert(symbol);
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

void ComputeStatesAndActions(const ParserDef &parser, ParserSolution &solution, FILE *vout, int verbosity) {
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
      Map<state_t,int>::iterator stateiter = statemap.find(nextState);
      int nextStateIdx = -1;
      if( stateiter == statemap.end() ) {
        nextStateIdx = solution.m_states.size();
        statemap[nextState] = nextStateIdx;
        solution.m_states.push_back(nextState);
      } else {
        nextStateIdx = stateiter->second;
      }
      solution.m_shifts[stateIdx][nextStateIdx].insert(symbol);
    }
    for( state_t::const_iterator curps = state.begin(), endps = state.end(); curps != endps; ++curps ) {
      if( curps->m_idx < curps->m_p->m_symbols.size() )
        continue;
      const Set<int> &follows = solution.m_follows[curps->m_p->m_nt];
      solution.m_reductions[stateIdx][curps->m_p].insert(follows.begin(),follows.end());
    }
  }
}

#define SHIFT (1)
#define REDUCE (2)
#define REDUCE2 (4)

void PrintStatesAndActions(const ParserDef &parser, const ParserSolution &solution, const Map<int,String> &tokens, FILE *out) {
  fprintf(out, "%d states\n\n", solution.m_states.size());
  for( int i = 0, n = solution.m_states.size(); i != n; ++i ) {
    Set<int> symbols;
    Set<int> shiftsymbols;
    Map<int,Set<const Production*> > reductions;
    fprintf(out, "State %d:\n", i);
    const state_t &state = solution.m_states[i];
    state.print(out,tokens);
    shiftmap_t::const_iterator shiftiter = solution.m_shifts.find(i);
    if( shiftiter != solution.m_shifts.end() ) {
      for( shifttosymbols_t::const_iterator curshift = shiftiter->second.begin(), endshift = shiftiter->second.end(); curshift != endshift; ++curshift ) {
        fprintf(out, "shift to state %d on ", curshift->first);
        bool bFirst = true;
        for( Set<int>::const_iterator cursym = curshift->second.begin(), endsym = curshift->second.end(); cursym != endsym; ++cursym ) {
          if( bFirst )
            bFirst = false;
          else
            fputs(", ", out);
          fputs(tokens[*cursym].c_str(), out);
          shiftsymbols.insert(*cursym);
          symbols.insert(*cursym);
        }
        fputs("\n",out);
      }
    }
    reducemap_t::const_iterator reduceiter = solution.m_reductions.find(i);
    if( reduceiter != solution.m_reductions.end() ) {
      for( reducebysymbols_t::const_iterator curreduce = reduceiter->second.begin(), endreduce = reduceiter->second.end(); curreduce != endreduce; ++curreduce ) {
        fputs("reduce by production ", out);
        curreduce->first->print(out,tokens);
        fputs(" on ", out);
        bool bFirst = true;
        for( Set<int>::const_iterator cursym = curreduce->second.begin(), endsym = curreduce->second.end(); cursym != endsym; ++cursym ) {
          const char *symstr = (*cursym==-1) ? "$" : tokens[*cursym].c_str();
          if( bFirst )
            bFirst = false;
          else
            fputs(", ", out);
          fputs(symstr, out);
          reductions[*cursym].insert(curreduce->first);
          symbols.insert(*cursym);
        }
        fputs("\n",out);
      }
    }
    for( Set<int>::const_iterator cursym = symbols.begin(), endsym = symbols.end(); cursym != endsym; ++cursym ) {
      int sym = *cursym;
      const char *symstr = (sym==-1) ? "$" : tokens[sym].c_str();
      if( shiftsymbols.find(sym) != shiftsymbols.end() && reductions.find(sym) != reductions.end() )
        fprintf(out, "shift/reduce conflict on %s\n", symstr);
      if( reductions.find(sym) != reductions.end() && reductions.find(sym)->second.size() > 1 )
        fprintf(out, "reduce/reduce conflict on %s\n", symstr);
    }
    fputs("\n",out);
  }
  fputs("\n",out);
}

void StringToInt_2_IntToString(const Map<String,int> &src, Map<int,String> &tokens);

void SolveParser(const ParserDef &parser, ParserSolution &solution, FILE *vout, int verbosity) {
  FILE *out = stderr;
  if( ! parser.getStartProduction() )
    error("The grammar definition requires a START production");
  ComputeFirsts(parser,solution,out,verbosity);
  ComputeFollows(parser,solution,out,verbosity);
  ComputeStatesAndActions(parser,solution,out,verbosity);
  if( verbosity >= 1 ) {
    Map<int,String> tokens;
    StringToInt_2_IntToString(parser.m_tokens,tokens);
    PrintStatesAndActions(parser,solution,tokens,vout);
  }
}
