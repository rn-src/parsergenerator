#include "parser.h"

namespace pptok {
#include "parsertok.h"
}

static void error(const String &err) {
  throw ParserError(err);
}

void ComputeFirsts(const ParserDef &parser, ParserSolution &solution, FILE *out, int verbosity, Map< ProductionsAtKey,Vector<productionandforbidstate_t> > &productionsAtResults) {
  // No need to add terminals... we just test if the symbol is a terminal in the rest of the code
  /*
  for( Map<int,SymbolDef>::const_iterator cur = parser.m_tokdefs.begin(), end = parser.m_tokdefs.end(); cur != end; ++cur ) {
    if( cur->second.m_symboltype == SymbolTypeTerminal ) {
      int tokid = cur->second.m_tokid;
      for( int forbidstate = 0; forbidstate < parser.m_forbid.nstates(); ++forbidstate )
        solution.m_firsts[symbolandforbidstate_t(tokid,forbidstate)].insert(cur->second.m_tokid);
    }
  }
  */
  bool added = true;
  while( added ) {
    added = false;
    for( Map<int,SymbolDef>::const_iterator cur = parser.m_tokdefs.begin(), end = parser.m_tokdefs.end(); cur != end; ++cur ) {
      if( cur->second.m_symboltype != SymbolTypeNonterminalProduction )
        continue;
      int tokid = cur->second.m_tokid;
      for( int forbidstate = 0, nstates = parser.m_forbid.nstates(); forbidstate < nstates; ++forbidstate ) {
        int beforecnt = (solution.m_firsts.find(symbolandforbidstate_t(tokid,forbidstate)) != solution.m_firsts.end()) ? solution.m_firsts[symbolandforbidstate_t(tokid,forbidstate)].size() : 0;
        const Production *p = cur->second.m_p;
        int s = p->m_symbols[0];
        if( parser.getSymbolType(s) == SymbolTypeTerminal ) {          
          if( verbosity > 2 ) {
            if( ! solution.m_firsts[symbolandforbidstate_t(tokid,forbidstate)].contains(s) )
              fprintf(out,"Adding %s to FIRST(%s,%d)\n",parser.m_tokdefs[s].m_name.c_str(),parser.m_tokdefs[tokid].m_name.c_str(),forbidstate);
          }
          solution.m_firsts[symbolandforbidstate_t(tokid,forbidstate)].insert(s);
        } else {
          Vector< Pair<Production*,int> > productions = parser.productionsAt(p,0,forbidstate,productionsAtResults);
          for( Vector< Pair<Production*,int> >::const_iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
            int pid = curp->first->m_pid;
            int pfst = curp->second;
            if( solution.m_firsts.find(symbolandforbidstate_t(pid,pfst)) != solution.m_firsts.end() ) {
              Set<int> &dst = solution.m_firsts[symbolandforbidstate_t(tokid,forbidstate)];
              const Set<int> &sfirsts = solution.m_firsts[symbolandforbidstate_t(pid,pfst)];
              if( verbosity > 2 ) {
                Set<int> added = sfirsts;
                for( Set<int>::iterator curdst = dst.begin(), enddst = dst.end(); curdst != enddst; ++curdst )
                  added.erase(*curdst);
                if( added.size() > 0 ) {
                  for( Set<int>::iterator curadd = added.begin(), endadd = added.end(); curadd != endadd; ++curadd ) {
                    fprintf(out,"Adding %s to FIRST(%s,%d)\n",parser.m_tokdefs[*curadd].m_name.c_str(),parser.m_tokdefs[tokid].m_name.c_str(),forbidstate);
                  }
                }
              }
              dst.insert(sfirsts.begin(),sfirsts.end());
            }
          }
        }
        int aftercnt = (solution.m_firsts.find(symbolandforbidstate_t(tokid,forbidstate)) != solution.m_firsts.end()) ? solution.m_firsts[symbolandforbidstate_t(tokid,forbidstate)].size() : 0;
        if( beforecnt != aftercnt )
          added = true;
      }
    }
  }
}

