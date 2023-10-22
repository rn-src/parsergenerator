#include "tokenizer.h"
#include "tests.h"

#define TEST_NFA_NEXTSTATE(nextstate) if( ! test_nfa_nextstate(&nfa,nextstate) ) return false;
#define TEST_NFA_STATECOUNT(count) if( ! test_nfa_nextstate(&nfa,count) ) return false;
#define TEST_NFA_SECTIONS(sections) if( ! test_nfa_sections(&nfa,sections) ) return false;
#define TEST_NFA_STARTCOUNT(count) if( ! test_nfa_startcount(&nfa,count) ) return false;
#define TEST_NFA_ENDCOUNT(count) if( ! test_nfa_endcount(&nfa,count) ) return false;
#define TEST_NFA_HASSTARTSTATE(state,expected) if( ! test_nfa_hasstartstate(&nfa,state,expected) ) return false;
#define TEST_NFA_HASENDSTATE(state,expected) if( ! test_nfa_hasendstate(&nfa,state,expected) ) return false;
#define TEST_NFA_HASENDSTATE_MANY(test,expected) if( ! test_nfa_hasendstate_many(&nfa,test,expected) ) return false;
#define TEST_NFA_HASEMPTYTRANSITION(from,to) if( ! test_nfa_has_empty_transition(&nfa,from,to) ) return false;
#define TEST_NFA_HASTRANSITION(from,to,s) if( ! test_nfa_has_transition(&nfa,from,to,s) ) return false;
#define TEST_NFA_HASSTATETOKEN(state,token) if( ! test_nfa_has_state_token(&nfa,state,token) ) return false;

static bool test_nfa_nextstate(Nfa *nfa, int nextstate) {
  if( nfa->m_nextState != nextstate ) {
    fprintf(stdout, "expectd nfa.nextstate=%d, got %d", nextstate, nfa->m_nextState);
    return false;
  }
  return true;
}

static bool test_nfa_sections(Nfa *nfa, int sections) {
  if( nfa->m_sections != sections ) {
    fprintf(stdout, "expected nfa.sections=%d, got %d", sections, nfa->m_sections);
    return false;
  }
  return true;
}

static bool test_nfa_startcount(Nfa *nfa, int count) {
  if( SetAny_size(&nfa->m_startStates) != count ) {
    fprintf(stdout, "expected len(nfa.startstates)=%d, got %d", count, SetAny_size(&nfa->m_startStates));
    return false;
  }
  return true;
}

static bool test_nfa_endcount(Nfa *nfa, int count) {
  if( SetAny_size(&nfa->m_endStates) != count ) {
    fprintf(stdout, "expected len(nfa.endstates)=%d, got %d", count, SetAny_size(&nfa->m_startStates));
    return false;
  }
  return true;
}

static bool test_nfa_hasstartstate(Nfa *nfa, int state, bool expected) {
  if( SetAny_contains(&nfa->m_startStates,&state) != expected ) {
    fprintf(stdout, "expected nfa.startstates.contains(%d) -> %s", state, expected ? "true" : "false");
    return false;
  }
  return true;
}

static bool test_nfa_hasendstate(Nfa *nfa, int state, bool expected) {
  if( SetAny_contains(&nfa->m_endStates,&state) != expected ) {
    fprintf(stdout, "expected nfa.endstates.contains(%d) -> %s", state, expected ? "true" : "false");
    return false;
  }
  return true;
}

static bool test_nfa_hasendstate_many(Nfa *nfa, SetAny /*<int>*/ *states, bool expected) {
  if( Nfa_hasEndState(nfa,states) != expected ) {
    fputs("expected nfa.endstates.contains(", stdout);
    for( int i = 0, n = SetAny_size(states); i < n; ++i ) {
      int state = SetAny_getByIndexConstT(states,i,int);
      if( i != 0 )
        fputs(",",stdout);
      fprintf(stdout,"%d",state);
    }
    fprintf(stdout, ") -> %s", expected ? "true" : "false");
    return false;
  }
  return true;
}

