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
    Push_Destroy(This,StringAndIntSet_destroy);
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

ElementOps StringAndIntSetElement = {sizeof(StringAndIntSet), false, false, StringAndIntSet_init, StringAndIntSet_destroy, StringAndIntSet_LessThan, StringAndIntSet_Equal, StringAndIntSet_Assign, 0};
ElementOps ProductionsAtKeyElement = {sizeof(ProductionsAtKey), false, false, 0, 0, ProductionsAtKey_LessThan, ProductionsAtKey_Equal, 0, 0};
extern ElementOps ProductionStateElement;

struct FirstAndFollowComputer;
typedef struct FirstAndFollowComputer FirstAndFollowComputer;

struct FirstAndFollowComputer {
  const ParserDef *parser;
  ParserSolution *solution;
  FILE *out;
  int verbosity;
  MapAny /*< ProductionsAtKey,Vector<productionandrestrictstate_t> >*/ *productionsAtResults;
  SetAny /*<productionandrestrictstate_t>*/ followproductionset;
  VectorAny /*<productionandrestrictstate_t>*/ followproductions;
};

FirstAndFollowComputer_destroy(FirstAndFollowComputer *This) {
  SetAny_destroy(&This->followproductionset);
  VectorAny_destroy(&This->followproductions);
}

FirstAndFollowComputer_init(FirstAndFollowComputer *This, const ParserDef *_parser, ParserSolution *_solution, FILE *_out, int _verbosity,
  MapAny /*< ProductionsAtKey,Vector<productionandrestrictstate_t> >*/ *_productionsAtResults, bool onstack)
  {
    This->parser = _parser;
    This->solution = _solution;
    This->out = _out;
    This->verbosity = _verbosity;
    This->productionsAtResults = _productionsAtResults;
    SetAny_init(&This->followproductionset, &ProductionAndRestrictStateElement,false);
    VectorAny_init(&This->followproductions, &ProductionAndRestrictStateElement,false);
    if( onstack )
      Push_Destroy(This,FirstAndFollowComputer_destroy);
  }

