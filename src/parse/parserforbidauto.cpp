#include "parser.h"

static void error(const String &err) {
  throw ParserError(err);
}

ForbidDescriptor::ForbidDescriptor() : m_positions(0), m_forbidden(0) {}
ForbidDescriptor::ForbidDescriptor(const String &name, ProductionDescriptors *positions, ProductionDescriptors *forbidden) : m_name(name), m_positions(positions), m_forbidden(forbidden) {}
ForbidDescriptor::ForbidDescriptor(const ForbidDescriptor &rhs) {
  m_name = rhs.m_name;
  m_positions = rhs.m_positions;
  m_forbidden = rhs.m_forbidden;
}

bool ForbidDescriptor::forbids(const Production *positionProduction, int pos, const Production *expandProduction) const {
  return m_positions->matchesProductionAndPosition(positionProduction,pos) && m_forbidden->matchesProduction(expandProduction);
}

bool ForbidDescriptor::operator<(const ForbidDescriptor &rhs) const {
  if( m_forbidden < rhs.m_forbidden )
    return true;
  return false;
}

ForbidSub::ForbidSub() : m_lhs(0), m_rhs(0) {}
ForbidSub::ForbidSub(const ProductionDescriptors *lhs, const ProductionDescriptors *rhs) : m_lhs(lhs), m_rhs(rhs) {}
ForbidSub::ForbidSub(const ForbidSub &rhs) : m_lhs(rhs.m_lhs), m_rhs(rhs.m_rhs) {}

bool ForbidSub::operator<(const ForbidSub &rhs) const {
  if( m_lhs < rhs.m_lhs )
    return true;
  if( rhs.m_lhs < m_lhs )
    return false;
  if( m_rhs < rhs.m_rhs )
    return true;
  return false;
}

bool ForbidSub::matches(const Production *lhs, int pos, const Production *rhs) const {
  return m_lhs->matchesProductionAndPosition(lhs,pos) && m_rhs->matchesProduction(rhs);
}

static String NameIndexToName(int idx) {
  String name;
  if( idx == 0 )
    name = "A";
  else {
    char namestr[10];
    namestr[9] = 0;
    char *s = namestr+9; 
    while( idx ) {
      --s;
      *s = (idx%26)+'A';
      idx /= 26;
    }
    name = s;
  }
  return name;
}

ForbidAutomata::ForbidAutomata() : m_nextstate(0), m_nxtnameidx(0) {
  m_q0 = newstate();
}

int ForbidAutomata::newstate() { return m_nextstate++; }

int ForbidAutomata::nstates() const {
  return m_nextstate;
}

void ForbidAutomata::addEmptyTransition(int s0, int s1) {
  m_emptytransitions[s0].insert(s1);
}

void ForbidAutomata::addTransition(int s0, int s1, const ProductionDescriptors *subLhs, const ProductionDescriptors *subRhs) {
  m_transitions[s0][ForbidSub(subLhs,subRhs)].insert(s1);
}

// Find *the* state after the current state, assuming this is a deterministic automata
int ForbidAutomata::nextState(const Production *curp, int pos, int forbidstateno, const Production *ptest) const {
  if( isForbidden(curp,pos,forbidstateno,ptest) )
    return -1;
  if( m_transitions.find(forbidstateno) != m_transitions.end() ) {
    const Map< ForbidSub,Set<int> > &t = m_transitions[forbidstateno];
    for( Map<ForbidSub,Set<int> >::const_iterator cursub = t.begin(), endsub = t.end(); cursub != endsub; ++cursub )
      if( cursub->first.matches(curp,pos,ptest) && cursub->second.size() )
        return *cursub->second.begin();
  }
  // Every other transition goes to 0.
  return 0;
}

bool ForbidAutomata::isForbidden(const Production *curp, int pos, int forbidstateno, const Production *ptest) const {
  if( pos < 0 || pos >= curp->m_symbols.size() || ptest->m_nt != curp->m_symbols[pos] )
    return true;
  if( forbidstateno < 0 || forbidstateno >= m_nextstate )
    return true;
  StateToForbids::const_iterator iter = m_statetoforbids.find(forbidstateno);
  if( iter == m_statetoforbids.end() )
    return false;
  for( Set<ForbidDescriptor>::const_iterator curfb = iter->second.begin(), endfb = iter->second.end(); curfb != endfb; ++curfb )
    if( curfb->forbids(curp,pos,ptest) )
      return true;
  return false;
}

