#include "parser.h"

namespace pptok {
#include "parsertok.h"
}

#include <stdint.h>
#ifdef WIN32
#include <windows.h>
#include <winnt.h>
#endif

typedef uint64_t ticks_t;

ticks_t getSystemTicks()
{
#ifdef WIN32
  LARGE_INTEGER timer;
  QueryPerformanceCounter(&timer);
  return timer.QuadPart;
#else
  //struct tms now;
  //times(&now);
  //return now.tms_stime;
  return 0;
#endif
}

ticks_t getSystemTicksFreq()
{
#ifdef WIN32
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return freq.QuadPart;
#else
  //return sysconf(_SC_CLK_TICK);
  return 1;
#endif
}

#define SHIFT (1)
#define REDUCE (2)
#define REDUCE2 (4)

void StringToInt_2_IntToString(const Map<String,int> &src, Map<int,String> &tokens);

class FirstAndFollowComputer {
  const ParserDef &parser;
  ParserSolution &solution;
  FILE *out;
  int verbosity;
  Map< ProductionsAtKey,Vector<productionandforbidstate_t> > &productionsAtResults;
  Set<productionandforbidstate_t> followproductionset;
  Vector<productionandforbidstate_t> followproductions;

public:

FirstAndFollowComputer(const ParserDef &_parser, ParserSolution &_solution, FILE *_out, int _verbosity,
  Map< ProductionsAtKey,Vector<productionandforbidstate_t> > &_productionsAtResults)
  : parser(_parser), solution(_solution), out(_out), verbosity(_verbosity), productionsAtResults(_productionsAtResults)
  {
  }

void ComputeFirsts() {
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
        if( p->m_symbols.size() == 0 )
          continue;
        int s = p->m_symbols[0];
        if( parser.getSymbolType(s) == SymbolTypeTerminal ) {          
          if( verbosity > 2 ) {
            if( ! solution.m_firsts[symbolandforbidstate_t(tokid,forbidstate)].contains(s) )
              fprintf(out,"Adding %s to FIRST(%s,%d)\n",parser.m_tokdefs[s].m_name.c_str(),parser.m_tokdefs[tokid].m_name.c_str(),forbidstate);
          }
          solution.m_firsts[symbolandforbidstate_t(tokid,forbidstate)].insert(s);
        } else {
          const Vector<productionandforbidstate_t> &productions = parser.productionsAt(p,0,forbidstate,productionsAtResults);
          for( Vector<productionandforbidstate_t>::const_iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
            int pid = curp->first->m_pid;
            int pfst = curp->second;
            if( solution.m_firsts.find(symbolandforbidstate_t(pid,pfst)) != solution.m_firsts.end() ) {
              Set<int> &dst = solution.m_firsts[symbolandforbidstate_t(tokid,forbidstate)];
              const Set<int> &sfirsts = solution.m_firsts[symbolandforbidstate_t(pid,pfst)];
              if( verbosity > 2 ) {
                Set<int> added = sfirsts;
                added.erase(dst.begin(),dst.end());
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


Vector< Pair< String,Set<int> > > getFollows(Production *p, int pos, int forbidstate) {
  Vector< Pair< String,Set<int> > > followslist;
  if( pos+1==p->m_symbols.size() ) {
    Pair< String,Set<int> > followitem;
    followitem.first = String::FormatString("FOLLOW(%s,%d)", parser.m_tokdefs[p->m_pid].m_name.c_str(), forbidstate);
    followitem.second = solution.m_follows[symbolandforbidstate_t(p->m_pid,forbidstate)];
    followslist.push_back(followitem);
  } else {
    int nexttok = p->m_symbols[pos+1];
    SymbolType stypenext = parser.getSymbolType(nexttok);
    if( stypenext == SymbolTypeTerminal ) {
      Pair< String,Set<int> > followitem;
      followitem.first = String::FormatString("FIRST(%s,%d)", parser.m_tokdefs[nexttok].m_name.c_str(), forbidstate);
      followitem.second.insert(nexttok);
      followslist.push_back(followitem);
    } else if( stypenext == SymbolTypeNonterminal ) {
      const Vector<productionandforbidstate_t> &productions = parser.productionsAt(p,pos+1,forbidstate,productionsAtResults);
      for( Vector<productionandforbidstate_t>::const_iterator curpat = productions.begin(), endpat = productions.end(); curpat != endpat; ++curpat ) {
        if( ! followproductionset.contains(*curpat) ) {
          followproductionset.insert(*curpat);
          followproductions.push_back(*curpat);
        }
        Pair< String,Set<int> > followitem;
        followitem.first = String::FormatString("FIRST(%s,%d)", parser.m_tokdefs[curpat->first->m_pid].m_name.c_str(), curpat->second);
        followitem.second = solution.m_firsts[symbolandforbidstate_t(curpat->first->m_pid,curpat->second)];
        followslist.push_back(followitem);
      }
    }
  }
  return followslist;
}

void ComputeFollows() {
  solution.m_follows[symbolandforbidstate_t(parser.getStartProduction()->m_pid,0)].insert(EOF_TOK);
  followproductionset.insert(productionandforbidstate_t(parser.getStartProduction(),0));
  followproductions.push_back(productionandforbidstate_t(parser.getStartProduction(),0));
  bool added = true;
  while( added ) {
    added = false;
    for( int i = 0; i < followproductions.size(); ++i ) {
      Production *p = followproductions[i].first;
      int forbidstate = followproductions[i].second;

      for( int pos = 0, endpos = p->m_symbols.size(); pos != endpos; ++pos ) {
        int tok = p->m_symbols[pos];
        SymbolType stype = parser.getSymbolType(tok);
        Vector< Pair<String,Set<int> > > followslist = getFollows(p,pos,forbidstate);
        if( verbosity > 2 ) {
          fputs("Evaluting ",out);
          p->print(out,parser.m_tokdefs,pos,forbidstate);
          fputs("\n",out);
        }

        if( stype == SymbolTypeTerminal ) {
          Set<int> &dst = solution.m_follows[symbolandforbidstate_t(tok,forbidstate)];
          for( int i = 0, n = followslist.size(); i < n; ++i ) {
            Pair< String,Set<int> > followitem = followslist[i];
            const String &descstr = followitem.first;
            const Set<int> &follows = followitem.second;
            if( verbosity > 2 ) {
              Set<int> adding = follows;
              adding.erase(dst.begin(),dst.end());
              if( adding.size() > 0 ) {
                fprintf(out, "%s {\n", descstr.c_str());
                for( Set<int>::iterator curadd = adding.begin(), endadd = adding.end(); curadd != endadd; ++curadd )
                  fprintf(out,"Adding %s to FOLLOW(%s,%d)\n",parser.m_tokdefs[*curadd].m_name.c_str(),parser.m_tokdefs[tok].m_name.c_str(),forbidstate);
                fprintf(out, "} %s\n", descstr.c_str());
              }
            }
            int before = dst.size(); 
            dst.insert(follows.begin(),follows.end());
            int after = dst.size();
            if( after > before )
              added = true;
          }
        }
        else {
          const Vector<productionandforbidstate_t> &productions = parser.productionsAt(p,pos,forbidstate,productionsAtResults);
          for( Vector<productionandforbidstate_t>::const_iterator curpat = productions.begin(), endpat = productions.end(); curpat != endpat; ++curpat ) {
            if( ! followproductionset.contains(*curpat) ) {
              followproductionset.insert(*curpat);
              followproductions.push_back(*curpat);
            }
            Set<int> &dst = solution.m_follows[symbolandforbidstate_t(curpat->first->m_pid,curpat->second)];
            for( int i = 0, n = followslist.size(); i < n; ++i ) {
              Pair< String,Set<int> > followitem = followslist[i];
              const String &descstr = followitem.first;
              const Set<int> &follows = followitem.second;
              if( verbosity > 2 ) {
                Set<int> adding = follows;
                adding.erase(dst.begin(),dst.end());
                if( adding.size() > 0 ) {
                  fprintf(out, "%s {\n", descstr.c_str());
                  for( Set<int>::iterator curadd = adding.begin(), endadd = adding.end(); curadd != endadd; ++curadd )
                    fprintf(out,"Adding %s to FOLLOW(%s,%d)\n", parser.m_tokdefs[*curadd].m_name.c_str(),parser.m_tokdefs[curpat->first->m_pid].m_name.c_str(),curpat->second);
                  fprintf(out, "} %s\n", descstr.c_str());
                }
              }
              int before = dst.size();
              dst.insert(follows.begin(),follows.end());
              int after = dst.size();
              if( after > before )
                added = true;
            }
          }
        }
      }
    }
  }
}

void closure(state_t &state) {
  int prevsize = 0;
  Set<ProductionState> newparts;
  while( state.size() > prevsize ) {
    prevsize = state.size();
    newparts.clear();
    for( Set<ProductionState>::const_iterator cur = state.begin(), end = state.end(); cur != end; ++cur ) {
      const Vector<productionandforbidstate_t> &productions = parser.productionsAt(cur->m_p,cur->m_pos,cur->m_forbidstate,productionsAtResults);
      for( Vector<productionandforbidstate_t>::const_iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
        ProductionState ps(curp->first,0,curp->second);
        newparts.insert(ps);
      }
    }
    for( Set<ProductionState>::const_iterator curpart = newparts.begin(), endpart = newparts.end(); curpart != endpart; ++curpart )
      state.insert(*curpart);
  }
}

void nexts(const state_t &state, Set<int> &nextSymbols) {
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

void advance(state_t &state, int tsymbol, state_t &nextState) {
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

void ComputeStatesAndActions() {
  Map<state_t,int> statemap;
  state_t state, nextState;
  Set<int> nextSymbols;
  state.insert(ProductionState(parser.getStartProduction(),0,0));
  closure(state);
  statemap[state] = solution.m_states.size();
  solution.m_states.push_back(state);
  for( int i = 0; i < solution.m_states.size(); ++i ) {
    if( verbosity > 2 )
      fprintf(out,"computing state %d\n",i);
    state = solution.m_states[i];
    int stateIdx = i;
    nexts(state,nextSymbols);
    for( Set<int>::iterator cur = nextSymbols.begin(), end = nextSymbols.end(); cur != end; ++cur ) {
      int symbol = *cur;
      advance(state,symbol,nextState);
      closure(nextState);
      if( nextState.empty() )
        continue;
      Map<state_t,int>::iterator stateiter = statemap.find(nextState);
      int nextStateIdx = -1;
      if( stateiter == statemap.end() ) {
        nextStateIdx = solution.m_states.size();
        statemap[nextState] = nextStateIdx;
        solution.m_states.push_back(nextState);
        if( verbosity > 2 )
          fprintf(out,"added state %d\n",nextStateIdx);
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

void PrintRules() {
  int i = 1;
  for( Vector<Production*>::const_iterator cur = parser.m_productions.begin(), end = parser.m_productions.end(); cur != end; ++cur ) {
    fprintf(out, "#%d ", i++);
    (*cur)->print(out, parser.m_tokdefs);
    fputs("\n",out);
  }
  fputs("\n",out);
}

void PrintStatesAndActions() {
  const Map<int,SymbolDef> &tokens = parser.m_tokdefs;
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
      Vector<Production*> reduces;
      for( reducebysymbols_t::const_iterator curreduce = reduceiter->second.begin(), endreduce = reduceiter->second.end(); curreduce != endreduce; ++curreduce )
        reduces.push_back(curreduce->first);
      if( reduces.size() > 1 )
        qsort(reduces.begin(),reduces.size(),sizeof(Production*),Production::cmpprdid);
      for( Vector<Production*>::const_iterator curp = reduces.begin(), endp = reduces.end(); curp != endp; ++curp ) {
        Production *p = *curp;
        const Set<int> &syms = (reduceiter->second)[p];
        if( p->m_rejectable )
          fputs("(rejectable) ", out);
        fputs("reduce by production [", out);
        p->print(out,tokens);
        fputs("] on ", out);
        bool bFirst = true;
        for( Set<int>::const_iterator cursym = syms.begin(), endsym = syms.end(); cursym != endsym; ++cursym ) {
          const char *symstr = (*cursym==-1) ? "$" : tokens[*cursym].m_name.c_str();
          if( bFirst )
            bFirst = false;
          else
            fputs(", ", out);
          fputs(symstr, out);
          reductions[*cursym].insert(p);
          symbols.insert(*cursym);
        }
        fputs("\n",out);
      }
    }
    for( Set<int>::const_iterator cursym = symbols.begin(), endsym = symbols.end(); cursym != endsym; ++cursym ) {
      int sym = *cursym;
      if( reductions.find(sym) == reductions.end() )
        continue; // there can be no conflicts if there are no reductions
      const char *symstr = (sym==-1) ? "$" : tokens[sym].m_name.c_str();
      if( shiftsymbols.find(sym) != shiftsymbols.end() )
        fprintf(out, "shift/reduce conflict on %s\n", symstr);
      Vector<const Production*> symreductions;
      symreductions.insert(symreductions.end(), reductions[sym].begin(), reductions[sym].end());
      for( int r0 = 0; r0 < symreductions.size(); ++r0 )
        for( int r1 = r0+1; r1 < symreductions.size(); ++r1 )
          if( !symreductions[r0]->m_rejectable && ! symreductions[r1]->m_rejectable )
            fprintf(out, "reduce/reduce conflict on %s\n", symstr);
    }
    fputs("\n",out);
  }
  fputs("\n",out);
}

int PrintConflicts() {
  int nconflicts = 0;
  const Map<int,SymbolDef> &tokens = parser.m_tokdefs;
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
      if( reductions.find(sym) == reductions.end() )
        continue; // there can be no conflicts if there are no reductions
      const char *symstr = (sym==-1) ? "$" : tokens[sym].m_name.c_str();
      if( shiftsymbols.find(sym) != shiftsymbols.end() ) {
        if( bFirst ) {
          bFirst = false;
          fprintf(stderr, "State %d:\n", i);
        }
        fprintf(stderr, "shift/reduce conflict on %s\n", symstr);
        ++nconflicts;
      }
      Vector<const Production*> symreductions;
      symreductions.insert(symreductions.end(), reductions[sym].begin(), reductions[sym].end());
      for( int r0 = 0; r0 < symreductions.size(); ++r0 )
        for( int r1 = r0+1; r1 < symreductions.size(); ++r1 )
          if( !symreductions[r0]->m_rejectable && ! symreductions[r1]->m_rejectable ) {
            if( bFirst ) {
              bFirst = false;
              fprintf(stderr, "State %d:\n", i);
            }
            fprintf(stderr, "reduce/reduce conflict on %s\n", symstr);
            ++nconflicts;
          }
    }
    if( ! bFirst )
      fputs("\n",stderr);
  }
  if( nconflicts )
    fprintf(stderr, "%d conflicts\n", nconflicts);
  return nconflicts;
}

int SolveParser() {
  ComputeFirsts();
  ComputeFollows();
  ComputeStatesAndActions();
  int nconflicts = PrintConflicts();
  if( verbosity >= 1 ) {
    PrintRules();
    PrintStatesAndActions();
  }
  return nconflicts;
}

}; // end of class

int SolveParser(ParserDef &parser, ParserSolution &solution, FILE *vout, int verbosity, int timed) {
  if( ! parser.getStartProduction() ) {
    fputs("The grammar definition requires a START production",stderr);
    return -1;
  }
  double start = getSystemTicks();
  parser.computeForbidAutomata();
  Map< ProductionsAtKey,Vector<productionandforbidstate_t> > productionsAtResults;
  FirstAndFollowComputer firstAndFollowComputer(parser,solution,vout,verbosity,productionsAtResults);
  int result = firstAndFollowComputer.SolveParser();
  double stop = getSystemTicks();
  if( timed ) {
    printf("Tokens %d\n", parser.m_tokens.size());
    printf("Productions %d\n", parser.m_productions.size());
    printf("States %d\n", solution.m_states.size());
    printf("Duration %.6f\n", (double)(stop-start)/(double)getSystemTicksFreq());
  }
  return result;
}