void ComputeFollows(const ParserDef &parser, ParserSolution &solution, FILE *out, int verbosity, Map< ProductionsAtKey,Vector<productionandforbidstate_t> > &productionsAtResults) {
  solution.m_follows[symbolandforbidstate_t(parser.getStartProduction()->m_pid,0)].insert(EOF_TOK);
  bool added = true;
  while( added ) {
    added = false;
    // compute the follows for each terminal/nonterminal-production symbol ...
    for( Map<int,SymbolDef>::const_iterator cur = parser.m_tokdefs.begin(), end = parser.m_tokdefs.end(); cur != end; ++cur ) {
      SymbolType stype = cur->second.m_symboltype;
      if( stype == SymbolTypeNonterminal )
        continue; // non nonterminals
      int tokid = cur->second.m_tokid;
      // ... at each forbid state
      for( int forbidstate = 0, nstates = parser.m_forbid.nstates(); forbidstate < nstates; ++forbidstate ) {
        int beforecnt = (solution.m_follows.find(symbolandforbidstate_t(tokid,forbidstate)) != solution.m_follows.end() ) ? solution.m_follows[symbolandforbidstate_t(tokid,forbidstate)].size() : 0;
        // ... by looking at every position of every prodution and finding where the symbol occurs
        for( Vector<Production*>::const_iterator curp = parser.m_productions.begin(), endp = parser.m_productions.end(); curp != endp; ++curp ) {
          const Production *p = *curp;
          for( int pos = 0, endpos = p->m_symbols.size(); pos != endpos; ++pos ) {
            // for terminals, we know we've found such a position if the symbol ids are the same
            if( stype == SymbolTypeTerminal && p->m_symbols[pos] != tokid )
              continue;
            // otherwise for nonterminal-productions, we need to test if the production can appear at the location.
            if( stype == SymbolTypeNonterminalProduction && parser.m_forbid.nextState(p,pos,forbidstate,cur->second.m_p) == -1 )
              continue;
            // If the symbol was found at the end of a production, then FOLLOW(symbol) needs to include FOLLOW(production,forbidstate)
            if( pos+1 == endpos ) {
              Map< symbolandforbidstate_t, Set<int> >::iterator pfollows = solution.m_follows.find(symbolandforbidstate_t(p->m_pid,forbidstate));
              if( pfollows != solution.m_follows.end() ) {
                Set<int> &dst = solution.m_follows[symbolandforbidstate_t(tokid,forbidstate)];                
                const Set<int> &follows = solution.m_follows[symbolandforbidstate_t(p->m_pid,forbidstate)];
                if( verbosity > 2 ) {
                  Set<int> added = follows;
                  for( Set<int>::iterator curdst = dst.begin(), enddst = dst.end(); curdst != enddst; ++curdst )
                    added.erase(*curdst);
                  if( added.size() > 0 ) {
                    fprintf(out,"From FOLLOW(%s,%d) {\n",parser.m_tokdefs[p->m_pid].m_name.c_str(),forbidstate);
                    for( Set<int>::iterator curadd = added.begin(), endadd = added.end(); curadd != endadd; ++curadd ) {
                      fprintf(out,"Adding %s to FOLLOW(%s,%d)\n",parser.m_tokdefs[*curadd].m_name.c_str(),parser.m_tokdefs[tokid].m_name.c_str(),forbidstate);
                    }
                    fprintf(out,"} From FOLLOW(%s,%d)\n",parser.m_tokdefs[p->m_pid].m_name.c_str(),forbidstate);
                  }
                }
                dst.insert(follows.begin(),follows.end());
              }
              continue;
            }
            // Otherwise FOLLOW(symbol) needs to include FIRSTS for the symbol following the matching position
            int nxts = p->m_symbols[pos+1];
            SymbolType nxtstype = parser.getSymbolType(nxts);
            // When the next symbol is a terminal, FIRSTS for next symbol is just the next symbol.
            if( nxtstype == SymbolTypeTerminal ) {
              if( ! solution.m_follows[symbolandforbidstate_t(tokid,forbidstate)].contains(nxts) ) {
                if( verbosity > 2 ) {
                  fprintf(out,"Adding %s to FOLLOW(%s,%d)\n",parser.m_tokdefs[nxts].m_name.c_str(),parser.m_tokdefs[tokid].m_name.c_str(),forbidstate);
                }
                solution.m_follows[symbolandforbidstate_t(tokid,forbidstate)].insert(nxts);
              }
              continue;
            }
            // otherwise when the next symbol is a nonterminal, FIRST for the next symbol is the union of the FIRSTS for the productions allowed at the position
            Vector<productionandforbidstate_t> productions = parser.productionsAt(p,pos+1,forbidstate,productionsAtResults);
            for( Vector<productionandforbidstate_t>::iterator curpat = productions.begin(), endpat = productions.end(); curpat != endpat; ++curpat ) {
              Map< symbolandforbidstate_t, Set<int> >::iterator pfirsts = solution.m_firsts.find(symbolandforbidstate_t(curpat->first->m_pid,curpat->second));
              if( pfirsts != solution.m_firsts.end() ) {
                Set<int> &dst = solution.m_follows[symbolandforbidstate_t(tokid,forbidstate)];                
                const Set<int> &firsts = pfirsts->second;
                if( verbosity > 2 ) {
                  Set<int> added = firsts;
                  for( Set<int>::iterator curdst = dst.begin(), enddst = dst.end(); curdst != enddst; ++curdst )
                    added.erase(*curdst);
                  if( added.size() > 0 ) {
                    fprintf(out,"From FIRSTS(%s,%d) {\n",parser.m_tokdefs[curpat->first->m_pid].m_name.c_str(),curpat->second);
                    for( Set<int>::const_iterator curadd = added.begin(), endadd = added.end(); curadd != endadd; ++curadd ) {
                      fprintf(out,"Adding %s to FOLLOW(%s,%d)\n",parser.m_tokdefs[*curadd].m_name.c_str(),parser.m_tokdefs[tokid].m_name.c_str(),forbidstate);
                    }
                    fprintf(out,"} From FIRSTS(%s,%d)\n",parser.m_tokdefs[curpat->first->m_pid].m_name.c_str(),curpat->second);
                  }
                }
                dst.insert(firsts.begin(),firsts.end());
              }
            }
          }
        }
        int aftercnt = (solution.m_follows.find(symbolandforbidstate_t(tokid,forbidstate)) != solution.m_follows.end() ) ? solution.m_follows[symbolandforbidstate_t(tokid,forbidstate)].size() : 0;
        if( beforecnt != aftercnt )
          added = true;
      }
    }
  }
}

