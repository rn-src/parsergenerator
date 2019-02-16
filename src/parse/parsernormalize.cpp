#include "parser.h"

class gnode {
public:
  Production *m_p;
  int m_stateNo;
  gnode(Production *p, int stateNo) : m_p(p), m_stateNo(stateNo) {}
  gnode() : m_p(0), m_stateNo(0) {}

  bool operator<(const gnode &rhs) const {
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

  bool operator==(const gnode &rhs) const {
    return m_p == rhs.m_p && m_stateNo == rhs.m_stateNo;
  }
};

class ForbidDescriptor {
public:
  String m_name;
  ProductionDescriptors *m_forbidden;

  bool forbids(const Production *p) const {
    for( ProductionDescriptors::iterator curdesc = m_forbidden->begin(), enddesc = m_forbidden->end(); curdesc != enddesc; ++curdesc ) {
      ProductionDescriptor *pd = *curdesc;
      if( pd->matchesProduction(p,true) )
        return true;
    }
    return false;
  }
};

typedef Vector<ForbidDescriptor> ForbidDescriptors;

typedef Map<int,ForbidDescriptors> StateToForbids;

class ForbidSub {
public:
  ProductionDescriptors *m_lhs, *m_rhs;
  ForbidSub() : m_lhs(0), m_rhs(0) {}
  ForbidSub(ProductionDescriptors *lhs, ProductionDescriptors *rhs) : m_lhs(lhs), m_rhs(rhs) {}
  ForbidSub(const ForbidSub &rhs) : m_lhs(rhs.m_lhs), m_rhs(rhs.m_rhs) {}

  bool operator<(const ForbidSub &rhs) const {
    if( m_lhs < rhs.m_lhs )
      return true;
    if( rhs.m_lhs < m_lhs )
      return false;
    if( m_rhs < rhs.m_rhs )
      return true;
    return false;
  }

  bool matches(const Production *lhs, const Production *rhs) const {
    return m_lhs->matchesProduction(lhs,true) && m_rhs->matchesProduction(rhs,true);
  }
};

class ForbidAutomata {
  int m_nextstate;
  ParserDef &m_parser;
  int m_q0;
  StateToForbids m_statetoforbids;
  Map< int,Set<int> > m_emptytransitions;
  Map< int,Map< ForbidSub,Set<int> > > m_transitions;
public:
  int newstate() { return m_nextstate++; }

  ForbidAutomata(ParserDef &parser) : m_nextstate(0), m_parser(parser) {
    m_q0 = newstate();
  }

  void addEmptyTransition(int s0, int s1) {
    m_emptytransitions[s0].insert(s1);
  }

  void addTransition(int s0, int s1, ProductionDescriptors *subLhs, ProductionDescriptors *subRhs) {
    m_transitions[s0][ForbidSub(subLhs,subRhs)].insert(s1);
  }

  // Find *the* state after the current state, assuming this is a deterministic automata
  int nextState(const gnode &g, Production *p) {
    if( m_transitions.find(g.m_stateNo) == m_transitions.end() )
      return 0; // "other" -> 0
    const Map< ForbidSub,Set<int> > &t = m_transitions[g.m_stateNo];
    for( Map<ForbidSub,Set<int> >::const_iterator cursub = t.begin(), endsub = t.end(); cursub != endsub; ++cursub ) {
      if( cursub->first.matches(g.m_p,p) ) {
        if( cursub->second.size() == 0 )
          return 0; // "other" -> 0
        return *cursub->second.begin();
      }
    }
    return 0; // "other" -> 0
  }

  void addRule(const DisallowRule *rule) {
    int qLast = newstate();
    addEmptyTransition(m_q0,qLast);
    ProductionDescriptors *prevDesc = rule->m_lead;
    for( int i = 0, n = rule->m_intermediates.size(); i < n; ++i ) {
      int qNext = newstate();
      DisallowProductionDescriptors *ddesc = rule->m_intermediates[i];
      ProductionDescriptors *desc = ddesc->m_descriptors;
      addTransition(qLast,qNext,prevDesc,desc);
      if( ddesc->m_star )
        addEmptyTransition(qNext,qLast);
      qLast = qNext;
      prevDesc = desc;
    }
  }