void FirstAndFollowComputer_ComputeFirsts(FirstAndFollowComputer *This) {
  SetAny /*<int>*/ adding;
  Scope_Push();
  SetAny_init(&adding, getIntElement(), true);
  // No need to add terminals... we just test if the symbol is a terminal in the rest of the code
  /*
  for( Map<int,SymbolDef>::const_iterator cur = parser.m_tokdefs.begin(), end = parser.m_tokdefs.end(); cur != end; ++cur ) {
    if( cur->second.m_symboltype == SymbolTypeTerminal ) {
      int tokid = cur->second.m_tokid;
      for( int restrictstate = 0; restrictstate < parser.m_restrict.nstates(); ++restrictstate )
        solution.m_firsts[symbolandrestrictstate_t(tokid,restrictstate)].insert(cur->second.m_tokid);
    }
  }
  */
  bool added = true;
  while( added ) {
    added = false;
    for( int cur = 0, end = MapAny_size(&This->parser->m_tokdefs); cur != end; ++cur ) {
      const int *tok = 0;
      const SymbolDef *symbolDef = 0;
      MapAny_getByIndexConst(&This->parser->m_tokdefs,cur,&tok,&symbolDef);
      if( symbolDef->m_symboltype != SymbolTypeNonterminalProduction )
        continue;
      int tokid = symbolDef->m_tokid;
      for( int restrictstate = 0, nstates = RestrictAutomata_nstates(&This->parser->m_restrict); restrictstate < nstates; ++restrictstate ) {
        symbolandrestrictstate_t symbolandrestrictstate;
        symbolandrestrictstate_t_set(&symbolandrestrictstate, tokid, restrictstate);
        SetAny /*<int>*/ *firsts = &MapAny_findT(&This->solution->m_firsts,&symbolandrestrictstate,SetAny);
        if (!firsts) {
          SetAny_clear(&adding);
          MapAny_insert(&This->solution->m_firsts,&symbolandrestrictstate,&adding);
          firsts = &MapAny_findT(&This->solution->m_firsts, &symbolandrestrictstate, SetAny);
        }
        int beforecnt = SetAny_size(firsts);
        const Production *p = symbolDef->m_p;
        if( VectorAny_size(&p->m_symbols) == 0 )
          continue;
        int s = VectorAny_ArrayOpConstT(&p->m_symbols,0,int);
        if( ParserDef_getSymbolType(This->parser,s) == SymbolTypeTerminal ) {          
          if( This->verbosity > 2 ) {
            if( ! SetAny_contains(firsts,&s) ) {
              const String *name_s = &MapAny_findConstT(&This->parser->m_tokdefs,&s,SymbolDef).m_name;
              const String *name_tokid = &MapAny_findConstT(&This->parser->m_tokdefs,&tokid,SymbolDef).m_name;
              fprintf(This->out,"Adding %s to FIRST(%s,%d)\n",String_Chars(name_s),String_Chars(name_tokid),restrictstate);
            }
          }
          SetAny_insert(firsts,&s,0);
        } else {
          const VectorAny /*<productionandrestrictstate_t>*/ *productions = ParserDef_productionsAt(This->parser,p,0,restrictstate,This->productionsAtResults);
          for( int curp = 0, endp = VectorAny_size(productions); curp != endp; ++curp ) {
            const productionandrestrictstate_t *curPFS = &VectorAny_ArrayOpConstT(productions,curp,productionandrestrictstate_t);
            symbolandrestrictstate_t_set(&symbolandrestrictstate, curPFS->production->m_pid, curPFS->restrictstate);
            const SetAny /*<int>*/ *sfirsts = &MapAny_findConstT(&This->solution->m_firsts, &symbolandrestrictstate, SetAny);
            if (!sfirsts)
              continue;
            if( This->verbosity > 2 ) {
              SetAny_clear(&adding);
              SetAny_Assign(&adding,sfirsts);
              SetAny_eraseMany(&adding,SetAny_ptrConst(firsts),SetAny_size(firsts));
              if( SetAny_size(&adding) > 0 ) {
                for( int curadd = 0, endadd = SetAny_size(&adding); curadd != endadd; ++curadd ) {
                  int curtok = SetAny_getByIndexConstT(&adding,curadd,int);
                  const String *str_curtok = &MapAny_findConstT(&This->parser->m_tokdefs,&curtok,SymbolDef).m_name;
                  const String *str_tokid = &MapAny_findConstT(&This->parser->m_tokdefs,&tokid,SymbolDef).m_name;
                  fprintf(This->out,"Adding %s to FIRST(%s,%d)\n",String_Chars(str_curtok),String_Chars(str_tokid),restrictstate);
                }
              }
            }
            SetAny_insertMany(firsts,SetAny_ptrConst(sfirsts),SetAny_size(sfirsts));
          }
        }
        int aftercnt =  SetAny_size(firsts);
        if( beforecnt != aftercnt )
          added = true;
      }
    }
  }
  Scope_Pop();
}

void FirstAndFollowComputer_getFollows(FirstAndFollowComputer *This, const Production *p, int pos, int restrictstate, VectorAny /*< Pair< String,Set<int> > >*/ *followslist) {
  Scope_Push();
  StringAndIntSet followitem;
  StringAndIntSet_init(&followitem,true);

  VectorAny_clear(followslist);
  symbolandrestrictstate_t symbolandrestrictstate;
  if( pos+1== VectorAny_size(&p->m_symbols) ) {
    StringAndIntSet_clear(&followitem);
    String_ReFormatString(&followitem.str,"FOLLOW(%s,%d)", String_Chars(&MapAny_findConstT(&This->parser->m_tokdefs,&p->m_pid,SymbolDef).m_name), restrictstate);
    const SetAny *pidfollows = &MapAny_findConstT(&This->solution->m_follows,symbolandrestrictstate_t_set(&symbolandrestrictstate,p->m_pid,restrictstate),SetAny);
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
      const VectorAny /*<productionandrestrictstate_t>*/ *productions = ParserDef_productionsAt(This->parser,p,pos+1,restrictstate,This->productionsAtResults);
      for( int i = 0, n = VectorAny_size(productions); i != n; ++i ) {
        const productionandrestrictstate_t *curpat = &VectorAny_ArrayOpConstT(productions,i,productionandrestrictstate_t);
        if( ! SetAny_contains(&This->followproductionset,curpat) ) {
          SetAny_insert(&This->followproductionset,curpat,0);
          VectorAny_push_back(&This->followproductions,curpat);
        }
        StringAndIntSet_clear(&followitem);
        String_ReFormatString(&followitem.str,"FIRST(%s,%d)", String_Chars(&MapAny_findConstT(&This->parser->m_tokdefs,&curpat->production->m_pid,SymbolDef).m_name), curpat->restrictstate);
        const SetAny *pidfirsts = &MapAny_findConstT(&This->solution->m_firsts,symbolandrestrictstate_t_set(&symbolandrestrictstate,curpat->production->m_pid,curpat->restrictstate),SetAny);
        if( pidfirsts )
          SetAny_Assign(&followitem.set,pidfirsts);
        VectorAny_push_back(followslist,&followitem);
      }
    }
  }
  Scope_Pop();
}