void ForbidAutomata::addRule(const DisallowRule *rule) {
  ProductionDescriptors *nextDesc = 0;
  ProductionDescriptors *disallowed = rule->m_disallowed;
  int qNext = -1;
  for( int i = rule->m_intermediates.size()-1; i >= 0; --i ) {
    int qCur = newstate();
    DisallowProductionDescriptors *ddesc = rule->m_intermediates[i];
    ProductionDescriptors *desc = ddesc->m_descriptors;
    if( nextDesc )
      addTransition(qCur,qNext,desc->clone(),nextDesc->UnDottedProductionDescriptors());
    if( disallowed ) {
      m_statetoforbids[qCur].insert(ForbidDescriptor(NameIndexToName(m_nxtnameidx++),desc->clone(),disallowed->clone()));
      if( ! ddesc->m_star )
        disallowed = 0;
    }
    if( ddesc->m_star )
      addTransition(qCur,qCur,desc->clone(),desc->UnDottedProductionDescriptors());
    qNext = qCur;
    nextDesc = desc;
  }
  int qFirst = newstate();
  ProductionDescriptors *desc = rule->m_lead;
  if( nextDesc )
    addTransition(qFirst,qNext,desc->clone(),nextDesc->UnDottedProductionDescriptors());
  if( disallowed )
    m_statetoforbids[qFirst].insert(ForbidDescriptor(NameIndexToName(m_nxtnameidx++),desc->clone(),disallowed->clone()));
  addEmptyTransition(m_q0,qFirst);
}

void ForbidAutomata::closure(Set<int> &multistate) const {
  int lastsize = 0;
  Set<int> snapshot;
  do {
    lastsize = multistate.size();
    snapshot = multistate;
    for( Set<int>::const_iterator curstate = snapshot.begin(), endstate = snapshot.end(); curstate != endstate; ++curstate ) {
      if( m_emptytransitions.contains(*curstate) ) {
        const Set<int> &nextstates = m_emptytransitions[*curstate];
        multistate.insert(nextstates.begin(), nextstates.end());
      }
    }
  } while( multistate.size() > lastsize );
}

void ForbidAutomata::ForbidsFromStates(const Set<int> &stateset, ForbidDescriptors &forbids) const {
  for( Set<int>::const_iterator curstate = stateset.begin(), endstate = stateset.end(); curstate != endstate; ++curstate ) {
    int state = *curstate;
    if( m_statetoforbids.contains(state) ) {
      const ForbidDescriptors &stateforbids = m_statetoforbids[state];
      forbids.insert(stateforbids.begin(), stateforbids.end());
    }
  }
}

void ForbidAutomata::SymbolsFromStates(const Set<int> &stateset, Set<ForbidSub> &symbols) const {
  for( Set<int>::const_iterator curstate = stateset.begin(), endstate = stateset.end(); curstate != endstate; ++curstate ) {
    int state = *curstate;
    if( m_transitions.contains(state) ) {
      const Map<ForbidSub,Set<int> > &statetransitions = m_transitions[state];
      for( Map<ForbidSub,Set<int> >::const_iterator curtrans = statetransitions.begin(), endtrans = statetransitions.end(); curtrans != endtrans; ++curtrans )
        symbols.insert(curtrans->first);
    }
  }
}

void ForbidAutomata::toDeterministicForbidAutomata(ForbidAutomata &out) const {
  Vector< Set<int> > statesets;
  Map< Set<int>, int > stateset2state;
  Set<int> initstate;
  initstate.insert(m_q0);
  closure(initstate);
  stateset2state[initstate] = out.m_q0;
  statesets.push_back(initstate);
  for( int i = 0; i < statesets.size(); ++i ) {
    Set<int> &stateset = statesets[i];
    ForbidDescriptors forbids;
    Set<ForbidSub> symbols;
    ForbidsFromStates(stateset,forbids);
    SymbolsFromStates(stateset,symbols);
    out.m_statetoforbids[i] = forbids;
    
    for( Set<ForbidSub>::const_iterator cursym = symbols.begin(), endsym = symbols.end(); cursym != endsym; ++cursym ) {
      Set<int> nextstate;
      const ForbidSub &sub = *cursym;
      for( Set<int>::const_iterator curstate = stateset.begin(), endstate = stateset.end(); curstate != endstate; ++curstate ) {
        int state = *curstate;
        if( m_transitions.contains(state) ) {
          const Map<ForbidSub,Set<int> > &statetransitions = m_transitions[state];
          if( statetransitions.contains(sub) ) {
            const Set<int> &nxtstates = statetransitions[sub];
            nextstate.insert(nxtstates.begin(),nxtstates.end());
          }
        }
      }
      nextstate.insert(m_q0); // all states should also transition to the initial state
      closure(nextstate);
      int nextStateNo = -1;
      if( ! stateset2state.contains(nextstate) ) {
        nextStateNo = out.newstate();
        stateset2state[nextstate] = nextStateNo;
        statesets.push_back(nextstate);
      } else
        nextStateNo = stateset2state[nextstate];
      // no need to add transition to 0, default is transition to 0
      if( nextStateNo != 0 )
        out.addTransition(i,nextStateNo,sub.m_lhs,sub.m_rhs);
    } 
  }
}