static bool test_nfa_has_empty_transition(const Nfa *nfa, int from, int to) {
  for( int i  = 0, n = SetAny_size(&nfa->m_emptytransitions); i < n; ++i ) {
    const Transition *t = &SetAny_getByIndexConstT(&nfa->m_emptytransitions,i,Transition);
    if( t->m_from == from && t->m_to == to )
      return true;
  }
  fprintf(stdout, "did not find empty transition %d -> %d", from, to);
  return false;
}

static bool test_nfa_has_transition(const Nfa *nfa, int from, int to, uint32_t s) {
  for( int i  = 0, n = MapAny_size(&nfa->m_transitions); i < n; ++i ) {
    const Transition *t = 0;
    const CharSet *cs = 0;
    MapAny_getByIndexConst(&nfa->m_transitions,i,(const void**)&t,(const void**)&cs);
    if( t->m_from != from || t->m_to != to )
      continue;
    if( CharSet_has(cs,s) )
      return true;
  }
  fprintf(stdout, "did not find transition %d -> %d on %d", from, to, s);
  return false;
}

static bool test_nfa_has_state_token(const Nfa *nfa, int state, int token) {
  if( Nfa_getStateToken(nfa,state) == token )
    return true;
  fprintf(stdout, "did not find token %d on state %d", token, state);
  return false;
}

static bool nfa_init(void *vp) {
  Nfa nfa;
  Scope_Push();
  Nfa_init(&nfa, true);
  TEST_NFA_STATECOUNT(0)
  TEST_NFA_STARTCOUNT(0)
  TEST_NFA_ENDCOUNT(0)
  TEST_NFA_SECTIONS(0)
  Scope_Pop();
  return true;
}

static bool nfa_addstate(void *vp) {
  Nfa nfa;
  Scope_Push();
  Nfa_init(&nfa, true);
  Nfa_addState(&nfa);
  TEST_NFA_STATECOUNT(1)
  TEST_NFA_STARTCOUNT(0)
  TEST_NFA_ENDCOUNT(0)
  TEST_NFA_SECTIONS(0)
  Scope_Pop();
  return true;
}

static bool nfa_statecount(void *vp) {
  Nfa nfa;
  Scope_Push();
  Nfa_init(&nfa, true);
  Nfa_addState(&nfa);
  TEST_NFA_STATECOUNT(1)
  TEST_NFA_NEXTSTATE(Nfa_stateCount(&nfa))
  TEST_NFA_STARTCOUNT(0)
  TEST_NFA_ENDCOUNT(0)
  TEST_NFA_SECTIONS(0)
  Scope_Pop();
  return true;
}

static bool nfa_addstartstate(void *vp) {
  Nfa nfa;
  Scope_Push();
  Nfa_init(&nfa, true);
  Nfa_addState(&nfa);
  Nfa_addState(&nfa);
  Nfa_addStartState(&nfa, Nfa_addState(&nfa));
  TEST_NFA_STATECOUNT(3)
  TEST_NFA_STARTCOUNT(1)
  TEST_NFA_HASSTARTSTATE(2,true)
  TEST_NFA_ENDCOUNT(0)
  TEST_NFA_SECTIONS(0)
  Scope_Pop();
  return true;
}

static bool nfa_addendstate(void *vp) {
  Nfa nfa;
  Scope_Push();
  Nfa_init(&nfa, true);
  Nfa_addState(&nfa);
  Nfa_addState(&nfa);
  Nfa_addEndState(&nfa, Nfa_addState(&nfa));
  TEST_NFA_STATECOUNT(3)
  TEST_NFA_STARTCOUNT(0)
  TEST_NFA_ENDCOUNT(1)
  TEST_NFA_HASENDSTATE(2,true)
  TEST_NFA_SECTIONS(0)
  Scope_Pop();
  return true;
}