void FirstAndFollowComputer_ComputeFollows(FirstAndFollowComputer *This) {
  symbolandrestrictstate_t symbolandrestrictstate;
  productionandrestrictstate_t productionandrestrictstate;
  VectorAny /*< StringAndIntSet >*/ followslist;
  StringAndIntSet followitem;
  SetAny /*<int>*/ adding;

  Scope_Push();
  VectorAny_init(&followslist, &StringAndIntSetElement,true);
  SetAny_init(&adding, getIntElement(), true);
  StringAndIntSet_init(&followitem, true);

  int eoftok = EOF_TOK;
  const Production *startProduction = ParserDef_getStartProductionConst(This->parser);
  symbolandrestrictstate_t_set(&symbolandrestrictstate, startProduction->m_pid, 0);
  productionandrestrictstate_t_set(&productionandrestrictstate, startProduction, 0);
  SetAny_insert(&adding, &eoftok, 0);
  MapAny_insert(&This->solution->m_follows, &adding, 0);

  SetAny_insert(&This->followproductionset, &productionandrestrictstate,0);
  VectorAny_push_back(&This->followproductions, &productionandrestrictstate);
  bool added = true;
  while( added ) {
    added = false;
    for( int ifp = 0; ifp < VectorAny_size(&This->followproductions); ++ifp ) {
      const Production *p = VectorAny_ArrayOpT(&This->followproductions,ifp,productionandrestrictstate_t).production;
      int restrictstate = VectorAny_ArrayOpT(&This->followproductions, ifp,productionandrestrictstate_t).restrictstate;

      for( int pos = 0, endpos = VectorAny_size(&p->m_symbols); pos != endpos; ++pos ) {
        int tok = VectorAny_ArrayOpConstT(&p->m_symbols,pos,int);
        SymbolType stype = ParserDef_getSymbolType(This->parser,tok);
        VectorAny_clear(&followslist);
        FirstAndFollowComputer_getFollows(This,p,pos,restrictstate,&followslist);
        if( This->verbosity > 2 ) {
          fputs("Evaluting ",This->out);
          Production_print(p,This->out,&This->parser->m_tokdefs,pos,restrictstate);
          fputs("\n",This->out);
        }
        if( stype == SymbolTypeTerminal ) {
          symbolandrestrictstate_t_set(&symbolandrestrictstate, tok, restrictstate);
          SetAny /*<int>*/ *follows = &MapAny_findT(&This->solution->m_follows, &symbolandrestrictstate, SetAny);
          if (!follows) {
            SetAny_clear(&adding);
            MapAny_insert(&This->solution->m_follows, &symbolandrestrictstate, &adding);
            follows = &MapAny_findT(&This->solution->m_follows, &symbolandrestrictstate, SetAny);
          }
          int before = SetAny_size(follows);
          for( int i = 0, n = VectorAny_size(&followslist); i < n; ++i ) {
            const StringAndIntSet *followitem = &VectorAny_ArrayOpConstT(&followslist,i,StringAndIntSet);
            const String *descstr = &followitem->str;
            const SetAny /*<int>*/ *followitemset = &followitem->set;
            if( This->verbosity > 2 ) {
              SetAny_clear(&adding);
              SetAny_Assign(&adding,followitemset);
              SetAny_eraseMany(&adding,SetAny_ptrConst(follows),SetAny_size(follows));
              if( SetAny_size(&adding) > 0 ) {
                fprintf(This->out, "%s {\n", String_Chars(descstr));
                for( int i = 0, n = SetAny_size(&adding); i < n; ++i ) {
                  int curadd = SetAny_getByIndexConstT(&adding,i,int);
                  const String *curadd_str = &MapAny_findConstT(&This->parser->m_tokdefs,&curadd,SymbolDef).m_name;
                  const String *tok_str = &MapAny_findConstT(&This->parser->m_tokdefs,&tok,SymbolDef).m_name;
                  fprintf(This->out,"Adding %s to FOLLOW(%s,%d)\n",String_Chars(curadd_str),String_Chars(tok_str),restrictstate);
                }
                fprintf(This->out, "} %s\n", String_Chars(descstr));
              }
            }
            SetAny_insertMany(follows,SetAny_ptrConst(followitemset),SetAny_size(followitemset));
          }
          int after = SetAny_size(follows);
          if (after > before)
            added = true;
        }
        else {
          const VectorAny /*<productionandrestrictstate_t>*/ *productions = ParserDef_productionsAt(This->parser,p,pos,restrictstate,This->productionsAtResults);
          for( int ip = 0, np = VectorAny_size(productions); ip < np; ++ip ) {
            const productionandrestrictstate_t *curpat = &VectorAny_ArrayOpConstT(productions,ip,productionandrestrictstate_t);
            if( ! SetAny_contains(&This->followproductionset,curpat) ) {
              SetAny_insert(&This->followproductionset,curpat,0);
              VectorAny_push_back(&This->followproductions,curpat);
            }
            symbolandrestrictstate_t_set(&symbolandrestrictstate, curpat->production->m_pid, curpat->restrictstate);
            SetAny /*<int>*/ *follows = &MapAny_findT(&This->solution->m_follows,&symbolandrestrictstate,SetAny);
            if (!follows) {
              SetAny_clear(&adding);
              MapAny_insert(&This->solution->m_follows, &symbolandrestrictstate, &adding);
              follows = &MapAny_findT(&This->solution->m_follows, &symbolandrestrictstate, SetAny);
            }
            int before = SetAny_size(follows);
            for( int i = 0, n = VectorAny_size(&followslist); i < n; ++i ) {
              StringAndIntSet_clear(&followitem);
              StringAndIntSet_Assign(&followitem,&VectorAny_ArrayOpT(&followslist,i,StringAndIntSet));
              const String *descstr = &followitem.str;
              const SetAny /*<int>*/ *followitemset = &followitem.set;
              if( This->verbosity > 2 ) {
                SetAny_clear(&adding);
                SetAny_Assign(&adding, followitemset);
                SetAny_eraseMany(&adding,SetAny_ptr(follows),SetAny_size(follows));
                if( SetAny_size(&adding) > 0 ) {
                  fprintf(This->out, "%s {\n", String_Chars(descstr));
                  for( int j = 0, m = SetAny_size(&adding); j < m; ++j ) {
                    int curadd = SetAny_getByIndexConstT(&adding,j,int);
                    fprintf(This->out,"Adding %s to FOLLOW(%s,%d)\n", String_Chars(&MapAny_findConstT(&This->parser->m_tokdefs,&curadd,SymbolDef).m_name),String_Chars(&MapAny_findConstT(&This->parser->m_tokdefs,&curpat->production->m_pid,SymbolDef).m_name),curpat->restrictstate);
                  }
                  fprintf(This->out, "} %s\n", String_Chars(descstr));
                }
              }
              SetAny_insertMany(follows,SetAny_ptrConst(followitemset),SetAny_size(followitemset));
            }
            int after = SetAny_size(follows);
            if (after > before)
              added = true;
          }
        }
      }
    }
  }
  Scope_Pop();
}

