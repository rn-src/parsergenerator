#include "parser.h"

static void error(const String &err) {
  throw ParserError(err);
}

gnode::gnode(Production *p, int stateNo) : m_p(p), m_stateNo(stateNo) {}
gnode::gnode() : m_p(0), m_stateNo(0) {}

bool gnode::operator<(const gnode &rhs) const {
  if( m_p < rhs.m_p )
    return true;
  else if( rhs.m_p < m_p )
    return false;
  if( m_stateNo < rhs.m_stateNo )
    return true;
  else if( rhs.m_stateNo < m_stateNo )
    return false;
  return false;
}

bool gnode::operator==(const gnode &rhs) const {
  return m_p == rhs.m_p && m_stateNo == rhs.m_stateNo;
}

void gnode::print(FILE *out, const Map<int,String> &tokens) const {
  fprintf(out,"{state=%d,[",m_stateNo);
  m_p->print(out,tokens);
  fputs("]}",out);
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

void ForbidAutomata::addEmptyTransition(int s0, int s1) {
  m_emptytransitions[s0].insert(s1);
}

void ForbidAutomata::addTransition(int s0, int s1, const ProductionDescriptors *subLhs, const ProductionDescriptors *subRhs) {
  m_transitions[s0][ForbidSub(subLhs,subRhs)].insert(s1);
}

// Find *the* state after the current state, assuming this is a deterministic automata
int ForbidAutomata::nextState(const Production *curp, int pos, int stateno, const Production *p) const {
  if( m_transitions.find(stateno) == m_transitions.end() )
    return 0; // "other" -> 0
  const Map< ForbidSub,Set<int> > &t = m_transitions[stateno];
  for( Map<ForbidSub,Set<int> >::const_iterator cursub = t.begin(), endsub = t.end(); cursub != endsub; ++cursub ) {
    if( cursub->first.matches(curp,pos,p) ) {
      if( cursub->second.size() == 0 )
        return 0; // "other" -> 0
      return *cursub->second.begin();
    }
  }
  return 0; // "other" -> 0
}

bool ForbidAutomata::isForbidden(const Production *p, int pos, int forbidstate, const Production *ptest) const {
  if( pos < 0 || pos >= p->m_symbols.size() || ptest->m_nt != p->m_symbols[pos] )
    return true;
  StateToForbids::const_iterator iter = m_statetoforbids.find(forbidstate);
  if( iter == m_statetoforbids.end() )
    return true;
  for( Set<ForbidDescriptor>::const_iterator curfb = iter->second.begin(), endfb = iter->second.end(); curfb != endfb; ++curfb )
    if( curfb->forbids(p,pos,ptest) )
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

void ForbidAutomata::toDeterministicForbidAutomata(ForbidAutomata &out) {
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
  // Turn the rules into a nondeterministic forbid automata
  for( Vector<DisallowRule*>::const_iterator cur = m_disallowrules.begin(), end = m_disallowrules.end(); cur != end; ++cur )
    nforbid.addRule(*cur);
  // Make the automata deterministic.
  nforbid.toDeterministicForbidAutomata(forbid);
  m_forbid = forbid;
}

