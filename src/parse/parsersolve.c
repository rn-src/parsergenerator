#include "parser.h"
#include "parsertokL.h"
#include <stdint.h>

#define SHIFT (1)
#define REDUCE (2)
#define REDUCE2 (4)

extern void *zalloc(size_t len);

typedef uint64_t ticks_t;
ticks_t getSystemTicks();
ticks_t getSystemTicksFreq();

void StringToInt_2_IntToString(const MapAny /*<String,int>*/ *src, MapAny /*<int,String>*/ *tokens);
extern ElementOps ProductionAndRestrictStateElement;
extern ElementOps StateElement;
struct StringAndIntSet;
typedef struct StringAndIntSet StringAndIntSet;

struct StringAndIntSet {
  String str;
  SetAny set;
};

void StringAndIntSet_destroy(StringAndIntSet *This);

void StringAndIntSet_init(StringAndIntSet *This, bool onstack) {
  String_init(&This->str,false);
  SetAny_init(&This->set, getIntElement(),false);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)StringAndIntSet_destroy);
}

void StringAndIntSet_destroy(StringAndIntSet *This) {
  String_clear(&This->str);
  SetAny_destroy(&This->set);
}

void StringAndIntSet_clear(StringAndIntSet *This) {
  String_clear(&This->str);
  SetAny_clear(&This->set);
}

bool StringAndIntSet_LessThan(const StringAndIntSet *lhs, const StringAndIntSet *rhs) {
  if( String_LessThan(&lhs->str,&rhs->str) )
    return true;
  if( String_LessThan(&rhs->str,&lhs->str) )
    return false;
  if( SetAny_LessThan(&lhs->set,&rhs->set) )
    return true;
  return false;
}

bool StringAndIntSet_Equal(const StringAndIntSet *lhs, const StringAndIntSet *rhs) {
  return String_Equal(&lhs->str,&rhs->str) && SetAny_Equal(&lhs->set,&rhs->set);
}

void StringAndIntSet_Assign(StringAndIntSet *lhs, const StringAndIntSet *rhs) {
  String_AssignString(&lhs->str,&rhs->str);
  SetAny_Assign(&lhs->set,&rhs->set);
}

ElementOps StringAndIntSetElement = {sizeof(StringAndIntSet), false, false, (elementInit)StringAndIntSet_init, (elementDestroy)StringAndIntSet_destroy, (elementLessThan)StringAndIntSet_LessThan, (elementEqual)StringAndIntSet_Equal, (elementAssign)StringAndIntSet_Assign, 0};
extern ElementOps ProductionStateElement;

struct FirstAndFollowState;
typedef struct FirstAndFollowState FirstAndFollowState;

struct FirstAndFollowState {
  ParserDef *parser;
  FILE *out;
  int verbosity;
  FirstsAndFollows *firstsAndFollows;
  SetAny /*<productionandrestrictstate_t>*/ visited;
  SetAny /*<int>*/ emptyintset;
  SetAny /*<int>*/ adding;
};

void FirstAndFollowState_destroy(FirstAndFollowState *This) {
  SetAny_destroy(&This->visited);
  SetAny_destroy(&This->emptyintset);
  SetAny_destroy(&This->adding);
}

void FirstAndFollowState_init(FirstAndFollowState *This, FirstsAndFollows *_firstsAndFollows, ParserDef *_parser, FILE *_out, int _verbosity, bool onstack) {
  This->firstsAndFollows = _firstsAndFollows;
  This->parser = _parser;
  This->out = _out;
  This->verbosity = _verbosity;
  SetAny_init(&This->visited, &ProductionAndRestrictStateElement, false);
  SetAny_init(&This->emptyintset, getIntElement(), false);
  SetAny_init(&This->adding, getIntElement(), false);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)FirstAndFollowState_destroy);
}

static void FirstAndFollowState_logAdded(FirstAndFollowState *This, const char *to, int symbol, int pid, int restrictState) {
  const char *symbolstr = (symbol == EOF_TOK) ? "$" : String_Chars(&MapAny_findConstT(&This->parser->m_tokdefs, &symbol, SymbolDef).m_name);
  const char *productionstr = String_Chars(&MapAny_findConstT(&This->parser->m_tokdefs, &pid, SymbolDef).m_name);
  fprintf(This->out, "Adding %s to %s(%s,%d)\n", symbolstr, to, productionstr, restrictState);
}