static bool nfa_hasendstate(void *vp) {
  Nfa nfa;
  SetAny /*<int>*/ miss;
  SetAny /*<int>*/ hit;
  int missvalues[3] = {97,99,101};
  int hitvalues[3] = {2,99,101};
  Scope_Push();
  Nfa_init(&nfa, true);
  SetAny_init(&hit,getIntElement(),true);
  SetAny_init(&miss,getIntElement(),true);
  SetAny_insertMany(&hit,hitvalues,sizeof(hitvalues)/sizeof(int));
  SetAny_insertMany(&miss,missvalues,sizeof(missvalues)/sizeof(int));
  Nfa_addState(&nfa);
  Nfa_addState(&nfa);
  Nfa_addEndState(&nfa, Nfa_addState(&nfa));
  TEST_NFA_STATECOUNT(3)
  TEST_NFA_STARTCOUNT(0)
  TEST_NFA_ENDCOUNT(1)
  TEST_NFA_HASENDSTATE(2,true)
  TEST_NFA_HASENDSTATE_MANY(&hit,true)
  TEST_NFA_HASENDSTATE_MANY(&miss,false)
  TEST_NFA_SECTIONS(0)
  Scope_Pop();
  return true;
}

static bool nfa_reverse(void *vp) {
  Nfa nfa;
  Scope_Push();
  Nfa_init(&nfa, true);
  int s0 = Nfa_addState(&nfa);
  int s1 = Nfa_addState(&nfa);
  int s2 = Nfa_addState(&nfa);
  int s3 = Nfa_addState(&nfa);
  Nfa_addStartState(&nfa,s0);
  Nfa_addEndState(&nfa, s3);
  Nfa_addEmptyTransition(&nfa,s0,s1);
  Nfa_addTransition(&nfa,s0,s2,'a');
  Nfa_addTransition(&nfa,s1,s3,'b');
  Nfa_addTransition(&nfa,s2,s3,'c');
  Nfa_setStateToken(&nfa,s0,1);
  Nfa_setStateToken(&nfa,s1,2);
  Nfa_setStateToken(&nfa,s2,3);
  Nfa_setStateToken(&nfa,s3,4);
  TEST_NFA_STATECOUNT(4)
  TEST_NFA_STARTCOUNT(1)
  TEST_NFA_ENDCOUNT(1)
  TEST_NFA_HASSTARTSTATE(0,true)
  TEST_NFA_HASENDSTATE(3,true)
  TEST_NFA_HASEMPTYTRANSITION(0,1)
  TEST_NFA_HASTRANSITION(0,2,'a')
  TEST_NFA_HASTRANSITION(1,3,'b')
  TEST_NFA_HASTRANSITION(2,3,'c')
  TEST_NFA_SECTIONS(0)
  TEST_NFA_HASSTATETOKEN(0,1)
  TEST_NFA_HASSTATETOKEN(1,2)
  TEST_NFA_HASSTATETOKEN(2,3)
  TEST_NFA_HASSTATETOKEN(3,4)
  Nfa_Reverse(&nfa,false);
  TEST_NFA_STATECOUNT(4)
  TEST_NFA_STARTCOUNT(1)
  TEST_NFA_ENDCOUNT(1)
  TEST_NFA_HASSTARTSTATE(3,true)
  TEST_NFA_HASENDSTATE(0,true)
  TEST_NFA_HASEMPTYTRANSITION(1,0)
  TEST_NFA_HASTRANSITION(2,0,'a')
  TEST_NFA_HASTRANSITION(3,1,'b')
  TEST_NFA_HASTRANSITION(3,2,'c')
  TEST_NFA_SECTIONS(0)
  TEST_NFA_HASSTATETOKEN(0,1)
  TEST_NFA_HASSTATETOKEN(1,2)
  TEST_NFA_HASSTATETOKEN(2,3)
  TEST_NFA_HASSTATETOKEN(3,4)
  Scope_Pop();
  return true;
}