void closure(state_t &state, ParserDef &parser, Map< ProductionsAtKey,Vector<productionandforbidstate_t> > &productionsAtResults) {
  int prevsize = 0;
  Set<ProductionState> newparts;
  Vector<productionandforbidstate_t> productions;
  while( state.size() > prevsize ) {
    prevsize = state.size();
    newparts.clear();
    for( Set<ProductionState>::const_iterator cur = state.begin(), end = state.end(); cur != end; ++cur ) {
      productions = parser.productionsAt(cur->m_p,cur->m_pos,cur->m_forbidstate,productionsAtResults);
      for( Vector<productionandforbidstate_t>::const_iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
        ProductionState ps(curp->first,0,curp->second);
        newparts.insert(ps);
      }
    }
    for( Set<ProductionState>::const_iterator curpart = newparts.begin(), endpart = newparts.end(); curpart != endpart; ++curpart )
      state.insert(*curpart);
  }
}

void nexts(const state_t &state, ParserDef &parser, Set<int> &nextSymbols, Map< ProductionsAtKey,Vector<productionandforbidstate_t> > &productionsAtResults) {
  for( state_t::const_iterator curs = state.begin(), ends = state.end(); curs != ends; ++curs ) {
    int symbol = curs->symbol();
    if( symbol == -1 )
      continue;
    if( parser.getSymbolType(symbol) == SymbolTypeTerminal )
      nextSymbols.insert(symbol);
    else {
      Vector<productionandforbidstate_t> productions = parser.productionsAt(curs->m_p,curs->m_pos,curs->m_forbidstate,productionsAtResults);
      for( Vector<productionandforbidstate_t>::const_iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp )
        nextSymbols.insert(curp->first->m_pid);
    }
  }
}