  void expandNode(const gnode &k, Set<gnode> &nodes, const Set<gnode> &processednodes) {
    ForbidDescriptors &forbids = m_statetoforbids[k.m_stateNo];
    for( int i = 0, n = k.m_p->m_symbols.size(); i < n; ++i ) {
      int s = k.m_p->m_symbols[i];
      if( m_parser.getSymbolType(s) != SymbolTypeNonterminal )
        continue;
      // If any production is forbidden, we'll have to create a new nonterminal and productions
      String forbidName;
      Vector<Production*> productions = m_parser.productionsAt(k.m_p,i);
      for( ForbidDescriptors::iterator curforbid = forbids.begin(), endforbid = forbids.end(); curforbid != endforbid; ++curforbid ) {
        for( Vector<Production*>::iterator curp = productions.begin(); curp != productions.end(); ++curp ) {
          if( curforbid->forbids(*curp) ) {
            forbidName = curforbid->m_name;
            curp = productions.erase(curp);
            --curp;
          }
        }
      }
      if( forbidName.length() ) {
        String sName = m_parser.m_tokdefs[s].m_name;
        sName += "_";
        sName += forbidName;
        s = m_parser.findSymbolId(sName);
        if( s == -1 ) {
          int nexts = m_parser.addSymbolId(sName,SymbolTypeNonterminal,m_parser.getBaseTokId(s));
          s = nexts;
          k.m_p->m_symbols[i] = s;
          for( Vector<Production*>::iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
            Production *pClone = (*curp)->clone();
            pClone->m_nt = s;
            m_parser.addProduction(pClone);
          }
        } else {
          k.m_p->m_symbols[i] = s;
        }
      }
      for( Vector<Production*>::iterator curp = productions.begin(), endp = productions.end(); curp != endp; ++curp ) {
        int nextStateNo = nextState(k,*curp);
        gnode nextk(*curp,nextStateNo);
        if( processednodes.find(nextk) == processednodes.end() )
          nodes.insert(nextk);
      }
    }
  }

  void closure(Set<int> &multistate) const {
    int lastsize = 0;
    Set<int> nextmultistate = multistate;
    do {
      multistate = nextmultistate;
      for( Set<int>::const_iterator curstate = multistate.begin(), endstate = multistate.end(); curstate != endstate; ++curstate ) {
        if( m_emptytransitions.find(*curstate) == m_emptytransitions.end() )
          continue;
        const Set<int> &nextstates = m_emptytransitions[*curstate];
        multistate.insert(nextstates.begin(), nextstates.end());
      }
    } while( multistate != nextmultistate );
    return nextmultistate;
  }

  void toDeterministicForbidAutomata(ForbidAutomata &out) {
    Vector< Set<int> > states;
    Map< Set<int>, int > multi2state;
    states.push_back(Set<int>());
    states[0].insert(0);
    closure(states[0]);
    for( int i = 0; i < states.size(); ++i ) {
      Set<int> &multistate = states[i];
      Set<ForbidSub> symbols;
      // TODO
      for( Set<int>::const_iterator curstate = multistate.begin(), endstate = multistate.end(); curstate != endstate; ++curstate ) {
        int state = *curstate;
        Set<ForbidSub> statesymbols = m_forbids[state];
        symbols.insert(statesymbols.begin(), statesymbols.end());
      }
      int state = out.nextstate();
      multi2state[multistate] = state;
    }
  }
};

bool NormalizeParser(ParserDef &parser) {
  ForbidAutomata nforbid(parser), forbid(parser);
  int q0 = nforbid.newstate();
  String err;
  // Expand and combine the rules
  if( ! parser.expandAssocRules(err) || ! parser.expandPrecRules(err) || ! parser.combineRules(err) ) {
    fputs(err.c_str(),stderr);
    return false;
  }
  // Turn the rules into a nondeterministic forbid automata
  for( Vector<DisallowRule*>::const_iterator cur = parser.m_disallowrules.begin(), end = parser.m_disallowrules.end(); cur != end; ++cur )
    nforbid.addRule(*cur);
  // Make the automata deterministic.
  nforbid.toDeterministicForbidAutomata(forbid);
  // add the initial production/state
  Production *S = parser.getStartProduction();
  Set<gnode> nodes, processednodes;
  nodes.insert(gnode(S,0));
  // expand each production/state until closure
  while( nodes.size() ) {
    Set<gnode>::iterator curgnode = nodes.begin();
    gnode k = *curgnode;
    nodes.erase(curgnode);
    processednodes.insert(k);
    forbid.expandNode(k,nodes,processednodes);
  }
  // done
  return true;
}