static bool nfa_reverse_and_renumber(void *vp) {
  Nfa nfa;
  Scope_Push();
  Nfa_init(&nfa, true);
  int s0 = Nfa_addState(&nfa);
  int s1 = Nfa_addState(&nfa);
  int s2 = Nfa_addState(&nfa);
  int s3 = Nfa_addState(&nfa);
  Nfa_addStartState(&nfa,s0);
  Nfa_addEndState(&nfa, s3);
  Nfa_addEmptyTransition(&nfa,s0,s1);
  Nfa_addTransition(&nfa,s0,s2,'a');
  Nfa_addTransition(&nfa,s1,s3,'b');
  Nfa_addTransition(&nfa,s2,s3,'c');
  Nfa_setStateToken(&nfa,s0,1);
  Nfa_setStateToken(&nfa,s1,2);
  Nfa_setStateToken(&nfa,s2,3);
  Nfa_setStateToken(&nfa,s3,4);
  TEST_NFA_STATECOUNT(4)
  TEST_NFA_STARTCOUNT(1)
  TEST_NFA_ENDCOUNT(1)
  TEST_NFA_HASSTARTSTATE(0,true)
  TEST_NFA_HASENDSTATE(3,true)
  TEST_NFA_HASEMPTYTRANSITION(0,1)
  TEST_NFA_HASTRANSITION(0,2,'a')
  TEST_NFA_HASTRANSITION(1,3,'b')
  TEST_NFA_HASTRANSITION(2,3,'c')
  TEST_NFA_SECTIONS(0)
  TEST_NFA_HASSTATETOKEN(0,1)
  TEST_NFA_HASSTATETOKEN(1,2)
  TEST_NFA_HASSTATETOKEN(2,3)
  TEST_NFA_HASSTATETOKEN(3,4)
  Nfa_Reverse(&nfa,true);
  TEST_NFA_STATECOUNT(4)
  TEST_NFA_STARTCOUNT(1)
  TEST_NFA_ENDCOUNT(1)
  TEST_NFA_HASSTARTSTATE(0,true)
  TEST_NFA_HASENDSTATE(3,true)
  TEST_NFA_HASEMPTYTRANSITION(2,3)
  TEST_NFA_HASTRANSITION(1,3,'a')
  TEST_NFA_HASTRANSITION(0,2,'b')
  TEST_NFA_HASTRANSITION(0,1,'c')
  TEST_NFA_SECTIONS(0)
  TEST_NFA_HASSTATETOKEN(3,1)
  TEST_NFA_HASSTATETOKEN(2,2)
  TEST_NFA_HASSTATETOKEN(1,3)
  TEST_NFA_HASSTATETOKEN(0,4)
  Scope_Pop();
  return true;
}


static void Nfa_Swap(Nfa *nfa, Nfa *dfa) {
  Nfa tmp;
  memcpy(&tmp,dfa,sizeof(Nfa));
  memcpy(dfa,nfa,sizeof(Nfa));
  memcpy(nfa,&tmp,sizeof(Nfa));
}