void ForbidDescriptor::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
  fputs(m_name.c_str(),out);
  fputs(": ", out);
  m_positions->print(out,tokens);
  fputs(" -/-> ",out);
  m_forbidden->print(out,tokens);
}

void ForbidSub::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
  m_lhs->print(out,tokens);
  fputs(" --> ", out);
  m_rhs->print(out,tokens);
}

void ForbidAutomata::print(FILE *out, const Map<int,SymbolDef> &tokens) const {
  fprintf(out, "%d states\n", m_nextstate);
  fprintf(out, "start state is %d\n", m_q0);
  for( int i = 0; i < m_nextstate; ++i ) {
    fprintf(out, "Forbid State %d:\n", i);
    if( m_statetoforbids.find(i) != m_statetoforbids.end() ) {
      const ForbidDescriptors &forbid = m_statetoforbids[i];
      fputs("  Forbids:\n", out);
      for( Set<ForbidDescriptor>::const_iterator cur = forbid.begin(), end = forbid.end(); cur != end; ++cur ) {
        fputs("    ", out);
        cur->print(out,tokens);
        fputs("\n",out);
      }
    }
    if( m_emptytransitions.find(i) != m_emptytransitions.end() ) {
      const Set<int> &empties = m_emptytransitions[i];
      for( Set<int>::const_iterator cur = empties.begin(), end = empties.end(); cur != end; ++cur )
        fprintf(out, "epsilon transition to %d\n", *cur);
    }
    if( m_transitions.find(i) != m_transitions.end() ) {
      const Map< ForbidSub, Set<int> > &trans = m_transitions[i];
      for( Map<ForbidSub,Set<int> >::const_iterator cur = trans.begin(), end = trans.end(); cur != end; ++cur ) {
        fputs("  on input ", out);
        cur->first.print(out,tokens);
        fputs("\n", out);
        for( Set<int>::const_iterator cur2 = cur->second.begin(), end2 = cur->second.end(); cur2 != end2; ++cur2 )
          fprintf(out, "    -> move to state %d\n", *cur2);
      }
    }
  }
}

void StringToInt_2_IntToString(const Map<String,int> &src, Map<int,String> &tokens) {
  for( Map<String,int>::const_iterator tok = src.begin(); tok != src.end(); ++tok )
    tokens[tok->second] = tok->first;
}

void ParserDef::computeForbidAutomata() {
  ForbidAutomata nforbid, forbid;
  // Expand and combine the rules
  expandAssocRules();
  expandPrecRules();
  combineRules();
  if( m_verbosity > 2 ) {
    fputs("final disallow rules:\n", m_vout);
    for( int i = 0; i < m_disallowrules.size(); ++i ) {
      m_disallowrules[i]->print(m_vout,m_tokdefs);
      fputs("\n",m_vout);
    }
  }
  // Turn the rules into a nondeterministic forbid automata
  for( Vector<DisallowRule*>::const_iterator cur = m_disallowrules.begin(), end = m_disallowrules.end(); cur != end; ++cur )
    nforbid.addRule(*cur);
  if( m_verbosity > 2 ) {
    fputs("final non-deterministic forbid automata:\n", m_vout);
      nforbid.print(m_vout,m_tokdefs);
  }
  // Make the automata deterministic.
  nforbid.toDeterministicForbidAutomata(forbid);
  m_forbid = forbid;
  if( m_verbosity > 2 ) {
    fputs("final deterministic forbid automata:\n", m_vout);
      m_forbid.print(m_vout,m_tokdefs);
  }
}