static  bool FirstAndFollowState_ComputeFirsts_productionandrestrictstate(FirstAndFollowState *This, const productionandrestrictstate_t *prs) {
  bool added = false;
  const Production *production = prs->production;
  int restrictState = prs->restrictstate;
  bool previouslyVisited = SetAny_contains(&This->visited, prs);

  if( ! previouslyVisited )
    SetAny_insert(&This->visited, prs, 0);

  // compute the firsts of the sub-productions, but avoid infinite recursion
  if( ! previouslyVisited ) {
    for (int placementdot = 0, placementend = VectorAny_size(&production->m_symbols); placementdot < placementend; ++placementdot) {
      int symbol = VectorAny_ArrayOpConstT(&production->m_symbols, placementdot, int);
      SymbolType stype = ParserDef_getSymbolType(This->parser, symbol);
      if (stype == SymbolTypeTerminal)
        continue;
      VectorAny subproductions;
      Scope_Push();
      VectorAny_init(&subproductions, 0, true);
      VectorAny_Assign(&subproductions, ParserDef_productionsAt(This->parser, production, placementdot, restrictState));
      for (int pidx = 0, endpidx = VectorAny_size(&subproductions); pidx < endpidx; ++pidx) {
        const productionandrestrictstate_t *subprs = &VectorAny_ArrayOpConstT(&subproductions, pidx, productionandrestrictstate_t);
        added = FirstAndFollowState_ComputeFirsts_productionandrestrictstate(This, subprs) || added;
      }
      Scope_Pop();
    }
  }

  // firsts are assigned until the first non-epsilon carrying placement is found
  bool assignFirsts = true;

  for( int placementdot = 0, placementend = VectorAny_size(&production->m_symbols); assignFirsts && placementdot < placementend; ++placementdot) {
    bool hasEpsilon = false;
    int symbol = VectorAny_ArrayOpConstT(&production->m_symbols, placementdot, int);
    SymbolType stype = ParserDef_getSymbolType(This->parser,symbol);
    if( stype == SymbolTypeTerminal ) {
      if( ! MapAny_contains(&This->firstsAndFollows->m_firsts,prs) )
        MapAny_insert(&This->firstsAndFollows->m_firsts, prs, &This->emptyintset);
      SetAny /*<int>*/ *firsts = &MapAny_findT(&This->firstsAndFollows->m_firsts, prs, SetAny);
      int before = SetAny_size(firsts);
      SetAny_insert(firsts,&symbol,0);
      int after = SetAny_size(firsts);
      if( after > before ) {
        added = true;
        if (This->verbosity > 2)
          FirstAndFollowState_logAdded(This, "FIRST", symbol, production->m_pid, restrictState);
      }
    } else {
      bool foundEmptyProduction = false;
      const VectorAny /*<productionandrestrictstate_t>*/ *subproductions = ParserDef_productionsAt(This->parser, production, placementdot, restrictState);
      for( int pidx = 0, endpidx = VectorAny_size(subproductions); pidx < endpidx; ++pidx ) {
        const productionandrestrictstate_t *subprs = &VectorAny_ArrayOpConstT(subproductions,pidx,productionandrestrictstate_t);

        // Watch for empty productions, which introduces an epsilon
        if( VectorAny_size(&subprs->production->m_symbols) == 0 )
          foundEmptyProduction = true;

        // Add to our own firsts if called for
        SetAny /*<int>*/ *subfirsts = &MapAny_findT(&This->firstsAndFollows->m_firsts, subprs, SetAny);
        if( subfirsts ) {
          if( ! MapAny_contains(&This->firstsAndFollows->m_firsts, prs) )
            MapAny_insert(&This->firstsAndFollows->m_firsts, prs, &This->emptyintset);
          SetAny /*<int>*/ *firsts = &MapAny_findT(&This->firstsAndFollows->m_firsts, prs, SetAny);
          subfirsts = &MapAny_findT(&This->firstsAndFollows->m_firsts, subprs, SetAny);
          if (This->verbosity > 2) {
            SetAny_Assign(&This->adding, subfirsts);
            SetAny_eraseMany(&This->adding, SetAny_ptrConst(firsts), SetAny_size(firsts));
            for (int addidx = 0, addend = SetAny_size(&This->adding); addidx < addend; ++addidx) {
              int addsymbol = SetAny_getByIndexConstT(&This->adding,addidx,int);
              FirstAndFollowState_logAdded(This, "FIRST", addsymbol, production->m_pid, restrictState);
            }
          }
          int before = SetAny_size(firsts);
          SetAny_insertMany(firsts, SetAny_ptr(subfirsts), SetAny_size(subfirsts));
          int after = SetAny_size(firsts);
          if (after > before)
            added = true;
        }
      }
      if( foundEmptyProduction )
        hasEpsilon = true;
    }
    if( ! hasEpsilon )
      assignFirsts = false;
  }

  return added;
}

void FirstAndFollowState_ComputeFirsts(FirstAndFollowState *This) {
  productionandrestrictstate_t prs;
  volatile bool runAgain = true;

  MapAny_clear(&This->firstsAndFollows->m_firsts);

  prs.production = This->parser->m_startProduction;
  prs.restrictstate = 0;

  while( runAgain ) {
    SetAny_clear(&This->visited);
    runAgain = FirstAndFollowState_ComputeFirsts_productionandrestrictstate(This, &prs);
  }
}