static bool nfa_todfa_nonmin(void *vp) {
  Nfa nfa, dfa;
  Scope_Push();
  Nfa_init(&nfa, true);
  Nfa_init(&dfa, true);
  int s0 = Nfa_addState(&nfa);
  int s1 = Nfa_addState(&nfa);
  int s2 = Nfa_addState(&nfa);
  int s3 = Nfa_addState(&nfa);
  int s4 = Nfa_addState(&nfa);
  int s5 = Nfa_addState(&nfa);
  int s6 = Nfa_addState(&nfa);
  Nfa_addStartState(&nfa,s0);
  Nfa_addEndState(&nfa, s5);
  Nfa_addEndState(&nfa, s6);
  Nfa_addEmptyTransition(&nfa,s0,s1);
  Nfa_addEmptyTransition(&nfa,s0,s2);
  Nfa_addTransition(&nfa,s1,s3,'a');
  Nfa_addTransition(&nfa,s2,s4,'b');
  Nfa_addTransition(&nfa,s3,s5,'c');
  Nfa_addTransition(&nfa,s4,s6,'c');
  Nfa_setStateToken(&nfa,s0,1);
  Nfa_setStateToken(&nfa,s1,2);
  Nfa_setStateToken(&nfa,s2,3);
  Nfa_setStateToken(&nfa,s3,4);
  Nfa_setStateToken(&nfa,s4,5);
  Nfa_setStateToken(&nfa,s5,6);
  Nfa_setStateToken(&nfa,s6,7);
  TEST_NFA_STATECOUNT(7)
  TEST_NFA_STARTCOUNT(1)
  TEST_NFA_ENDCOUNT(2)
  TEST_NFA_HASSTARTSTATE(0,true)
  TEST_NFA_HASENDSTATE(5,true)
  TEST_NFA_HASENDSTATE(6,true)
  TEST_NFA_HASEMPTYTRANSITION(0,1)
  TEST_NFA_HASEMPTYTRANSITION(0,2)
  TEST_NFA_HASTRANSITION(1,3,'a')
  TEST_NFA_HASTRANSITION(2,4,'b')
  TEST_NFA_HASTRANSITION(3,5,'c')
  TEST_NFA_HASTRANSITION(4,6,'c')
  TEST_NFA_SECTIONS(0)
  TEST_NFA_HASSTATETOKEN(0,1)
  TEST_NFA_HASSTATETOKEN(1,2)
  TEST_NFA_HASSTATETOKEN(2,3)
  TEST_NFA_HASSTATETOKEN(3,4)
  TEST_NFA_HASSTATETOKEN(4,5)
  TEST_NFA_HASSTATETOKEN(5,6)
  TEST_NFA_HASSTATETOKEN(6,7)
  Nfa_toDfa_NonMinimal(&nfa,&dfa);
  Nfa_Swap(&nfa,&dfa);
  TEST_NFA_STATECOUNT(5)
  TEST_NFA_STARTCOUNT(1)
  TEST_NFA_ENDCOUNT(2)
  TEST_NFA_HASSTARTSTATE(0,true)
  TEST_NFA_HASENDSTATE(3,true)
  TEST_NFA_HASENDSTATE(4,true)
  TEST_NFA_HASTRANSITION(0,1,'a')
  TEST_NFA_HASTRANSITION(0,2,'b')
  TEST_NFA_HASTRANSITION(1,3,'c')
  TEST_NFA_HASTRANSITION(2,4,'c')
  TEST_NFA_SECTIONS(0)
  TEST_NFA_HASSTATETOKEN(0,1)
  TEST_NFA_HASSTATETOKEN(1,4)
  TEST_NFA_HASSTATETOKEN(2,5)
  TEST_NFA_HASSTATETOKEN(3,6)
  TEST_NFA_HASSTATETOKEN(4,7)
  Scope_Pop();
  return true;
}