void advance(state_t &state, int tsymbol, const ParserDef &parser, state_t &nextState) {
  nextState.clear();
  Production *p = 0;
  if( parser.getSymbolType(tsymbol) == SymbolTypeNonterminalProduction )
    p = parser.m_tokdefs[tsymbol].m_p;
  for( state_t::const_iterator curs = state.begin(), ends = state.end(); curs != ends; ++curs ) {
    int symbol = curs->symbol();
    if( symbol == tsymbol || (p && p->m_nt == symbol && ! parser.m_forbid.isForbidden(curs->m_p, curs->m_pos, curs->m_forbidstate, p)) )
      nextState.insert( ProductionState(curs->m_p, curs->m_pos+1, curs->m_forbidstate) );
  }
}

void ComputeStatesAndActions(ParserDef &parser, ParserSolution &solution, FILE *vout, int verbosity, Map< ProductionsAtKey,Vector<productionandforbidstate_t> > &productionsAtResults) {
  Map<state_t,int> statemap;
  state_t state, nextState;
  Set<int> nextSymbols;
  state.insert(ProductionState(parser.getStartProduction(),0,0));
  closure(state,parser,productionsAtResults);
  statemap[state] = solution.m_states.size();
  solution.m_states.push_back(state);
  for( int i = 0; i < solution.m_states.size(); ++i ) {
    if( verbosity > 2 )
      fprintf(vout,"computing state %d\n",i);
    state = solution.m_states[i];
    int stateIdx = i;
    nexts(state,parser,nextSymbols,productionsAtResults);
    for( Set<int>::iterator cur = nextSymbols.begin(), end = nextSymbols.end(); cur != end; ++cur ) {
      int symbol = *cur;
      advance(state,symbol,parser,nextState);
      closure(nextState,parser,productionsAtResults);
      if( nextState.empty() )
        continue;
      Map<state_t,int>::iterator stateiter = statemap.find(nextState);
      int nextStateIdx = -1;
      if( stateiter == statemap.end() ) {
        nextStateIdx = solution.m_states.size();
        statemap[nextState] = nextStateIdx;
        solution.m_states.push_back(nextState);
        if( verbosity > 2 )
          fprintf(vout,"added state %d\n",nextStateIdx);
      } else {
        nextStateIdx = stateiter->second;
      }
      solution.m_shifts[stateIdx][nextStateIdx].insert(symbol);
    }
    for( state_t::const_iterator curps = state.begin(), endps = state.end(); curps != endps; ++curps ) {
      if( curps->m_pos < curps->m_p->m_symbols.size() )
        continue;
      const Set<int> &follows = solution.m_follows[symbolandforbidstate_t(curps->m_p->m_pid,curps->m_forbidstate)];
      solution.m_reductions[stateIdx][curps->m_p].insert(follows.begin(),follows.end());
    }
  }
}

void PrintRules(const ParserDef &parser, FILE *out) {
  int i = 1;
  for( Vector<Production*>::const_iterator cur = parser.m_productions.begin(), end = parser.m_productions.end(); cur != end; ++cur ) {
    fprintf(out, "#%d ", i++);
    (*cur)->print(out, parser.m_tokdefs);
    fputs("\n",out);
  }
  fputs("\n",out);
}

#define SHIFT (1)
#define REDUCE (2)
#define REDUCE2 (4)

void PrintStatesAndActions(const ParserDef &parser, const ParserSolution &solution, const Map<int,SymbolDef> &tokens, FILE *out) {
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
          fputs(tokens[*cursym].m_name.c_str(), out);
          shiftsymbols.insert(*cursym);
          symbols.insert(*cursym);
        }
        fputs("\n",out);
      }
    }
    reducemap_t::const_iterator reduceiter = solution.m_reductions.find(i);
    if( reduceiter != solution.m_reductions.end() ) {
      for( reducebysymbols_t::const_iterator curreduce = reduceiter->second.begin(), endreduce = reduceiter->second.end(); curreduce != endreduce; ++curreduce ) {
        fputs("reduce by production [", out);
        curreduce->first->print(out,tokens);
        fputs("] on ", out);
        bool bFirst = true;
        for( Set<int>::const_iterator cursym = curreduce->second.begin(), endsym = curreduce->second.end(); cursym != endsym; ++cursym ) {
          const char *symstr = (*cursym==-1) ? "$" : tokens[*cursym].m_name.c_str();
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
      const char *symstr = (sym==-1) ? "$" : tokens[sym].m_name.c_str();
      if( shiftsymbols.find(sym) != shiftsymbols.end() && reductions.find(sym) != reductions.end() )
        fprintf(out, "shift/reduce conflict on %s\n", symstr);
      if( reductions.find(sym) != reductions.end() && reductions.find(sym)->second.size() > 1 )
        fprintf(out, "reduce/reduce conflict on %s\n", symstr);
    }
    fputs("\n",out);
  }
  fputs("\n",out);
}