static  bool FirstAndFollowState_ComputeFollows_productionandrestrictstate(FirstAndFollowState *This, const productionandrestrictstate_t *prs) {
  bool added = false;
  const Production *production = prs->production;
  int restrictState = prs->restrictstate;
  bool previouslyVisited = SetAny_contains(&This->visited, prs);

  if( ! previouslyVisited )
    SetAny_insert(&This->visited, prs, 0);

  // Add follows to sub-productions
  for (int placementdot = 0, placementend = VectorAny_size(&production->m_symbols); placementdot < placementend; ++placementdot) {
    int symbol = VectorAny_ArrayOpConstT(&production->m_symbols, placementdot, int);
    SymbolType stype = ParserDef_getSymbolType(This->parser, symbol);
    if (stype == SymbolTypeTerminal)
      continue; // not sub-productions
    const VectorAny /*<productionandrestrictstate_t>*/ *subproductions = ParserDef_productionsAt(This->parser, production, placementdot, restrictState);
    for (int pidx = 0, endpidx = VectorAny_size(subproductions); pidx < endpidx; ++pidx) {
      const productionandrestrictstate_t *subprs = &VectorAny_ArrayOpConstT(subproductions, pidx, productionandrestrictstate_t);
      const Production *subProduction = subprs->production;
      int subRestrictState = subprs->restrictstate;
      if (!MapAny_contains(&This->firstsAndFollows->m_follows, subprs))
        MapAny_insert(&This->firstsAndFollows->m_follows, subprs, &This->emptyintset);
      SetAny /*<int>*/ *subfollows = &MapAny_findT(&This->firstsAndFollows->m_follows, subprs, SetAny);
      if( placementdot+1 < placementend) {
        // Use the firsts from the next symbol in the production
        int nextsymbol = VectorAny_ArrayOpConstT(&production->m_symbols, placementdot+1, int);
        SymbolType nextstype = ParserDef_getSymbolType(This->parser, nextsymbol);
        // Easy case, next symbol is a terminal
        if(nextstype == SymbolTypeTerminal) {
          int before = SetAny_size(subfollows);
          SetAny_insert(subfollows,&nextsymbol,0);
          int after = SetAny_size(subfollows);
          if( after > before ) {
            added = true;
            if (This->verbosity > 2)
              FirstAndFollowState_logAdded(This,"FOLLOW", nextsymbol, subProduction->m_pid,restrictState);
          }
        } else {
          // Next symbol is a nonterminal, use the union of firsts from the sub-productions
          const VectorAny /*<productionandrestrictstate_t>*/ *subnextproductions = ParserDef_productionsAt(This->parser, production, placementdot+1, restrictState);
          for (int nextpidx = 0, nextendpidx = VectorAny_size(subnextproductions); nextpidx < nextendpidx; ++nextpidx) {
            const productionandrestrictstate_t *subnextprs = &VectorAny_ArrayOpConstT(subnextproductions, nextpidx, productionandrestrictstate_t);
            SetAny /*<int>*/ *nextfirsts = &MapAny_findT(&This->firstsAndFollows->m_firsts, subnextprs, SetAny);
            if (This->verbosity > 2) {
              SetAny_Assign(&This->adding, nextfirsts);
              SetAny_eraseMany(&This->adding, SetAny_ptrConst(subfollows), SetAny_size(subfollows));
              for (int addidx = 0, addend = SetAny_size(&This->adding); addidx < addend; ++addidx) {
                int addsymbol = SetAny_getByIndexConstT(&This->adding, addidx, int);
                FirstAndFollowState_logAdded(This, "FOLLOW", addsymbol, subProduction->m_pid, subRestrictState);
              }
            }
            int before = SetAny_size(subfollows);
            SetAny_insertMany(subfollows, SetAny_ptr(nextfirsts), SetAny_size(nextfirsts));
            int after = SetAny_size(subfollows);
            if (after > before)
              added = true;
          }
        }
      } else {
        // At the end, add our own follows to the sub-production
        SetAny /*<int>*/ *follows = &MapAny_findT(&This->firstsAndFollows->m_follows, prs, SetAny);
        if (This->verbosity > 2) {
          SetAny_Assign(&This->adding, follows);
          SetAny_eraseMany(&This->adding, SetAny_ptrConst(subfollows), SetAny_size(subfollows));
          for (int addidx = 0, addend = SetAny_size(&This->adding); addidx < addend; ++addidx) {
            int addsymbol = SetAny_getByIndexConstT(&This->adding, addidx, int);
            FirstAndFollowState_logAdded(This, "FOLLOW", addsymbol, subProduction->m_pid, subRestrictState);
          }
        }
        SetAny_insertMany(subfollows, SetAny_ptr(follows), SetAny_size(follows));
      }
    }
  }

  // Repeat for sub-productions
  if( ! previouslyVisited ) {
    for (int placementdot = 0, placementend = VectorAny_size(&production->m_symbols); placementdot < placementend; ++placementdot) {
      int symbol = VectorAny_ArrayOpConstT(&production->m_symbols, placementdot, int);
      SymbolType stype = ParserDef_getSymbolType(This->parser, symbol);
      if (stype == SymbolTypeTerminal)
        continue; // not sub-productions
      const VectorAny /*<productionandrestrictstate_t>*/ *subproductions = ParserDef_productionsAt(This->parser, production, placementdot, restrictState);
      for (int pidx = 0, endpidx = VectorAny_size(subproductions); pidx < endpidx; ++pidx) {
        const productionandrestrictstate_t *subprs = &VectorAny_ArrayOpConstT(subproductions, pidx, productionandrestrictstate_t);
        added = FirstAndFollowState_ComputeFollows_productionandrestrictstate(This, subprs) || added;
      }
    }
  }

  return added;
}

void FirstAndFollowState_ComputeFollows(FirstAndFollowState *This) {
  productionandrestrictstate_t prs;
  volatile bool runAgain = true;
  int eoftok = EOF_TOK;

  MapAny_clear(&This->firstsAndFollows->m_follows);

  if (This->verbosity > 2)
    FirstAndFollowState_logAdded(This, "FOLLOW", EOF_TOK, This->parser->m_startProduction->m_pid, 0);
  prs.production = This->parser->m_startProduction;
  prs.restrictstate = 0;
  MapAny_insert(&This->firstsAndFollows->m_follows, &prs, &This->emptyintset);
  SetAny_insert(&MapAny_findT(&This->firstsAndFollows->m_follows, &prs, SetAny), &eoftok, 0);

  while (runAgain) {
    SetAny_clear(&This->visited);
    runAgain = FirstAndFollowState_ComputeFollows_productionandrestrictstate(This, &prs);
  }
}