static bool nfa_todfa(void *vp) {
  Nfa nfa, dfa;
  Scope_Push();
  Nfa_init(&nfa, true);
  Nfa_init(&dfa, true);
  int s0 = Nfa_addState(&nfa);
  int s1 = Nfa_addState(&nfa);
  int s2 = Nfa_addState(&nfa);
  int s3 = Nfa_addState(&nfa);
  int s4 = Nfa_addState(&nfa);
  int s5 = Nfa_addState(&nfa);
  int s6 = Nfa_addState(&nfa);
  Nfa_addStartState(&nfa,s0);
  Nfa_addEndState(&nfa, s5);
  Nfa_addEndState(&nfa, s6);
  Nfa_addEmptyTransition(&nfa,s0,s1);
  Nfa_addEmptyTransition(&nfa,s0,s2);
  Nfa_addTransition(&nfa,s1,s3,'a');
  Nfa_addTransition(&nfa,s2,s4,'b');
  Nfa_addTransition(&nfa,s3,s5,'c');
  Nfa_addTransition(&nfa,s4,s6,'c');
  Nfa_setStateToken(&nfa,s0,1);
  Nfa_setStateToken(&nfa,s1,2);
  Nfa_setStateToken(&nfa,s2,3);
  Nfa_setStateToken(&nfa,s3,4);
  Nfa_setStateToken(&nfa,s4,5);
  Nfa_setStateToken(&nfa,s5,6);
  Nfa_setStateToken(&nfa,s6,7);
  TEST_NFA_STATECOUNT(7)
  TEST_NFA_STARTCOUNT(1)
  TEST_NFA_ENDCOUNT(2)
  TEST_NFA_HASSTARTSTATE(0,true)
  TEST_NFA_HASENDSTATE(5,true)
  TEST_NFA_HASENDSTATE(6,true)
  TEST_NFA_HASEMPTYTRANSITION(0,1)
  TEST_NFA_HASEMPTYTRANSITION(0,2)
  TEST_NFA_HASTRANSITION(1,3,'a')
  TEST_NFA_HASTRANSITION(2,4,'b')
  TEST_NFA_HASTRANSITION(3,5,'c')
  TEST_NFA_HASTRANSITION(4,6,'c')
  TEST_NFA_SECTIONS(0)
  TEST_NFA_HASSTATETOKEN(0,1)
  TEST_NFA_HASSTATETOKEN(1,2)
  TEST_NFA_HASSTATETOKEN(2,3)
  TEST_NFA_HASSTATETOKEN(3,4)
  TEST_NFA_HASSTATETOKEN(4,5)
  TEST_NFA_HASSTATETOKEN(5,6)
  TEST_NFA_HASSTATETOKEN(6,7)
  Nfa_toDfa(&nfa,&dfa);
  Nfa_Swap(&nfa,&dfa);
  TEST_NFA_STATECOUNT(3)
  TEST_NFA_STARTCOUNT(1)
  TEST_NFA_ENDCOUNT(1)
  TEST_NFA_HASSTARTSTATE(0,true)
  TEST_NFA_HASENDSTATE(2,true)
  TEST_NFA_HASTRANSITION(0,1,'a')
  TEST_NFA_HASTRANSITION(0,1,'b')
  TEST_NFA_HASTRANSITION(1,2,'c')
  TEST_NFA_SECTIONS(0)
  TEST_NFA_HASSTATETOKEN(0,1)
  TEST_NFA_HASSTATETOKEN(1,4)
  TEST_NFA_HASSTATETOKEN(2,6)
  Scope_Pop();
  return true;
}

// using C++ to allow auto registration
static int g_nfa_init = addTest("nfa","init",nfa_init,0);
static int g_nfa_addstate = addTest("nfa","add_state",nfa_addstate,0);
static int g_nfa_statecount = addTest("nfa","state_count",nfa_statecount,0);
static int g_nfa_addstartstate = addTest("nfa","add_start_state",nfa_addstartstate,0);
static int g_nfa_addendstate = addTest("nfa","add_end_state",nfa_addendstate,0);
static int g_nfa_hasendstate = addTest("nfa","has_end_state",nfa_hasendstate,0);
static int g_nfa_reverse = addTest("nfa","reverse",nfa_reverse,0);
static int g_nfa_reverse_and_renumber = addTest("nfa","reverse_and_renumber",nfa_reverse_and_renumber,0);
static int g_nfa_todfa_nonmin = addTest("nfa","to_dfa_non_min",nfa_todfa_nonmin,0);
static int g_nfa_todfa = addTest("nfa","to_dfa",nfa_todfa,0);