void FirstAndFollowComputer_closure(FirstAndFollowComputer *This, state_t *state) {
  int prevsize = 0;
  SetAny /*<ProductionState>*/ newparts;
  Scope_Push();
  SetAny_init(&newparts,&ProductionStateElement,true);
  while( state_t_size(state) > prevsize ) {
    prevsize = state_t_size(state);
    SetAny_clear(&newparts);
     for( int cur = 0, end = state_t_size(state); cur != end; ++cur ) {
      const ProductionState *curState = &state_t_getByIndexConst(state,cur);
      const VectorAny /*<productionandrestrictstate_t>*/ *productions = ParserDef_productionsAt(This->parser,curState->m_p,curState->m_pos,curState->m_restrictstate,This->productionsAtResults);
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

void FirstAndFollowComputer_nexts(FirstAndFollowComputer *This, const state_t *state, SetAny /*<int>*/ *nextSymbols) {
  for( int curs = 0, ends = state_t_size(state); curs != ends; ++curs ) {
    const ProductionState *curState = &state_t_getByIndexConst(state,curs);
    int symbol = ProductionState_symbol(curState);
    if( symbol == -1 )
      continue;
    if( ParserDef_getSymbolType(This->parser,symbol) == SymbolTypeTerminal )
      SetAny_insert(nextSymbols,&symbol,0);
    else {
      const VectorAny /*<productionandrestrictstate_t>*/ *productions = ParserDef_productionsAt(This->parser,curState->m_p,curState->m_pos,curState->m_restrictstate,This->productionsAtResults);
      for( int curp = 0, endp = VectorAny_size(productions); curp != endp; ++curp ) {
        const productionandrestrictstate_t *curProductionAndRestrictState = &VectorAny_ArrayOpConstT(productions,curp,productionandrestrictstate_t);
        SetAny_insert(nextSymbols,&curProductionAndRestrictState->production->m_pid,0);
      }
    }
  }
}

void FirstAndFollowComputer_advance(FirstAndFollowComputer *This, state_t *state, int tsymbol, state_t *nextState) {
  state_t_clear(nextState);
  const Production *p = 0;
  ProductionState ps;
  if( ParserDef_getSymbolType(This->parser,tsymbol) == SymbolTypeNonterminalProduction )
    p = MapAny_findConstT(&This->parser->m_tokdefs,&tsymbol,SymbolDef).m_p;
  for( int curs = 0, ends = state_t_size(state); curs != ends; ++curs ) {
    const ProductionState *curState = &state_t_getByIndexConst(state,curs);
    int symbol = ProductionState_symbol(curState);
    if( symbol == tsymbol || (p && p->m_nt == symbol && ! RestrictAutomata_isRestricted(&This->parser->m_restrict,curState->m_p,curState->m_pos,curState->m_restrictstate,p)) )
      state_t_insert(nextState,ProductionState_set(&ps,curState->m_p,curState->m_pos+1,curState->m_restrictstate),0);
  }
}

void FirstAndFollowComputer_ComputeStatesAndActions(FirstAndFollowComputer *This) {
  MapAny /*<state_t,int>*/ statemap;
  state_t state, nextState;
  SetAny /*<int>*/ nextSymbols;
  SetAny /*<int>*/ emptyintset;
  ProductionState ps;
  symbolandrestrictstate_t symbolandrestrictstate;
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

  state_t_insert(&state,ProductionState_set(&ps,ParserDef_getStartProductionConst(This->parser),0,0),0);
  FirstAndFollowComputer_closure(This,&state);
  int n = VectorAny_size(&This->solution->m_states);
  MapAny_insert(&statemap,&state,&n);
  VectorAny_push_back(&This->solution->m_states,&state);
  for( int i = 0; i < VectorAny_size(This->solution->m_states); ++i ) {
    if( This->verbosity > 2 )
      fprintf(This->out,"computing state %d\n",i);
    state_t_Assign(&state,&VectorAny_ArrayOpConstT(&This->solution->m_states,i,state_t));
    int stateIdx = i;
    FirstAndFollowComputer_nexts(This,&state,&nextSymbols);
    for( int cur = 0, end = SetAny_size(&nextSymbols); cur != end; ++cur ) {
      int symbol = SetAny_getByIndexConstT(&nextSymbols,cur,int);
      FirstAndFollowComputer_advance(This,&state,symbol,&nextState);
      FirstAndFollowComputer_closure(This,&nextState);
      if( state_t_empty(&nextState) )
        continue;
      int nextStateIdx = -1;
      const int *stateiter = &MapAny_findConstT(&statemap,&nextState,int);
      if( ! stateiter ) {
        nextStateIdx = VectorAny_size(&This->solution->m_states);
        MapAny_insert(&statemap,&nextState,&nextStateIdx);
        VectorAny_push_back(&This->solution->m_states,&nextState);
        if( This->verbosity > 2 )
          fprintf(This->out,"added state %d\n",nextStateIdx);
      } else {
        nextStateIdx = *stateiter;
      }
      shifttosymbols_t *shifttosymbols = &MapAny_findT(&This->solution->m_shifts,&stateIdx,shifttosymbols_t);
      if (!shifttosymbols) {
        MapAny_insert(&This->solution->m_shifts, &stateIdx, &emptyshifttosymbols);
        shifttosymbols = &MapAny_findT(&This->solution->m_shifts, &stateIdx, shifttosymbols_t);
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
      const SetAny /*<int>*/ *follows = &MapAny_findConstT(&This->solution->m_follows,symbolandrestrictstate_t_set(&symbolandrestrictstate,curProductionState->m_p->m_pid,curProductionState->m_restrictstate),SetAny);
      if (!follows)
        continue;
      reducebysymbols_t *reducebysymbols = &MapAny_findT(&This->solution->m_reductions,&stateIdx,reducebysymbols_t);
      if (!reducebysymbols) {
        MapAny_insert(&This->solution->m_reductions, &stateIdx, &emptyreducebysymbols);
        reducebysymbols = &MapAny_findT(&This->solution->m_reductions, &stateIdx, reducebysymbols_t);
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

void FirstAndFollowComputer_PrintRules(const FirstAndFollowComputer *This) {
  int i = 1;
  for( int cur = 0, end = VectorAny_size(&This->parser->m_productions); cur != end; ++cur ) {
    const Production *curP = VectorAny_ArrayOpConstT(&This->parser->m_productions,cur,Production*);
    fprintf(This->out, "#%d ", i++);
    Production_print(curP,This->out,&This->parser->m_tokdefs,-1,0);
    fputs("\n",This->out);
  }
  fputs("\n",This->out);
}

void FirstAndFollowComputer_PrintStatesAndActions(const FirstAndFollowComputer *This) {
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

  const MapAny /*<int,SymbolDef>*/ *tokens = &This->parser->m_tokdefs;
  fprintf(This->out, "%d states\n\n", VectorAny_size(This->solution->m_states));
  for( int i = 0, n = VectorAny_size(&This->solution->m_states); i != n; ++i ) {
    SetAny_clear(&symbols);
    SetAny_clear(&shiftsymbols);
    MapAny_clear(&reductions);

    fprintf(This->out, "State %d:\n", i);
    const state_t *state = &VectorAny_ArrayOpConstT(&This->solution->m_states,i,state_t);
    state_t_print(state,This->out,tokens);
    const shifttosymbols_t *shifttosymbols = &MapAny_findConstT(&This->solution->m_shifts,&i,shifttosymbols_t);
    if( shifttosymbols ) {
      for( int curshift = 0, endshift = MapAny_size(shifttosymbols); curshift != endshift; ++curshift ) {
        const int *tostate = 0;
        const SetAny /*<int>*/ *onsymbols = 0;
        MapAny_getByIndexConst(shifttosymbols,curshift,&tostate,&onsymbols);
        fprintf(This->out, "shift to state %d on ", *tostate);
        bool bFirst = true;
        for( int cursym = 0, endsym = SetAny_size(onsymbols); cursym != endsym; ++cursym ) {
          int onsymbol = SetAny_getByIndexConstT(onsymbols,cursym,int);
          if( bFirst )
            bFirst = false;
          else
            fputs(", ", This->out);
          fputs(String_Chars(&MapAny_findConstT(tokens,&onsymbol,SymbolDef).m_name), This->out);
          SetAny_insert(&shiftsymbols,&onsymbol,0);
          SetAny_insert(&symbols,&onsymbol,0);
        }
        fputs("\n",This->out);
      }
    }
    //typedef MapAny /*<int,reducebysymbols_t>*/ reducemap_t;
    //int *reduceby reducemap_t::const_iterator reduceiter = 
    const reducebysymbols_t *reducebysymbols = &MapAny_findConstT(&This->solution->m_reductions,&i,reducebysymbols_t);
    if( reducebysymbols ) {
      VectorAny_clear(&reduces);
      for( int curreduce = 0, endreduce = MapAny_size(reducebysymbols); curreduce != endreduce; ++curreduce ) {
        const Production *production = 0;
        const SetAny /*<int>*/ *reduceby;
        MapAny_getByIndexConst(reducebysymbols,curreduce,&production,&reduceby);
        VectorAny_push_back(&reduces,production);
      }
      if( VectorAny_size(reduces) > 1 )
        qsort(VectorAny_ptr(&reduces),VectorAny_size(&reduces),sizeof(Production*),Production_CompareId);
      for( int curp = 0, endp = VectorAny_size(&reduces); curp != endp; ++curp ) {
        Production *p = VectorAny_ArrayOpT(&reduces,curp,Production*);
        const SetAny /*<int>*/ *syms = &MapAny_findConstT(reducebysymbols,&p,SetAny);
        if( p->m_rejectable )
          fputs("(rejectable) ", This->out);
        fputs("reduce by production [", This->out);
        Production_print(p,This->out,tokens,-1,0);
        fputs("] on ", This->out);
        bool bFirst = true;
        for( int cursym = 0, endsym = SetAny_size(syms); cursym != endsym; ++cursym ) {
          int curSymbol = SetAny_getByIndexConstT(syms,cursym,int);
          const char *symstr = (curSymbol==-1) ? "$" : String_Chars(&MapAny_findConstT(tokens,&curSymbol,SymbolDef).m_name);
          if( bFirst )
            bFirst = false;
          else
            fputs(", ", This->out);
          fputs(symstr, This->out);
          SetAny *reductionsymbols = &MapAny_findT(&reductions, &curSymbol, SetAny);
          if (!reductionsymbols) {
            MapAny_insert(&reductions, &curSymbol, &emptyproductionset);
            reductionsymbols = &MapAny_findT(&reductions, &curSymbol, SetAny);
          }
          SetAny_insert(reductionsymbols,&p,0);
          SetAny_insert(&symbols,&curSymbol,0);
        }
        fputs("\n",This->out);
      }
    }
    for( int cursym = 0, endsym = SetAny_size(&symbols); cursym != endsym; ++cursym ) {
      int sym = SetAny_getByIndexConstT(&symbols,cursym,int);
      if( ! MapAny_find(&reductions,&sym) )
        continue; // there can be no conflicts if there are no reductions
      const char *symstr = (sym==-1) ? "$" : String_Chars(&MapAny_findConstT(tokens,&sym,SymbolDef).m_name);
      if( ! SetAny_contains(&shiftsymbols,&sym) )
        fprintf(This->out, "shift/reduce conflict on %s\n", symstr);
      VectorAny_clear(&symreductions); /*<const Production*>*/
      // MapAny /*<int,Set<const Production*> > */ reductions;
      const SetAny /*<const Production*>*/ *reduceProductions = &MapAny_findConstT(&reductions,&sym,SetAny);
      VectorAny_insertMany(&symreductions,VectorAny_size(&symreductions),SetAny_ptrConst(reduceProductions),SetAny_size(reduceProductions));
      for( int r0 = 0; r0 < VectorAny_size(&symreductions); ++r0 )
        for( int r1 = r0+1; r1 < VectorAny_size(&symreductions); ++r1 )
          if( ! VectorAny_ArrayOpConstT(&symreductions,r0,Production*)->m_rejectable && ! VectorAny_ArrayOpConstT(&symreductions,r1,Production*)->m_rejectable )
            fprintf(This->out, "reduce/reduce conflict on %s\n", symstr);
    }
    fputs("\n",This->out);
  }
  fputs("\n",This->out);
  Scope_Pop();
}

int FirstAndFollowComputer_PrintConflicts(const FirstAndFollowComputer *This) {
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
  const MapAny /*<int,SymbolDef>*/ *tokens = &This->parser->m_tokdefs;
  for( int i = 0, n = VectorAny_size(&This->solution->m_states); i != n; ++i ) {
    SetAny_clear(&symbols);
    SetAny_clear(&shiftsymbols);
    MapAny_clear(&reductions);
    const state_t *state = &VectorAny_ArrayOpConstT(&This->solution->m_states,i,state_t);
    shifttosymbols_t *shifttosymbols = &MapAny_findT(&This->solution->m_shifts,&i,shifttosymbols_t);
    if( shifttosymbols ) {
      for( int curshift = 0, endshift = MapAny_size(shifttosymbols); curshift != endshift; ++curshift ) {
        const int *shiftto = 0;
        SetAny /*<int>*/ *shiftsymbols = 0;
        MapAny_getByIndex(shifttosymbols,curshift,&shiftto,&shiftsymbols);
        for( int cursym = 0, endsym = SetAny_size(shiftsymbols); cursym != endsym; ++cursym ) {
          int sym = SetAny_getByIndexConstT(shiftsymbols,cursym,int);
          SetAny_insert(shiftsymbols,&sym,0);
          SetAny_insert(&symbols,&sym,0);
        }
      }
    }
    const reducebysymbols_t *reducebysymbols = &MapAny_findConstT(&This->solution->m_reductions,&i,reducebysymbols_t);
    if( reducebysymbols ) {
      for( int curreduce = 0, endreduce = MapAny_size(reducebysymbols); curreduce != endreduce; ++curreduce ) {
        const Production *production = 0;
        const SetAny /*<int>*/ *reducebys = 0;
        MapAny_getByIndexConst(reducebysymbols,curreduce,&production,&reducebys);
        for( int cursym = 0, endsym = SetAny_size(reducebys); cursym != endsym; ++cursym ) {
          int sym = SetAny_getByIndexConstT(reducebys,cursym,int);
          const char *symstr = (sym==-1) ? "$" : String_Chars(&MapAny_findConstT(tokens,&sym,SymbolDef).m_name);
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

int FirstAndFollowComputer_SolveParser(FirstAndFollowComputer *This) {
  FirstAndFollowComputer_ComputeFirsts(This);
  FirstAndFollowComputer_ComputeFollows(This);
  FirstAndFollowComputer_ComputeStatesAndActions(This);
  int nconflicts = FirstAndFollowComputer_PrintConflicts(This);
  if( This->verbosity >= 1 ) {
    FirstAndFollowComputer_PrintRules(This);
    FirstAndFollowComputer_PrintStatesAndActions(This);
  }
  return nconflicts;
}

int SolveParser(ParserDef *parser, ParserSolution *solution, FILE *vout, int verbosity, int timed) {
  if( ! ParserDef_getStartProduction(parser) ) {
    fputs("The grammar definition requires a START production",stderr);
    return -1;
  }
  ticks_t start = getSystemTicks();
  ParserDef_computeRestrictAutomata(parser);
  MapAny /*< ProductionsAtKey,Vector<productionandrestrictstate_t> >*/ productionsAtResults;
  FirstAndFollowComputer firstAndFollowComputer;
  Scope_Push();
  MapAny_init(&productionsAtResults, &ProductionsAtKeyElement, getVectorAnyElement(),true);
  FirstAndFollowComputer_init(&firstAndFollowComputer,parser,solution,vout,verbosity,&productionsAtResults,true);
  int result = FirstAndFollowComputer_SolveParser(&firstAndFollowComputer);
  ticks_t stop = getSystemTicks();
  if( timed ) {
    printf("Tokens %d\n", VectorAny_size(&parser->m_tokens));
    printf("Productions %d\n", VectorAny_size(&parser->m_productions));
    printf("States %d\n", VectorAny_size(&solution->m_states));
    printf("Duration %.6f\n", (double)(stop-start)/(double)getSystemTicksFreq());
  }
  Scope_Pop();
  return result;
}