void FirstAndFollowState_getFollows(FirstAndFollowState *This, const Production *p, int pos, int restrictstate, VectorAny /*< Pair< String,Set<int> > >*/ *followslist) {
  Scope_Push();
  StringAndIntSet followitem;
  StringAndIntSet_init(&followitem,true);

  VectorAny_clear(followslist);
  productionandrestrictstate_t prs;
  if( pos+1== VectorAny_size(&p->m_symbols) ) {
    StringAndIntSet_clear(&followitem);
    String_ReFormatString(&followitem.str,"FOLLOW(%s,%d)", String_Chars(&MapAny_findConstT(&This->parser->m_tokdefs,&p->m_pid,SymbolDef).m_name), restrictstate);
    const SetAny *pidfollows = &MapAny_findConstT(&This->firstsAndFollows->m_follows,productionandrestrictstate_t_set(&prs,p,restrictstate),SetAny);
    if( pidfollows )
      SetAny_Assign(&followitem.set,pidfollows);
    VectorAny_push_back(followslist,&followitem);
  } else {
    int nexttok = VectorAny_ArrayOpConstT(&p->m_symbols,pos+1,int);
    SymbolType stypenext = ParserDef_getSymbolType(This->parser,nexttok);
    if( stypenext == SymbolTypeTerminal ) {
      StringAndIntSet_clear(&followitem);
      String_ReFormatString(&followitem.str, "FIRST(%s,%d)", String_Chars(&MapAny_findConstT(&This->parser->m_tokdefs,&nexttok,SymbolDef).m_name), restrictstate);
      SetAny_insert(&followitem.set,&nexttok,0);
      VectorAny_push_back(followslist,&followitem);
    } else if( stypenext == SymbolTypeNonterminal ) {
      const VectorAny /*<productionandrestrictstate_t>*/ *productions = ParserDef_productionsAt(This->parser,p,pos+1,restrictstate);
      for( int i = 0, n = VectorAny_size(productions); i != n; ++i ) {
        const productionandrestrictstate_t *curpat = &VectorAny_ArrayOpConstT(productions,i,productionandrestrictstate_t);
        StringAndIntSet_clear(&followitem);
        String_ReFormatString(&followitem.str,"FIRST(%s,%d)", String_Chars(&MapAny_findConstT(&This->parser->m_tokdefs,&curpat->production->m_pid,SymbolDef).m_name), curpat->restrictstate);
        const SetAny *pidfirsts = &MapAny_findConstT(&This->firstsAndFollows->m_firsts, productionandrestrictstate_t_set(&prs,curpat->production,curpat->restrictstate),SetAny);
        if( pidfirsts )
          SetAny_Assign(&followitem.set,pidfirsts);
        VectorAny_push_back(followslist,&followitem);
      }
    }
  }
  Scope_Pop();
}

void State_closure(state_t *state, ParserDef *parser) {
  int prevsize = 0;
  SetAny /*<ProductionState>*/ newparts;
  Scope_Push();
  SetAny_init(&newparts,&ProductionStateElement,true);
  while( state_t_size(state) > prevsize ) {
    prevsize = state_t_size(state);
    SetAny_clear(&newparts);
     for( int cur = 0, end = state_t_size(state); cur != end; ++cur ) {
      const ProductionState *curState = &state_t_getByIndexConst(state,cur);
      const VectorAny /*<productionandrestrictstate_t>*/ *productions = ParserDef_productionsAt(parser,curState->m_p,curState->m_pos,curState->m_restrictstate);
      for( int curp = 0, endp = VectorAny_size(productions); curp != endp; ++curp ) {
        const productionandrestrictstate_t *curProductionAndRestrictState = &VectorAny_ArrayOpConstT(productions,curp,productionandrestrictstate_t);
        ProductionState ps;
        ProductionState_set(&ps,curProductionAndRestrictState->production,0,curProductionAndRestrictState->restrictstate);
        SetAny_insert(&newparts,&ps,0);
      }
    }
    for( int curpart = 0, endpart = SetAny_size(&newparts); curpart != endpart; ++curpart ) {
      const ProductionState *curProductionState = &SetAny_getByIndexConstT(&newparts,curpart,ProductionState);
      state_t_insert(state,curProductionState,0);
    }
  }
  Scope_Pop();
}

void State_nexts(const state_t *state, ParserDef *parser, SetAny /*<int>*/ *nextSymbols) {
  for( int curs = 0, ends = state_t_size(state); curs != ends; ++curs ) {
    const ProductionState *curState = &state_t_getByIndexConst(state,curs);
    int symbol = ProductionState_symbol(curState);
    if( symbol == -1 )
      continue;
    if( ParserDef_getSymbolType(parser,symbol) == SymbolTypeTerminal )
      SetAny_insert(nextSymbols,&symbol,0);
    else {
      const VectorAny /*<productionandrestrictstate_t>*/ *productions = ParserDef_productionsAt(parser,curState->m_p,curState->m_pos,curState->m_restrictstate);
      for( int curp = 0, endp = VectorAny_size(productions); curp != endp; ++curp ) {
        const productionandrestrictstate_t *curProductionAndRestrictState = &VectorAny_ArrayOpConstT(productions,curp,productionandrestrictstate_t);
        SetAny_insert(nextSymbols,&curProductionAndRestrictState->production->m_pid,0);
      }
    }
  }
}