void PrintConflicts(const ParserDef &parser, const ParserSolution &solution, const Map<int,SymbolDef> &tokens, FILE *pout) {
  fprintf(pout, "%d states\n\n", solution.m_states.size());
  for( int i = 0, n = solution.m_states.size(); i != n; ++i ) {
    Set<int> symbols;
    Set<int> shiftsymbols;
    Map<int,Set<const Production*> > reductions;
    const state_t &state = solution.m_states[i];
    shiftmap_t::const_iterator shiftiter = solution.m_shifts.find(i);
    if( shiftiter != solution.m_shifts.end() ) {
      for( shifttosymbols_t::const_iterator curshift = shiftiter->second.begin(), endshift = shiftiter->second.end(); curshift != endshift; ++curshift ) {
        for( Set<int>::const_iterator cursym = curshift->second.begin(), endsym = curshift->second.end(); cursym != endsym; ++cursym ) {
          shiftsymbols.insert(*cursym);
          symbols.insert(*cursym);
        }
      }
    }
    reducemap_t::const_iterator reduceiter = solution.m_reductions.find(i);
    if( reduceiter != solution.m_reductions.end() ) {
      for( reducebysymbols_t::const_iterator curreduce = reduceiter->second.begin(), endreduce = reduceiter->second.end(); curreduce != endreduce; ++curreduce ) {
        for( Set<int>::const_iterator cursym = curreduce->second.begin(), endsym = curreduce->second.end(); cursym != endsym; ++cursym ) {
          const char *symstr = (*cursym==-1) ? "$" : tokens[*cursym].m_name.c_str();
          reductions[*cursym].insert(curreduce->first);
          symbols.insert(*cursym);
        }
      }
    }
    bool bFirst = true;
    for( Set<int>::const_iterator cursym = symbols.begin(), endsym = symbols.end(); cursym != endsym; ++cursym ) {
      int sym = *cursym;
      const char *symstr = (sym==-1) ? "$" : tokens[sym].m_name.c_str();
      if( shiftsymbols.find(sym) != shiftsymbols.end() && reductions.find(sym) != reductions.end() ) {
        if( bFirst ) {
          bFirst = false;
          fprintf(pout, "State %d:\n", i);
        }
        fprintf(pout, "shift/reduce conflict on %s\n", symstr);
      }
      if( reductions.find(sym) != reductions.end() && reductions.find(sym)->second.size() > 1 ) {
        if( bFirst ) {
          bFirst = false;
          fprintf(pout, "State %d:\n", i);
        }
        fprintf(pout, "reduce/reduce conflict on %s\n", symstr);
      }
    }
    if( ! bFirst )
      fputs("\n",pout);
  }
}

void StringToInt_2_IntToString(const Map<String,int> &src, Map<int,String> &tokens);

void SolveParser(ParserDef &parser, ParserSolution &solution, FILE *vout, int verbosity) {
  FILE *out = stderr;
  if( ! parser.getStartProduction() )
    error("The grammar definition requires a START production");
  parser.computeForbidAutomata();
  Map< ProductionsAtKey,Vector<productionandforbidstate_t> > productionsAtResults;
  ComputeFirsts(parser,solution,out,verbosity,productionsAtResults);
  ComputeFollows(parser,solution,out,verbosity,productionsAtResults);
  ComputeStatesAndActions(parser,solution,out,verbosity,productionsAtResults);
  if( verbosity >= 1 ) {
    PrintRules(parser,vout);
    PrintStatesAndActions(parser,solution,parser.m_tokdefs,vout);
  } else
    PrintConflicts(parser,solution,parser.m_tokdefs,vout);
}