void State_advance(state_t *state, int tsymbol, ParserDef *parser, state_t *nextState) {
  state_t_clear(nextState);
  const Production *p = 0;
  ProductionState ps;
  if( ParserDef_getSymbolType(parser,tsymbol) == SymbolTypeNonterminalProduction )
    p = MapAny_findConstT(&parser->m_tokdefs,&tsymbol,SymbolDef).m_p;
  for( int curs = 0, ends = state_t_size(state); curs != ends; ++curs ) {
    const ProductionState *curState = &state_t_getByIndexConst(state,curs);
    int symbol = ProductionState_symbol(curState);
    if( symbol == tsymbol || (p && p->m_nt == symbol && ! RestrictAutomata_isRestricted(&parser->m_restrict,curState->m_p,curState->m_pos,curState->m_restrictstate,p)) )
      state_t_insert(nextState,ProductionState_set(&ps,curState->m_p,curState->m_pos+1,curState->m_restrictstate),0);
  }
}

void LR_ComputeStatesAndActions(ParserDef *parser, LRParserSolution *solution, FILE *out, int verbosity) {
  MapAny /*<state_t,int>*/ statemap;
  state_t state, nextState;
  SetAny /*<int>*/ nextSymbols;
  SetAny /*<int>*/ emptyintset;
  ProductionState ps;
  productionandrestrictstate_t prs;
  shifttosymbols_t emptyshifttosymbols;
  reducebysymbols_t emptyreducebysymbols;


  Scope_Push();
  MapAny_init(&statemap,&StateElement,getIntElement(),true);
  state_t_init(&state,true);
  state_t_init(&nextState,true);
  SetAny_init(&nextSymbols,getIntElement(),true);
  SetAny_init(&emptyintset, getIntElement(), true);
  MapAny_init(&emptyshifttosymbols, getIntElement(), getSetAnyElement(), true);
  MapAny_init(&emptyreducebysymbols, getPointerElement(), getSetAnyElement(), true);

  state_t_insert(&state,ProductionState_set(&ps,ParserDef_getStartProductionConst(parser),0,0),0);
  State_closure(&state,parser);
  int n = VectorAny_size(&solution->m_states);
  MapAny_insert(&statemap,&state,&n);
  VectorAny_push_back(&solution->m_states,&state);
  for( int i = 0; i < VectorAny_size(&solution->m_states); ++i ) {
    if( verbosity > 2 )
      fprintf(out,"computing state %d\n",i);
    state_t_Assign(&state,&VectorAny_ArrayOpConstT(&solution->m_states,i,state_t));
    int stateIdx = i;
    State_nexts(&state,parser,&nextSymbols);
    for( int cur = 0, end = SetAny_size(&nextSymbols); cur != end; ++cur ) {
      int symbol = SetAny_getByIndexConstT(&nextSymbols,cur,int);
      State_advance(&state,symbol,parser,&nextState);
      State_closure(&nextState,parser);
      if( state_t_empty(&nextState) )
        continue;
      int nextStateIdx = -1;
      const int *stateiter = &MapAny_findConstT(&statemap,&nextState,int);
      if( ! stateiter ) {
        nextStateIdx = VectorAny_size(&solution->m_states);
        MapAny_insert(&statemap,&nextState,&nextStateIdx);
        VectorAny_push_back(&solution->m_states,&nextState);
        if( verbosity > 2 )
          fprintf(out,"added state %d\n",nextStateIdx);
      } else {
        nextStateIdx = *stateiter;
      }
      shifttosymbols_t *shifttosymbols = &MapAny_findT(&solution->m_shifts,&stateIdx,shifttosymbols_t);
      if (!shifttosymbols) {
        MapAny_insert(&solution->m_shifts, &stateIdx, &emptyshifttosymbols);
        shifttosymbols = &MapAny_findT(&solution->m_shifts, &stateIdx, shifttosymbols_t);
      }
      SetAny *stateShiftTos = &MapAny_findT(shifttosymbols,&nextStateIdx,SetAny);
      if (!stateShiftTos) {
        MapAny_insert(shifttosymbols,&nextStateIdx,&emptyintset);
        stateShiftTos = &MapAny_findT(shifttosymbols, &nextStateIdx, SetAny);
      }
      SetAny_insert(stateShiftTos,&symbol,0);
    }
    for( int curps = 0, endps = state_t_size(&state); curps != endps; ++curps ) {
      const ProductionState *curProductionState = &state_t_getByIndexConst(&state,curps);
      if( curProductionState->m_pos < VectorAny_size(&curProductionState->m_p->m_symbols) )
        continue;
      const SetAny /*<int>*/ *follows = &MapAny_findConstT(&solution->m_firstsAndFollows.m_follows,productionandrestrictstate_t_set(&prs,curProductionState->m_p,curProductionState->m_restrictstate),SetAny);
      if (!follows)
        continue;
      reducebysymbols_t *reducebysymbols = &MapAny_findT(&solution->m_reductions,&stateIdx,reducebysymbols_t);
      if (!reducebysymbols) {
        MapAny_insert(&solution->m_reductions, &stateIdx, &emptyreducebysymbols);
        reducebysymbols = &MapAny_findT(&solution->m_reductions, &stateIdx, reducebysymbols_t);
      }
      SetAny *productionFollows = &MapAny_findT(reducebysymbols,&curProductionState->m_p,SetAny);
      if (!productionFollows) {
        MapAny_insert(reducebysymbols, &curProductionState->m_p, &emptyintset);
        productionFollows = &MapAny_findT(reducebysymbols, &curProductionState->m_p, SetAny);
      }
      SetAny_insertMany(productionFollows,SetAny_ptrConst(follows),SetAny_size(follows));
    }
  }
  Scope_Pop();
}

void PrintRules(const ParserDef *parser, FILE *out) {
  int i = 1;
  for( int cur = 0, end = VectorAny_size(&parser->m_productions); cur != end; ++cur ) {
    const Production *curP = VectorAny_ArrayOpConstT(&parser->m_productions,cur,Production*);
    fprintf(out, "#%d ", i++);
    Production_print(curP,out,&parser->m_tokdefs,-1,0);
    fputs("\n",out);
  }
  fputs("\n",out);
}

void LR_PrintStatesAndActions(const ParserDef *parser, const LRParserSolution *solution, FILE *out) {
  SetAny /*<int>*/ symbols;
  SetAny /*<int>*/ shiftsymbols;
  SetAny /*<Production*>*/ emptyproductionset;
  MapAny /*<int,Set<const Production*> > */ reductions;
  VectorAny /*<Production*>*/ reduces;
  VectorAny /*<const Production*>*/ symreductions;

  Scope_Push();
  SetAny_init(&symbols,getIntElement(),true);
  SetAny_init(&shiftsymbols,getIntElement(),true);
  SetAny_init(&emptyproductionset, getPointerElement(), true);
  MapAny_init(&reductions,getIntElement(),getSetAnyElement(),true);
  VectorAny_init(&reduces,getPointerElement(),true);
  VectorAny_init(&symreductions,getPointerElement(),true);

  const MapAny /*<int,SymbolDef>*/ *tokens = &parser->m_tokdefs;
  fprintf(out, "%d states\n\n", VectorAny_size(&solution->m_states));
  for( int i = 0, n = VectorAny_size(&solution->m_states); i != n; ++i ) {
    SetAny_clear(&symbols);
    SetAny_clear(&shiftsymbols);
    MapAny_clear(&reductions);

    fprintf(out, "State %d:\n", i);
    const state_t *state = &VectorAny_ArrayOpConstT(&solution->m_states,i,state_t);
    state_t_print(state,out,tokens);
    const shifttosymbols_t *shifttosymbols = &MapAny_findConstT(&solution->m_shifts,&i,shifttosymbols_t);
    if( shifttosymbols ) {
      for( int curshift = 0, endshift = MapAny_size(shifttosymbols); curshift != endshift; ++curshift ) {
        const int *tostate = 0;
        const SetAny /*<int>*/ *onsymbols = 0;
        MapAny_getByIndexConst(shifttosymbols,curshift,(const void**)&tostate,(const void**)&onsymbols);
        fprintf(out, "shift to state %d on ", *tostate);
        bool bFirst = true;
        for( int cursym = 0, endsym = SetAny_size(onsymbols); cursym != endsym; ++cursym ) {
          int onsymbol = SetAny_getByIndexConstT(onsymbols,cursym,int);
          if( bFirst )
            bFirst = false;
          else
            fputs(", ", out);
          fputs(String_Chars(&MapAny_findConstT(tokens,&onsymbol,SymbolDef).m_name), out);
          SetAny_insert(&shiftsymbols,&onsymbol,0);
          SetAny_insert(&symbols,&onsymbol,0);
        }
        fputs("\n",out);
      }
    }
    //typedef MapAny /*<int,reducebysymbols_t>*/ reducemap_t;
    //int *reduceby reducemap_t::const_iterator reduceiter = 
    const reducebysymbols_t *reducebysymbols = &MapAny_findConstT(&solution->m_reductions,&i,reducebysymbols_t);
    if( reducebysymbols ) {
      VectorAny_clear(&reduces);
      for( int curreduce = 0, endreduce = MapAny_size(reducebysymbols); curreduce != endreduce; ++curreduce ) {
        const Production *production = 0;
        const SetAny /*<int>*/ *reduceby;
        MapAny_getByIndexConst(reducebysymbols,curreduce,(const void**)&production,(const void**)&reduceby);
        VectorAny_push_back(&reduces,production);
      }
      if( VectorAny_size(&reduces) > 1 )
        qsort(VectorAny_ptr(&reduces),VectorAny_size(&reduces),sizeof(Production*),Production_CompareId);
      for( int curp = 0, endp = VectorAny_size(&reduces); curp != endp; ++curp ) {
        Production *p = VectorAny_ArrayOpT(&reduces,curp,Production*);
        const SetAny /*<int>*/ *syms = &MapAny_findConstT(reducebysymbols,&p,SetAny);
        if( p->m_rejectable )
          fputs("(rejectable) ", out);
        fputs("reduce by production [", out);
        Production_print(p,out,tokens,-1,0);
        fputs("] on ", out);
        bool bFirst = true;
        for( int cursym = 0, endsym = SetAny_size(syms); cursym != endsym; ++cursym ) {
          int curSymbol = SetAny_getByIndexConstT(syms,cursym,int);
          const char *symstr = (curSymbol==-1) ? "$" : String_Chars(&MapAny_findConstT(tokens,&curSymbol,SymbolDef).m_name);
          if( bFirst )
            bFirst = false;
          else
            fputs(", ", out);
          fputs(symstr, out);
          SetAny *reductionsymbols = &MapAny_findT(&reductions, &curSymbol, SetAny);
          if (!reductionsymbols) {
            MapAny_insert(&reductions, &curSymbol, &emptyproductionset);
            reductionsymbols = &MapAny_findT(&reductions, &curSymbol, SetAny);
          }
          SetAny_insert(reductionsymbols,&p,0);
          SetAny_insert(&symbols,&curSymbol,0);
        }
        fputs("\n",out);
      }
    }
    for( int cursym = 0, endsym = SetAny_size(&symbols); cursym != endsym; ++cursym ) {
      int sym = SetAny_getByIndexConstT(&symbols,cursym,int);
      if( ! MapAny_contains(&reductions,&sym) )
        continue; // there can be no conflicts if there are no reductions
      const char *symstr = (sym==-1) ? "$" : String_Chars(&MapAny_findConstT(tokens,&sym,SymbolDef).m_name);
      if( SetAny_contains(&shiftsymbols,&sym) )
        fprintf(out, "shift/reduce conflict on %s\n", symstr);
      VectorAny_clear(&symreductions); /*<const Production*>*/
      // MapAny /*<int,Set<const Production*> > */ reductions;
      const SetAny /*<const Production*>*/ *reduceProductions = &MapAny_findConstT(&reductions,&sym,SetAny);
      VectorAny_insertMany(&symreductions,VectorAny_size(&symreductions),SetAny_ptrConst(reduceProductions),SetAny_size(reduceProductions));
      for( int r0 = 0; r0 < VectorAny_size(&symreductions); ++r0 )
        for( int r1 = r0+1; r1 < VectorAny_size(&symreductions); ++r1 )
          if( ! VectorAny_ArrayOpConstT(&symreductions,r0,Production*)->m_rejectable && ! VectorAny_ArrayOpConstT(&symreductions,r1,Production*)->m_rejectable )
            fprintf(out, "reduce/reduce conflict on %s\n", symstr);
    }
    fputs("\n",out);
  }
  fputs("\n",out);
  Scope_Pop();
}

int LR_PrintConflicts(ParserDef *parser, LRParserSolution *solution, FILE *out) {
  SetAny /*<int>*/ symbols;
  SetAny /*<int>*/ shiftsymbols;
  SetAny /*<Production*>*/ emptyproductionset;
  MapAny /*<int,Set<const Production*> >*/ reductions;
  VectorAny /*<const Production*>*/ symreductions;

  Scope_Push();
  SetAny_init(&symbols,getIntElement(),true);
  SetAny_init(&shiftsymbols,getIntElement(),true);
  SetAny_init(&emptyproductionset, getPointerElement(), true);
  MapAny_init(&reductions,getIntElement(),getSetAnyElement(),true);
  VectorAny_init(&symreductions,getPointerElement(),true);

  int nconflicts = 0;
  const MapAny /*<int,SymbolDef>*/ *tokens = &parser->m_tokdefs;
  for( int i = 0, n = VectorAny_size(&solution->m_states); i != n; ++i ) {
    SetAny_clear(&symbols);
    SetAny_clear(&shiftsymbols);
    MapAny_clear(&reductions);
    shifttosymbols_t *shifttosymbols = &MapAny_findT(&solution->m_shifts,&i,shifttosymbols_t);
    if( shifttosymbols ) {
      for( int curshift = 0, endshift = MapAny_size(shifttosymbols); curshift != endshift; ++curshift ) {
        const int *shiftto = 0;
        SetAny /*<int>*/ *shiftsymbols = 0;
        MapAny_getByIndex(shifttosymbols,curshift,(const void**)&shiftto,(void**)&shiftsymbols);
        for( int cursym = 0, endsym = SetAny_size(shiftsymbols); cursym != endsym; ++cursym ) {
          int sym = SetAny_getByIndexConstT(shiftsymbols,cursym,int);
          SetAny_insert(shiftsymbols,&sym,0);
          SetAny_insert(&symbols,&sym,0);
        }
      }
    }
    const reducebysymbols_t *reducebysymbols = &MapAny_findConstT(&solution->m_reductions,&i,reducebysymbols_t);
    if( reducebysymbols ) {
      for( int curreduce = 0, endreduce = MapAny_size(reducebysymbols); curreduce != endreduce; ++curreduce ) {
        const Production *production = 0;
        const SetAny /*<int>*/ *reducebys = 0;
        MapAny_getByIndexConst(reducebysymbols,curreduce,(const void**)&production,(const void**)&reducebys);
        for( int cursym = 0, endsym = SetAny_size(reducebys); cursym != endsym; ++cursym ) {
          int sym = SetAny_getByIndexConstT(reducebys,cursym,int);
          SetAny *reductionproductions = &MapAny_findT(&reductions, &sym, SetAny);
          if (!reductionproductions) {
            MapAny_insert(&reductions, &sym, &emptyproductionset);
            reductionproductions = &MapAny_findT(&reductions, &sym, SetAny);
          }
          SetAny_insert(reductionproductions,&production,0);
          SetAny_insert(&symbols,&sym,0);
        }
      }
    }
    bool bFirst = true;
    for( int cursym = 0, endsym = SetAny_size(&symbols); cursym != endsym; ++cursym ) {
      int sym = SetAny_getByIndexConstT(&symbols,cursym,int);
      if( ! MapAny_findConst(&reductions,&sym) )
        continue; // there can be no conflicts if there are no reductions
      const char *symstr = (sym==-1) ? "$" : String_Chars(&MapAny_findConstT(tokens,&sym,SymbolDef).m_name);
      if( SetAny_contains(&shiftsymbols,&sym) ) {
        if( bFirst ) {
          bFirst = false;
          fprintf(stderr, "State %d:\n", i);
        }
        fprintf(stderr, "shift/reduce conflict on %s\n", symstr);
        ++nconflicts;
      }
      VectorAny_clear(&symreductions);
      SetAny *reductionsforsym = &MapAny_findT(&reductions,&sym,SetAny);
      VectorAny_insertMany(&symreductions,VectorAny_size(&symreductions),SetAny_ptr(reductionsforsym),SetAny_size(reductionsforsym));
      for( int r0 = 0; r0 < VectorAny_size(&symreductions); ++r0 )
        for( int r1 = r0+1; r1 < VectorAny_size(&symreductions); ++r1 )
          if( ! VectorAny_ArrayOpConstT(&symreductions,r0,Production*)->m_rejectable && ! VectorAny_ArrayOpConstT(&symreductions,r1,Production*)->m_rejectable ) {
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
  Scope_Pop();
  return nconflicts;
}

void FirstsAndFollows_Compute(FirstsAndFollows *This, ParserDef *parser, FILE *vout, int verbosity) {
  FirstAndFollowState firstAndFollowState;
  Scope_Push();
  FirstAndFollowState_init(&firstAndFollowState, This, parser, vout, verbosity, true);
  FirstAndFollowState_ComputeFirsts(&firstAndFollowState);
  FirstAndFollowState_ComputeFollows(&firstAndFollowState);
  Scope_Pop();
}

void FirstsAndFollows_init(FirstsAndFollows *This, bool onstack) {
  MapAny_init(&This->m_firsts, &ProductionAndRestrictStateElement, getSetAnyElement(), false);
  MapAny_init(&This->m_follows, &ProductionAndRestrictStateElement, getSetAnyElement(), false);
  if (onstack)
    Push_Destroy(This, (vpstack_destroyer)FirstsAndFollows_destroy);
}

void FirstsAndFollows_destroy(FirstsAndFollows *This) {
  MapAny_destroy(&This->m_firsts);
  MapAny_destroy(&This->m_follows);
}

void LRParserSolution_init(LRParserSolution *This, bool onstack) {
  FirstsAndFollows_init(&This->m_firstsAndFollows, false);
  VectorAny_init(&This->m_states, &StateElement, false);
  MapAny_init(&This->m_shifts, getIntElement(), getMapAnyElement(), false);
  MapAny_init(&This->m_reductions, getIntElement(), getMapAnyElement(), false);
  if( onstack )
    Push_Destroy(This, (vpstack_destroyer)LRParserSolution_destroy);
}

void LRParserSolution_destroy(LRParserSolution *This) {
  FirstsAndFollows_destroy(&This->m_firstsAndFollows);
  VectorAny_destroy(&This->m_states);
  MapAny_destroy(&This->m_shifts);
  MapAny_destroy(&This->m_reductions);
}

void LLParserSolution_init(LLParserSolution *This, bool onstack) {
  FirstsAndFollows_init(&This->m_firstsAndFollows, false);
  if (onstack)
    Push_Destroy(This, (vpstack_destroyer)LLParserSolution_destroy);
}

void LLParserSolution_destroy(LLParserSolution *This) {
  FirstsAndFollows_destroy(&This->m_firstsAndFollows);
}

int LR_SolveParser(ParserDef *parser, LRParserSolution *solution, FILE *vout, int verbosity, int timed) {
  if( ! ParserDef_getStartProduction(parser) ) {
    fputs("The grammar definition requires a START production",stderr);
    return -1;
  }
  ticks_t start = getSystemTicks();
  ParserDef_computeRestrictAutomata(parser);
  FirstsAndFollows_Compute(&solution->m_firstsAndFollows, parser, vout, verbosity);
  LR_ComputeStatesAndActions(parser,solution,vout,verbosity);
  int nconflicts = LR_PrintConflicts(parser,solution,vout);
  if (verbosity >= 1) {
    PrintRules(parser,vout);
    LR_PrintStatesAndActions(parser,solution,vout);
  }
  ticks_t stop = getSystemTicks();
  if( timed ) {
    printf("Tokens %d\n", MapAny_size(&parser->m_tokens));
    printf("Productions %d\n", VectorAny_size(&parser->m_productions));
    printf("States %d\n", VectorAny_size(&solution->m_states));
    printf("Duration %.6f\n", (double)(stop-start)/(double)getSystemTicksFreq());
  }
  return nconflicts;
}

int LL_SolveParser(ParserDef *parser, LLParserSolution *solution, FILE *vout, int verbosity, int timed) {
  ticks_t start = getSystemTicks();
  ParserDef_computeRestrictAutomata(parser);
  FirstsAndFollows_Compute(&solution->m_firstsAndFollows, parser, vout, verbosity);
  ticks_t stop = getSystemTicks();
  if (timed) {
    printf("Tokens %d\n", MapAny_size(&parser->m_tokens));
    printf("Productions %d\n", VectorAny_size(&parser->m_productions));
    printf("Duration %.6f\n", (double)(stop - start) / (double)getSystemTicksFreq());
  }
  return 0;
}
