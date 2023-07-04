#include "parser.h"
#include "parsertokL.h"

ElementOps ActionItemElement = { sizeof(ActionItem), false, false, (elementInit)ActionItem_init, (elementDestroy)ActionItem_destroy, 0, 0, (elementAssign)ActionItem_Assign, 0 }; // This definition only suitable for VectorAny
ElementOps ProductionDescriptorElement = { sizeof(ProductionDescriptor), false, false, (elementInit)ProductionDescriptor_init, (elementDestroy)ProductionDescriptor_destroy, (elementLessThan)ProductionDescriptor_LessThan, (elementEqual)ProductionDescriptor_Equal, (elementAssign)ProductionDescriptor_Assign, 0 };
ElementOps ProductionDescriptorsElement = { sizeof(ProductionDescriptors), false, false, (elementInit)ProductionDescriptors_init, (elementDestroy)ProductionDescriptors_destroy, (elementLessThan)ProductionDescriptors_LessThan, (elementEqual)ProductionDescriptors_Equal, (elementAssign)ProductionDescriptors_Assign, 0 };
ElementOps SymbolDefElement = { sizeof(ProductionDescriptor), false, false, (elementInit)SymbolDef_init, (elementDestroy)SymbolDef_destroy, (elementLessThan)SymbolDef_LessThan, (elementEqual)SymbolDef_Equal, (elementAssign)SymbolDef_Assign, 0 };
ElementOps ProductionAndRestrictStateElement = { sizeof(productionandrestrictstate_t), false, false, 0, 0, (elementLessThan)productionandrestrictstate_t_LessThan, (elementEqual)productionandrestrictstate_t_Equal, 0, 0 };
ElementOps ProductionStateElement = { sizeof(ProductionState), false, false, 0, 0, (elementLessThan)ProductionState_LessThan, (elementEqual)ProductionState_Equal, 0, 0 };
ElementOps StateElement = { sizeof(state_t), false, false, (elementInit)state_t_init, (elementDestroy)state_t_destroy, (elementLessThan)state_t_LessThan, (elementEqual)state_t_Equal, (elementAssign)state_t_Assign, 0 };
ElementOps RestrictionElement = { sizeof(Restriction), false, false, (elementInit)Restriction_init, (elementDestroy)Restriction_destroy, (elementLessThan)Restriction_LessThan, (elementEqual)Restriction_Equal, (elementAssign)Restriction_Assign, 0 };
ElementOps ProductionsAtKeyElement = { sizeof(ProductionsAtKey), false, false, 0, 0, (elementLessThan)ProductionsAtKey_LessThan, (elementEqual)ProductionsAtKey_Equal, 0, 0 };

void *zalloc(size_t len) {
  void *ret = malloc(len);
  memset(ret,0,len);
  return ret;
}

static void error(Tokenizer *tokenizer, const char *err) {
  String errstr;
  String file;
  Scope_Push();
  String_init(&errstr,true);
  String_init(&file,true);
  String_AssignChars(&errstr,err);
  tokenizer->filename(tokenizer,&file);
  setParseError(tokenizer->line(tokenizer),tokenizer->col(tokenizer),String_Chars(&file),&errstr);
  Scope_Pop();
}

static void errorString(Tokenizer *tokenizer, const String *err) {
  error(tokenizer,String_Chars(err));
}

// parser : parsepart +
// parsepart : (typedefrule|production|associativityrule|precedencerule|restrictrule) SEMI
// typedefrule : TYPEDEF ID nonterminal+
// productiondecl : REJECTABLE? production action?
// production: nonterminal COLON symbol+ (VERTICAL symbol+)+
// nonterminal : START | ID
// symbol : ID | ERROR
// associativityrule : ['LEFT-ASSOCIATIVE'|'RIGHT-ASSOCIATIVE'|'NON-ASSOCIATIVE'] productiondescriptors
// precedencerule : PRECEDENCE precedenceparts
// precedenceparts : precedencepart ['>' precedencepart]+
// precedencepart : productiondescriptors
// productiondescriptors : productiondescriptors productiondescriptor
// productiondescriptor : '[' nonterminal COLON (symbol|'.' symbol)+ ']'
// productions : ('[' nonterminal COLON (symbol)+ ']')+
// restrictrule : DISALLOW productiondescriptors (ARROW productiondescriptors '*'?)+ NOTARROW productions

void ActionItem_init(ActionItem *This, bool onstack) {
  This->m_actiontype = ActionTypeUnknown;
  String_init(&This->m_str,false);
  This->m_dollarnum = -1;
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)ActionItem_destroy);
}

void ActionItem_destroy(ActionItem *This) {
  String_clear(&This->m_str);
}

void ActionItem_Assign(ActionItem *lhs, const ActionItem *rhs) {
  lhs->m_actiontype = rhs->m_actiontype;
  lhs->m_dollarnum = rhs->m_dollarnum;
  String_AssignString(&lhs->m_str,&rhs->m_str);
}

/*
void Production_init(Production *This) {
  This->m_rejectable = false;
  This->m_nt = -1;
  This->m_pid = -1;
  VectorAny_init(&This->m_symbols,getIntElement());
  VectorAny_init(&This->m_action,&ActionItemElement);
  This->m_lineno = -1;
  String_init(&This->m_filename);
}
*/

void Production_destroy(Production *This) {
  VectorAny_destroy(&This->m_symbols);
  VectorAny_destroy(&This->m_action);
  String_clear(&This->m_filename);
}

void Production_init(Production *This, bool rejectable, int nt, const VectorAny /*<int>*/ *symbols, const VectorAny /*<ActionItem>*/ *action, int lineno, const String *filename, bool onstack) {
  This->m_rejectable = rejectable;
  This->m_nt = nt;
  This->m_pid = -1;
  VectorAny_init(&This->m_symbols,getIntElement(),false);
  VectorAny_init(&This->m_action,&ActionItemElement,false);
  VectorAny_Assign(&This->m_symbols,symbols);
  VectorAny_Assign(&This->m_action,action);
  This->m_lineno = lineno;
  String_init(&This->m_filename,false);
  String_AssignString(&This->m_filename,filename);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)Production_destroy);
}

Production *Production_clone(const Production *This) {
  Production *ret = (Production*)zalloc(sizeof(Production));
  Production_init(ret, This->m_rejectable, This->m_nt, &This->m_symbols, &This->m_action, This->m_lineno, &This->m_filename,false);
  return ret;
}

bool Production_Equal(const Production *lhs, const Production *rhs) {
  return lhs->m_pid == rhs->m_pid && lhs->m_nt == rhs->m_nt && VectorAny_Equal(&lhs->m_symbols,&rhs->m_symbols);
}

bool Production_NotEqual(const Production *lhs, const Production *rhs) {
  return ! Production_Equal(lhs,rhs);
}

bool Production_LessThan(const Production *lhs, const Production *rhs) {
  if( lhs->m_pid < rhs->m_pid )
    return true;
  if( lhs->m_pid > rhs->m_pid )
    return false;
  if( lhs->m_pid != -1 && lhs->m_pid == rhs->m_pid )
    return false;
  if( lhs->m_nt < rhs->m_nt )
    return true;
  if( lhs->m_nt > rhs->m_nt )
    return false;
  if( VectorAny_LessThan(&lhs->m_symbols,&rhs->m_symbols) )
    return true;
  return false;
}

void Production_print(const Production *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens, int pos /*=-1*/, int restrictstate /*=0*/) {
  const SymbolDef *sdef = &MapAny_findConstT(tokens,&This->m_nt,SymbolDef);
  fputs(String_Chars(&sdef->m_name), out);
  if( restrictstate != 0 )
    fprintf(out, "{%d}", restrictstate);
  fputs(" :", out);
  for( int i = 0; i < VectorAny_size(&This->m_symbols); ++i ) {
    if( pos == i )
      fputc('.',out);
    else
      fputc(' ',out);
    int tok = VectorAny_ArrayOpConstT(&This->m_symbols,i,int);
    sdef = &MapAny_findConstT(tokens,&tok,SymbolDef);
    fputs(String_Chars(&sdef->m_name),out);
  }
  if( pos == VectorAny_size(&This->m_symbols) )
    fputc('.',out);
}

int Production_CompareId(const void *lhs, const void *rhs) {
  const Production *l = *(const Production**)lhs, *r = *(const Production**)rhs;
  return l->m_pid - r->m_pid;
}

void SymbolDef_init(SymbolDef *This, bool onstack) {
  This->m_tokid = -1;
  String_init(&This->m_name,false);
  This->m_symboltype = SymbolTypeUnknown;
  String_init(&This->m_semantictype,false);
  This->m_p = 0;
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)SymbolDef_destroy);
}

void SymbolDef_destroy(SymbolDef *This) {
  String_clear(&This->m_name);
  String_clear(&This->m_semantictype);
}

bool SymbolDef_LessThan(const SymbolDef *lhs, const SymbolDef *rhs) {
  return lhs->m_tokid < rhs->m_tokid;
}
bool SymbolDef_Equal(const SymbolDef *lhs, const SymbolDef *rhs) {
  return lhs->m_tokid == rhs->m_tokid;
}

void SymbolDef_Assign(SymbolDef *lhs, const SymbolDef *rhs) {
  lhs->m_tokid = rhs->m_tokid;
  String_AssignString(&lhs->m_name,& rhs->m_name);
  lhs->m_symboltype = rhs->m_symboltype;
  lhs->m_semantictype = rhs->m_semantictype;
  lhs->m_p = rhs->m_p;
}

SymbolDef *SymbolDef_setinfo(SymbolDef *This, int tokid, const String *name, SymbolType symbolType) {
  This->m_tokid = tokid;
  String_AssignString(&This->m_name,name);
  This->m_symboltype = symbolType;
  return This;
}

ProductionState *ProductionState_set(ProductionState *This, const Production *p, int pos, int restrictstate) {
  This->m_p = p;
  This->m_pos = pos;
  This->m_restrictstate = restrictstate;
  return This;
}

bool ProductionState_LessThan(const ProductionState *lhs, const ProductionState *rhs) {
  if( Production_LessThan(lhs->m_p,rhs->m_p) )
    return true;
  else if( Production_LessThan(rhs->m_p,lhs->m_p) )
    return false;
  if( lhs->m_restrictstate < rhs->m_restrictstate )
    return true;
  else if( rhs->m_restrictstate < lhs->m_restrictstate )
    return false;
  if( lhs->m_pos < rhs->m_pos )
    return true;
  return false;
}

bool ProductionState_Equal(const ProductionState *lhs, const ProductionState *rhs) {
  return Production_Equal(lhs->m_p,rhs->m_p) && lhs->m_restrictstate == rhs->m_restrictstate && lhs->m_pos == rhs->m_pos;
}

int ProductionState_symbol(const ProductionState *This) {
  if( This->m_pos >= VectorAny_size(&This->m_p->m_symbols) )
    return -1;
  return VectorAny_ArrayOpConstT(&This->m_p->m_symbols,This->m_pos,int);
}

void ProductionDescriptor_init(ProductionDescriptor *This, bool onstack) {
  VectorAny_init(&This->m_symbols, getIntElement(),false);
  This->m_nt = -1;
  This->m_dots = 0;
  This->m_dotcnt = 0;
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)ProductionDescriptor_destroy);
}

void ProductionDescriptor_destroy(ProductionDescriptor *This) {
  VectorAny_destroy(&This->m_symbols);
}

ProductionDescriptor *ProductionDescriptor_setinfo(ProductionDescriptor *This, int nt, const VectorAny /*<int>*/ *symbols) {
  This->m_nt = nt;
  VectorAny_clear(&This->m_symbols);
  This->m_dots = 0;
  This->m_dotcnt = 0;
	int j = 0;
  for( int i = 0, n = VectorAny_size(symbols); i < n; ++i ) {
	  int sym = VectorAny_ArrayOpConstT(symbols,i,int);
	  if( sym == MARKER_DOT ) {
		  if( (This->m_dots&(1<<j)) == 0 ) {
			  This->m_dots |= (1<<j);
			  ++This->m_dotcnt;
		  }
	  } else {
		  VectorAny_push_back(&This->m_symbols,&sym);
		  ++j;
	  }
  }
  return This;
}

void ProductionDescriptor_setInfoWithDots(ProductionDescriptor *This, int nt, const VectorAny /*<int>*/ *symbols, int dots, int dotcnt) {
  This->m_nt = nt;
  VectorAny_Assign(&This->m_symbols,symbols);
  This->m_dots = dots;
  This->m_dotcnt = dotcnt;
}

bool ProductionDescriptor_hasSameProductionAs(const ProductionDescriptor *This, const ProductionDescriptor *rhs) {
  return This->m_nt == rhs->m_nt && VectorAny_Equal(&This->m_symbols,&rhs->m_symbols);
}

int ProductionDescriptor_appearancesOf(const ProductionDescriptor *This, int sym, int *at) {
	if( sym == MARKER_DOT ) {
		*at = This->m_dots;
		return This->m_dotcnt;
	}
	*at = 0;
	int cnt = 0;
	for( int i = 0, n = VectorAny_size(&This->m_symbols); i < n; ++i ) {
		if( VectorAny_ArrayOpConstT(&This->m_symbols,i,int) == sym ) {
		*at |= (1<<i);
		++cnt;
		}
	}
	return cnt;
}

bool ProductionDescriptor_hasDotAt(const ProductionDescriptor *This, int pos) {
	return (This->m_dots&(1<<pos)) != 0;
}

bool ProductionDescriptor_matchesProduction(const ProductionDescriptor *This, const Production *p) {
  // '.' is ignored for matching purposes.
  return This->m_nt == p->m_nt && VectorAny_Equal(&This->m_symbols,&p->m_symbols);
}

bool ProductionDescriptor_matchesProductionAndPosition(const ProductionDescriptor *This, const Production *p, int pos) {
  return ProductionDescriptor_matchesProduction(This,p) && ProductionDescriptor_hasDotAt(This,pos);
}

bool ProductionDescriptor_addDotsFrom(ProductionDescriptor *This, const ProductionDescriptor *rhs) {
  if( ! ProductionDescriptor_hasSameProductionAs(This,rhs) )
    return false;
  ProductionDescriptor_addDots(This,rhs->m_dots);
  return true;
}

void ProductionDescriptor_addDots(ProductionDescriptor *This, int dots) {
  This->m_dots |= dots;
  This->m_dotcnt = 0;
  for( int i = 0, n = VectorAny_size(&This->m_symbols); i < n; ++i )
	  if( (This->m_dots & (1<<i)) != 0 )
		  ++This->m_dotcnt;
}

void ProductionDescriptor_removeDots(ProductionDescriptor *This, int dots) {
  This->m_dots &= ~dots;
  This->m_dotcnt = 0;
  for( int i = 0, n = VectorAny_size(&This->m_symbols); i < n; ++i )
	  if( (This->m_dots & (1<<i)) != 0 )
		  ++This->m_dotcnt;
}

bool ProductionDescriptor_LessThan(const ProductionDescriptor *lhs, const ProductionDescriptor *rhs) {
  if( lhs->m_nt < rhs->m_nt )
    return true;
  else if( rhs->m_nt < lhs->m_nt )
    return false;
  if( lhs->m_dots < rhs->m_dots )
    return true;
  else if( lhs->m_dots > rhs->m_dots )
    return false;
  if( lhs->m_dotcnt < rhs->m_dotcnt )
    return true;
  else if( lhs->m_dotcnt > rhs->m_dotcnt )
    return false;
  if( VectorAny_LessThan(&lhs->m_symbols,&rhs->m_symbols) )
    return true;
  return false;
}

void ProductionDescriptor_Assign(ProductionDescriptor *lhs, const ProductionDescriptor *rhs) {
  lhs->m_nt = rhs->m_nt;
  lhs->m_dots = rhs->m_dots;
  lhs->m_dotcnt = rhs->m_dotcnt;
  VectorAny_Assign(&lhs->m_symbols,&rhs->m_symbols);
}

ProductionDescriptor *ProductionDescriptor_clone(const ProductionDescriptor *This) {
  ProductionDescriptor *ret = (ProductionDescriptor*)zalloc(sizeof(ProductionDescriptor));
  ProductionDescriptor_init(ret,false);
  ret->m_nt = This->m_nt;
  VectorAny_Assign(&ret->m_symbols,&This->m_symbols);
  ret->m_dots = This->m_dots;
  ret->m_dotcnt = This->m_dotcnt;
  return ret;
}

void ProductionDescriptor_UnDottedProductionDescriptor(const ProductionDescriptor *This, ProductionDescriptor *out) {
  ProductionDescriptor_setinfo(out, This->m_nt, &This->m_symbols);
}

void ProductionDescriptor_DottedProductionDescriptor(const ProductionDescriptor *This, ProductionDescriptor *out, int nt, Assoc assoc)  {
  if( assoc == AssocNull ) {
    ProductionDescriptor_UnDottedProductionDescriptor(This,out);
    return;
  }
  int ntat=0, ntcnt=0;
  int dots=0, dotcnt=0;
  ntcnt = ProductionDescriptor_appearancesOf(This,nt,&ntat);
  if( assoc == AssocNon ) {
	  dots = ntat;
	  dotcnt = ntcnt;
  } else if( assoc == AssocLeft ) {
	  if( ntcnt ) {
		  int firstnt = 0;
		  while( (ntat&(1<<firstnt)) == 0 )
			  ++firstnt;
		  dots = ntat&(~(1<<firstnt));
		  dotcnt = ntcnt-1;
	  }
  } else if( assoc == AssocRight ) {
	  if( ntcnt ) {
		  int lastnt = 0;
		  while( (ntat>>lastnt) != 1 )
			  ++lastnt;
		  dots = ntat&(~(1<<lastnt));
		  dotcnt = ntcnt-1;
	  }
  }
  ProductionDescriptor_setInfoWithDots(out,This->m_nt,&This->m_symbols,dots,dotcnt);
}

void ProductionDescriptor_symbolsAtDots(const ProductionDescriptor *This, VectorAny /*<int>*/ *symbols) {
	VectorAny_clear(symbols);
	for( int i = 0, n = VectorAny_size(&This->m_symbols); i < n; ++i ) {
		if( ProductionDescriptor_hasDotAt(This,i) )
			VectorAny_push_back(symbols,&VectorAny_ArrayOpConstT(&This->m_symbols,i,int));
	}
}

bool ProductionDescriptor_Equal(const ProductionDescriptor *lhs, const ProductionDescriptor *rhs) {
  return lhs->m_nt == rhs->m_nt && lhs->m_dots == rhs->m_dots && lhs->m_dotcnt == rhs->m_dotcnt && VectorAny_Equal(&lhs->m_symbols,&rhs->m_symbols);
}

bool ProductionDescriptor_NotEqual(const ProductionDescriptor *lhs, const ProductionDescriptor *rhs) {
  return ! ProductionDescriptor_Equal(lhs,rhs);
}

void ProductionDescriptor_print(const ProductionDescriptor *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  fputc('[',out);
  const SymbolDef *sdef = &MapAny_findConstT(tokens,&This->m_nt,SymbolDef);
  fputs(String_Chars(&sdef->m_name), out);
  fputs(" :", out);
  for( int i = 0; i < VectorAny_size(&This->m_symbols); ++i ) {
    if( ProductionDescriptor_hasDotAt(This,i) ) {
      fputc('.',out);
    } else {
      fputc(' ',out);
    }
    int tok = VectorAny_ArrayOpConstT(&This->m_symbols,i,int);
    sdef = &MapAny_findConstT(tokens,&tok,SymbolDef);
    fputs(String_Chars(&sdef->m_name),out);
  }
  fputc(']',out);
}

void ProductionDescriptor_clear(ProductionDescriptor *This) {
  VectorAny_clear(&This->m_symbols);
  This->m_nt = -1;
  This->m_dots = 0;
  This->m_dotcnt = 0;
}

void ProductionDescriptors_init(ProductionDescriptors *This, bool onstack) {
  SetAny_init(&This->m_productionDescriptors,&ProductionDescriptorElement,onstack);
}

void ProductionDescriptors_destroy(ProductionDescriptors *This) {
  SetAny_destroy(&This->m_productionDescriptors);
}

void ProductionDescriptors_clear(ProductionDescriptors *This) {
  SetAny_clear(&This->m_productionDescriptors);
}

ProductionDescriptors *ProductionDescriptors_UnDottedProductionDescriptors(const ProductionDescriptors *This, ProductionDescriptors *psundotted) {
  ProductionDescriptor pd;
  Scope_Push();
  ProductionDescriptor_init(&pd,true);
  SetAny_clear(&psundotted->m_productionDescriptors);
  for( int cur = 0, end = SetAny_size(&This->m_productionDescriptors); cur != end; ++cur ) {
    ProductionDescriptor_UnDottedProductionDescriptor(&SetAny_getByIndexConstT(&This->m_productionDescriptors,cur,ProductionDescriptor), &pd);
    SetAny_insert(&psundotted->m_productionDescriptors,&pd,0);
  }
  Scope_Pop();
  return psundotted;
}

static ProductionRegex *rxpd(ProductionDescriptor *pd);
static ProductionRegex *rxbinary(BinaryOp op, ProductionRegex *lhs, ProductionRegex *rhs);

ProductionDescriptors *ProductionDescriptors_DottedProductionDescriptors(const ProductionDescriptors *This, ProductionDescriptors *pdsdotted, Assoc assoc) {
  ProductionDescriptors_clear(pdsdotted);
  ProductionDescriptor pddotted;
  Scope_Push();
  ProductionDescriptor_init(&pddotted,true);
  int nt = -1;
  for (int i = 0, n = ProductionDescriptors_size(This); i < n; ++i) {
    const ProductionDescriptor *pd = &ProductionDescriptors_getByIndexConst(This, i);
    ProductionDescriptor_clear(&pddotted);
    if (nt == -1)
      nt = pd->m_nt;
    ProductionDescriptor_DottedProductionDescriptor(pd, &pddotted, nt, assoc);
    ProductionDescriptors_insert(pdsdotted,&pddotted,0);
  }
  Scope_Pop();
  return pdsdotted;
}

ProductionDescriptors *ProductionDescriptors_clone(const ProductionDescriptors *This) {
  ProductionDescriptors *ret = (ProductionDescriptors*)zalloc(sizeof(ProductionDescriptors));
  ProductionDescriptors_init(ret,false);
  SetAny_Assign(&ret->m_productionDescriptors,&This->m_productionDescriptors);
  return ret;
}

void ProductionDescriptors_print(const ProductionDescriptors *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  for( int c = 0, e = SetAny_size(&This->m_productionDescriptors); c != e; ++c ) {
    const ProductionDescriptor *pd = &SetAny_getByIndexConstT(&This->m_productionDescriptors,c,ProductionDescriptor);
    ProductionDescriptor_print(pd,out,tokens);
  }
}

bool ProductionDescriptors_LessThan(const ProductionDescriptors *lhs, const ProductionDescriptors *rhs) {
  return SetAny_LessThan(&lhs->m_productionDescriptors,&rhs->m_productionDescriptors);
}

bool ProductionDescriptors_Equal(const ProductionDescriptors *lhs, const ProductionDescriptors *rhs) {
  return SetAny_Equal(&lhs->m_productionDescriptors,&rhs->m_productionDescriptors);
}

bool ProductionDescriptors_NotEqual(const ProductionDescriptors *lhs, const ProductionDescriptors *rhs) {
  return ! ProductionDescriptors_Equal(lhs,rhs);
}

void ProductionDescriptors_Assign(ProductionDescriptors *lhs, const ProductionDescriptors *rhs) {
  SetAny_Assign(&lhs->m_productionDescriptors, &rhs->m_productionDescriptors);
}

bool ProductionDescriptors_matchesProduction(const ProductionDescriptors *This, const Production *lhs) {
  for( int c = 0, e = SetAny_size(&This->m_productionDescriptors); c != e; ++c ) {
    const ProductionDescriptor *pd = &SetAny_getByIndexConstT(&This->m_productionDescriptors,c,ProductionDescriptor);
    if( ProductionDescriptor_matchesProduction(pd,lhs) )
      return true;
  }
  return false;
}

bool ProductionDescriptors_matchesProductionAndPosition(const ProductionDescriptors *This, const Production *lhs, int pos) {
  for( int c = 0, e = SetAny_size(&This->m_productionDescriptors); c != e; ++c ) {
    const ProductionDescriptor *pd = &SetAny_getByIndexConstT(&This->m_productionDescriptors,c,ProductionDescriptor);
    if( ProductionDescriptor_matchesProductionAndPosition(pd,lhs,pos) )
      return true;
  }
  return false;
}

void ProductionRegex_init(ProductionRegex *This, bool onstack) {
  This->m_t = RxType_None;
  This->m_op = BinaryOp_None;
  This->m_lhs = 0;
  This->m_rhs = 0;
  This->m_low = 1;
  This->m_high = 1;
  This->m_pd = 0;
  This->m_paren = false;
  if (onstack)
    Push_Destroy(This, (vpstack_destroyer)ProductionRegex_destroy);
}

void ProductionRegex_destroy(ProductionRegex *This) {
  if (This->m_lhs) {
    ProductionRegex_destroy(This->m_lhs);
    free(This->m_lhs);
    This->m_lhs = 0;
  }
  if (This->m_rhs) {
    ProductionRegex_destroy(This->m_rhs);
    free(This->m_rhs);
    This->m_rhs = 0;
  }
  if (This->m_pd ) {
    ProductionDescriptor_destroy(This->m_pd);
    free(This->m_pd);
    This->m_pd = 0;
  }
}

void ProductionRegex_print(ProductionRegex *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  if( ! This )
    return;
  if (This->m_paren)
    fputs("(", out);
  switch (This->m_t) {
  case RxType_Production:
    ProductionDescriptor_print(This->m_pd, out, tokens);
    break;
  case RxType_BinaryOp:
    switch (This->m_op) {
    case BinaryOp_Or:
      ProductionRegex_print(This->m_lhs, out, tokens);
      fputs(" | ", out);
      ProductionRegex_print(This->m_rhs, out, tokens);
      break;
    case BinaryOp_Concat:
      ProductionRegex_print(This->m_lhs, out, tokens);
      ProductionRegex_print(This->m_rhs, out, tokens);
      break;
    default:
      break;
    }
    break;
  case RxType_Many:
    ProductionRegex_print(This->m_lhs, out, tokens);
    if (This->m_low == 0 && This->m_high == -1)
      fputs("*", out);
    else if (This->m_low == 1 && This->m_high == -1)
      fputs("+", out);
    else if (This->m_low == 0 && This->m_high == 1)
      fputs("?", out);
    else if (This->m_low == -1)
      fprintf(out, "<,%d>", This->m_high);
    else if (This->m_high == -1)
      fprintf(out, "<%d,>", This->m_low);
    else
      fprintf(out, "<%d,%d>", This->m_low, This->m_high);
    break;
  default:
    break;
  }
  if (This->m_paren)
    fputs(")", out);
}

void RestrictRule_print(const RestrictRule *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  ProductionRegex_print(This->m_rx, out, tokens);
  fputs(" --> ", out);
  ProductionDescriptors_print(&This->m_restriction->m_at, out, tokens);
  fputs(" -/-> ", out);
  ProductionDescriptors_print(&This->m_restriction->m_restricted, out, tokens);
}

void PrecedenceRule_init(PrecedenceRule *This, bool onstack) {
  VectorAny_init(&This->m_productionDescriptors, getPointerElement(), onstack);
}

void PrecedenceRule_destroy(PrecedenceRule *This) {
  VectorAny_destroy(&This->m_productionDescriptors);
}

void PrecedenceRule_print(const PrecedenceRule *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  bool first = true;
  fputs("PRECEDENCE ",out);
  for (int cur = 0, end = PrecedenceRule_size(This); cur != end; ++cur) {
    const ProductionDescriptors *pds = PrecedenceRule_ArrayOpConst(This,cur);
    if( first )
      first = false;
    else
      fputs(" > ",out);
    ProductionDescriptors_print(pds,out,tokens);
  }
}

void PrecedenceRule_push_back(PrecedenceRule *This, ProductionDescriptors *part) {
  VectorAny_push_back(&This->m_productionDescriptors,&part);
}

AssociativeRule *AssociativeRule_set(AssociativeRule *This, Assoc assoc, ProductionDescriptors *pds) {
  This->m_assoc = assoc;
  This->m_pds = pds;
  return This;
}

void AssociativeRule_print(const AssociativeRule *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  switch(This->m_assoc) {
  case AssocLeft:
    fputs("LEFT-ASSOCIATIVE ", out);
    break;
  case AssocRight:
    fputs("RIGHT-ASSOCIATIVE ", out);
    break;
  case AssocNon:
    fputs("NON-ASSOCIATIVE ", out);
    break;
  case AssocNull:
    break;
  }
  ProductionDescriptors_print(This->m_pds,out,tokens);
}

productionandrestrictstate_t *productionandrestrictstate_t_set(productionandrestrictstate_t*This, const Production *production, int restrictstate) {
  This->production = production;
  This->restrictstate = restrictstate;
  return This;
}

bool ProductionsAtKey_LessThan(const ProductionsAtKey *lhs, const ProductionsAtKey *rhs) {
  if( lhs->m_p < rhs->m_p )
    return true;
  else if( lhs->m_p > rhs->m_p )
    return false;
  if( lhs->m_pos < rhs->m_pos )
    return true;
  else if( lhs->m_pos > rhs->m_pos )
    return false;
  if( lhs->m_restrictstate < rhs->m_restrictstate )
    return true;
  return false;
}

bool ProductionsAtKey_Equal(const ProductionsAtKey *lhs, const ProductionsAtKey *rhs) {
    return lhs->m_p == rhs->m_p && lhs->m_pos == rhs->m_pos && lhs->m_restrictstate == rhs->m_restrictstate;
}

bool ProductionsAtKey_NotEqual(const ProductionsAtKey *lhs, const ProductionsAtKey *rhs) {
  return ProductionsAtKey_Equal(lhs,rhs);
}

void ParserDef_init(ParserDef *This, FILE *vout, int verbosity, bool onstack) {
  This->m_nextsymbolid = 0;
  This->m_startProduction = 0;
  This->m_vout = vout;
  This->m_verbosity = verbosity;
  MapAny_init(&This->m_tokens, getStringElement(), getIntElement(),false);
  MapAny_init(&This->m_tokdefs, getIntElement(), &SymbolDefElement,false);
  VectorAny_init(&This->m_productions, getPointerElement(),false);
  VectorAny_init(&This->m_restrictrules, getPointerElement(),false);
  VectorAny_init(&This->m_assocrules, getPointerElement(),false);
  VectorAny_init(&This->m_precrules, getPointerElement(),false);
  RestrictAutomata_init(&This->m_restrict,false);
  MapAny_init(&This->productionsAtResults, &ProductionsAtKeyElement, getVectorAnyElement(), false);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)ParserDef_destroy);
}

void ParserDef_destroy(ParserDef *This) {
  MapAny_destroy(&This->m_tokens);
  MapAny_destroy(&This->m_tokdefs);
  VectorAny_destroy(&This->m_productions);
  VectorAny_destroy(&This->m_restrictrules);
  VectorAny_destroy(&This->m_assocrules);
  VectorAny_destroy(&This->m_precrules);
  RestrictAutomata_destroy(&This->m_restrict);
  MapAny_destroy(&This->productionsAtResults);
}

void ParserDef_addProduction(ParserDef *This, Tokenizer *toks, Production *p) {
  if( p->m_nt == ParserDef_getStartNt(This) ) {
    if( This->m_startProduction )
      error(toks,"START production can be defined only once");
    This->m_startProduction = p;
  }
  VectorAny_push_back(&This->m_productions, &p);
  String s;
  Scope_Push();
  String_init(&s,true);
  String_AddCharsInPlace(&s,"[");
  String_AddStringInPlace(&s,&MapAny_findT(&This->m_tokdefs,&p->m_nt,SymbolDef).m_name);
  String_AddCharsInPlace(&s," :");
  for( int i = 0; i < VectorAny_size(&p->m_symbols); ++i ) {
    String_AddCharsInPlace(&s," ");
    String_AddStringInPlace(&s,&MapAny_findT(&This->m_tokdefs,VectorAny_ArrayOp(&p->m_symbols,i),SymbolDef).m_name);
  }
  String_AddCharsInPlace(&s,"]");
  p->m_pid = ParserDef_findOrAddSymbolId(This,toks,&s,SymbolTypeNonterminalProduction);
  MapAny_findT(&This->m_tokdefs,&p->m_pid,SymbolDef).m_p = p;
  Scope_Pop();
}

bool ParserDef_addProductionWithCheck(ParserDef *This, Production *p) {
  if( p->m_nt == ParserDef_getStartNt(This) ) {
    if( This->m_startProduction )
      return false;
    This->m_startProduction = p;
  }
  VectorAny_push_back(&This->m_productions,p);
  String s;
  Scope_Push();
  String_init(&s,true);
  String_AddCharsInPlace(&s,"[");
  String_AddStringInPlace(&s,&MapAny_findT(&This->m_tokdefs,&p->m_nt,SymbolDef).m_name);
  String_AddCharsInPlace(&s," :");
  for( int i = 0; i < VectorAny_size(&p->m_symbols); ++i ) {
    String_AddCharsInPlace(&s," ");
    String_AddStringInPlace(&s,&MapAny_findT(&This->m_tokdefs,VectorAny_ArrayOp(&p->m_symbols,i),SymbolDef).m_name);
  }
  String_AddCharsInPlace(&s,"]");
  p->m_pid = ParserDef_addSymbolId(This,&s,SymbolTypeNonterminalProduction);
  MapAny_findT(&This->m_tokdefs,&p->m_pid,SymbolDef).m_p = p;
  Scope_Pop();
  return true;
}

SymbolType ParserDef_getSymbolType(const ParserDef *This, int tok) {
  if( ! MapAny_findConst(&This->m_tokdefs,&tok) )
    return SymbolTypeUnknown;
  return MapAny_findConstT(&This->m_tokdefs,&tok,SymbolDef).m_symboltype;
}

int ParserDef_findSymbolId(const ParserDef *This, const String *s) {
  if( ! MapAny_findConst(&This->m_tokens,s) )
    return -1;
  return MapAny_findConstT(&This->m_tokens,s,int);
}

int ParserDef_addSymbolId(ParserDef *This, const String *name, SymbolType stype) {
  if( MapAny_find(&This->m_tokens,name) )
    return -1;
  int tokid = This->m_nextsymbolid++;
  MapAny_insert(&This->m_tokens,name,&tokid);
  SymbolDef sdef;
  Scope_Push();
  SymbolDef_init(&sdef,true);
  SymbolDef_setinfo(&sdef,tokid,name,stype);
  MapAny_insert(&This->m_tokdefs,&tokid,&sdef);
  Scope_Pop();
  return tokid;
 } 

static const char *getSymbolTypeName(SymbolType stype) {
  switch( stype ){
  case SymbolTypeTerminal:
    return "Terminal";
  case SymbolTypeNonterminal:
    return "Nonterminal";
  case SymbolTypeNonterminalProduction:
    return "Nonterminal Production";
  default:
    break;
  }
  return "?";
}

int ParserDef_findOrAddSymbolIdChars(ParserDef *This, Tokenizer *toks, const char *name, SymbolType stype) {
  String s;
  Scope_Push();
  String_init(&s,true);
  String_AssignChars(&s,name);
  int tok = ParserDef_findOrAddSymbolId(This,toks,&s,stype);
  Scope_Pop();
  return tok;
}

int ParserDef_findOrAddSymbolId(ParserDef *This, Tokenizer *toks, const String *name, SymbolType stype) {
  const int *ptokid = &MapAny_findConstT(&This->m_tokens,name,int);
  if( ptokid ) {
    SymbolDef *symboldef = &MapAny_findT(&This->m_tokdefs,ptokid,SymbolDef);
    if( stype == SymbolTypeUnknown ) {
      // is ok
    } else if( symboldef->m_symboltype == SymbolTypeUnknown && stype != SymbolTypeUnknown ) {
      // now we know
      symboldef->m_symboltype = stype;
    } else if( symboldef->m_symboltype != stype ) {
      String err;
      Scope_Push();
      String_init(&err,true);
      String_AssignChars(&err,"Attempted to turn ");
      String_AddCharsInPlace(&err, getSymbolTypeName(symboldef->m_symboltype));
      String_AddCharsInPlace(&err, " ");
      String_AddStringInPlace(&err,&symboldef->m_name);
      String_AddCharsInPlace(&err," into a ");
      String_AddCharsInPlace(&err, getSymbolTypeName(stype));
      errorString(toks,&err);
      Scope_Pop();
    }
    return *ptokid;
  }
  int tokid = This->m_nextsymbolid++;
  // Don't want that one.
  if( tokid == MARKER_DOT )
    ++tokid;
  MapAny_insert(&This->m_tokens,name,&tokid);
  SymbolDef symboldef;
  Scope_Push();
  SymbolDef_init(&symboldef,true);
  MapAny_insert(&This->m_tokdefs,&tokid,SymbolDef_setinfo(&symboldef,tokid,name,stype));
  Scope_Pop();
  return tokid;
}

int ParserDef_getStartNt(const ParserDef *This) {
  return 0;
}

int ParserDef_getExtraNt(const ParserDef *This) {
  return 1;
}

const VectorAny /*<productionandrestrictstate_t>*/ *ParserDef_productionsAt(ParserDef *This, const Production *p, int pos, int restrictstate) {
  ProductionsAtKey badkey;
  badkey.m_p = 0;
  badkey.m_pos = -1;
  badkey.m_restrictstate = -1;
  if( ! MapAny_find(&This->productionsAtResults,&badkey) ) {
    VectorAny v;
    Scope_Push();
    VectorAny_init(&v,&ProductionAndRestrictStateElement,true);
    MapAny_insert(&This->productionsAtResults,&badkey,&v);
    Scope_Pop();
  }
  if( pos < 0 || pos >= VectorAny_size(&p->m_symbols) )
    return &MapAny_findConstT(&This->productionsAtResults,&badkey,VectorAny);
  int symbol = VectorAny_ArrayOpConstT(&p->m_symbols,pos,int);
  if( ParserDef_getSymbolType(This,symbol) != SymbolTypeNonterminal )
    return &MapAny_findConstT(&This->productionsAtResults,&badkey,VectorAny);
  ProductionsAtKey key;
  key.m_p = p;
  key.m_pos = pos;
  key.m_restrictstate = restrictstate;
  if( MapAny_find(&This->productionsAtResults,&key) )
    return &MapAny_findConstT(&This->productionsAtResults,&key,VectorAny);
  if( This->m_verbosity > 2 ) {
    static int nout = 1;
    Production_print(p,This->m_vout,&This->m_tokdefs,pos,restrictstate);
    fprintf(This->m_vout," has... (%d)\n",nout);
    nout += 1;
  }
  VectorAny productions;
  Scope_Push();
  VectorAny_init(&productions, &ProductionAndRestrictStateElement,true);
  for( int cur = 0, end = VectorAny_size(&This->m_productions); cur != end; ++cur ) {
    const Production *ptest = VectorAny_ArrayOpConstT(&This->m_productions,cur,Production*);
    if( ptest->m_nt != symbol )
      continue;
    int nxtState = RestrictAutomata_nextState(&This->m_restrict,p,pos,restrictstate,ptest);
    if( nxtState == -1 ) {
      if( This->m_verbosity > 2 ) {
        fputs("  -/-> ", This->m_vout);
      }
    } else {
      if( This->m_verbosity > 2 ) {
        fputs("   --> ", This->m_vout);
      }
      productionandrestrictstate_t productionandrestrictstate;
      productionandrestrictstate.production = (Production*)ptest;
      productionandrestrictstate.restrictstate = nxtState;
      VectorAny_push_back(&productions,&productionandrestrictstate);
    }
    if( This->m_verbosity > 2 ) {
      Production_print(ptest,This->m_vout,&This->m_tokdefs,-1,nxtState==-1?0:nxtState);
      fputs("\n", This->m_vout);
    }
  }
  MapAny_insert(&This->productionsAtResults,&key,&productions);
  Scope_Pop();
  return &MapAny_findConstT(&This->productionsAtResults,&key,VectorAny);
}

// No rule may nest into another rule in the same group of productions, including itself.
// We'll assume the rules are valid because they were validated by the parser.
void ParserDef_expandAssocRules(ParserDef *This) {
  for (int ruleno = 0, endruleno = VectorAny_size(&This->m_assocrules); ruleno < endruleno; ++ruleno ) {
    AssociativeRule *rule = VectorAny_ArrayOpT(&This->m_assocrules, endruleno-ruleno-1, AssociativeRule*);
    if (This->m_verbosity > 2) {
      fputs("expanding assoc rule : ", This->m_vout);
      AssociativeRule_print(rule, This->m_vout, &This->m_tokdefs);
      fputs("\n", This->m_vout);
    }
    RestrictRule *rr = (RestrictRule*)zalloc(sizeof(RestrictRule));
    rr->m_rx = 0; // no prerequisite to reach this restrict state
    rr->m_restriction = (Restriction*)zalloc(sizeof(Restriction));
    Restriction_init(rr->m_restriction,false);
    ProductionDescriptors_DottedProductionDescriptors(rule->m_pds, &rr->m_restriction->m_at, rule->m_assoc);
    ProductionDescriptors_Assign(&rr->m_restriction->m_restricted, rule->m_pds);
    ParserDef_addRestrictRule(This, rr);
  }
}

// No rule may appear in a rule of higher precedence.
// We'll assume the rules are valid because they were validated by the parser.
void ParserDef_expandPrecRules(ParserDef *This) {
  ProductionDescriptors descriptors;
  Scope_Push();
  ProductionDescriptors_init(&descriptors,true);
  for (int ruleno = 0, endruleno = VectorAny_size(&This->m_precrules); ruleno < endruleno; ++ruleno) {
    PrecedenceRule *precrule = VectorAny_ArrayOpT(&This->m_precrules,endruleno-ruleno-1, PrecedenceRule*);
    if (This->m_verbosity > 2) {
      fputs("expanding precedence rule : ", This->m_vout);
      PrecedenceRule_print(precrule, This->m_vout, &This->m_tokdefs);
      fputs("\n", This->m_vout);
    }
    for (int curpr = 0, endpr = PrecedenceRule_size(precrule); curpr < endpr; ++curpr) {
      ProductionDescriptors *pds = PrecedenceRule_ArrayOp(precrule,endpr-curpr-1);
      if( curpr > 0 ) {
        RestrictRule *rr = (RestrictRule*)zalloc(sizeof(RestrictRule));
        rr->m_rx = 0; // no prerequisites to get to the restrict rule
        rr->m_restriction = (Restriction*)zalloc(sizeof(Restriction));
        Restriction_init(rr->m_restriction,false);
        ProductionDescriptors_DottedProductionDescriptors(pds, &rr->m_restriction->m_at, AssocNon);
        ProductionDescriptors_insertMany(&rr->m_restriction->m_restricted, ProductionDescriptors_ptr(&descriptors), ProductionDescriptors_size(&descriptors));
        ParserDef_addRestrictRule(This, rr);
      }
      ProductionDescriptors_insertMany(&descriptors,ProductionDescriptors_ptr(pds),ProductionDescriptors_size(pds));
    }
  }
  Scope_Pop();
}

void ParserDef_getProductionsOfNt(const ParserDef *This, int nt, VectorAny /*<Production*>*/ *productions) {
  VectorAny_clear(productions);
  for( int i = 0, n = VectorAny_size(&This->m_productions); i < n; ++i ) {
    const Production *p = VectorAny_ArrayOpConstT(&This->m_productions, i, Production*);
    if( p->m_nt == nt )
      VectorAny_push_back(productions,&p);
  }
}

void ParserDef_addRestrictRule(ParserDef *This, RestrictRule *rule) {
  static int i = 1;
  VectorAny_push_back(&This->m_restrictrules,&rule);
  if( This->m_verbosity > 2 ) {
    fprintf(This->m_vout, "added (add # %d)\n", i++);
    RestrictRule_print(rule,This->m_vout,&This->m_tokdefs);
    fputs("\n", This->m_vout);
  }
}

void ParserDef_print(const ParserDef *This, FILE *out) {
  for( int i = 0, n = VectorAny_size(&This->m_productions); i != n; ++i) {
    const Production *p = VectorAny_ArrayOpConstT(&This->m_productions,i,Production*);
    Production_print(p,out,&This->m_tokdefs,-1,0);
    fputs(" ;\n",out);
  }
  for( int i = 0, n = VectorAny_size(&This->m_precrules); i != n; ++i ) {
    const PrecedenceRule *r = VectorAny_ArrayOpConstT(&This->m_precrules,i,PrecedenceRule*);
    PrecedenceRule_print(r,out,&This->m_tokdefs);
    fputs(" ;\n", out);
  }
  for( int i = 0, n = VectorAny_size(&This->m_assocrules); i != n; ++i) {
    const AssociativeRule *r = VectorAny_ArrayOpConstT(&This->m_assocrules,i,AssociativeRule*);
    AssociativeRule_print(r,out,&This->m_tokdefs);
    fputs(" ;\n", out);
  }
  for( int i = 0, n = VectorAny_size(&This->m_restrictrules); i != n; ++i) {
    const RestrictRule *r = VectorAny_ArrayOpConstT(&This->m_restrictrules,i,RestrictRule*);
    RestrictRule_print(r,out,&This->m_tokdefs);
    fputs(" ;\n", out);
  }
}

Production *ParserDef_getStartProduction(ParserDef *This) {
  return This->m_startProduction;
}

const Production *ParserDef_getStartProductionConst(const ParserDef *This) {
  return This->m_startProduction;
}

static int ParseNonterminal(Tokenizer *toks, ParserDef *parser) {
  if( toks->peek(toks) == PARSERTOK_START ) {
    toks->discard(toks);
    return ParserDef_getStartNt(parser);
  } else if( toks->peek(toks) == PARSERTOK_ID ) {
    String s;
    Scope_Push();
    String_init(&s,true);
    toks->tokstr(toks,&s);
    toks->discard(toks);
    int tok = ParserDef_findOrAddSymbolId(parser,toks,&s,SymbolTypeNonterminal);
    Scope_Pop();
    return tok;
  }
  return -1;
}

static int ParseNonterminalOrExtra(Tokenizer *toks, ParserDef *parser) {
  if( toks->peek(toks) == PARSERTOK_START ) {
    toks->discard(toks);
    return ParserDef_getStartNt(parser);
  } else if( toks->peek(toks) == PARSERTOK_EXTRA ) {
    String s;
    Scope_Push();
    String_init(&s,true);
    toks->tokstr(toks,&s);
    toks->discard(toks);
    int tok = ParserDef_getExtraNt(parser);
    Scope_Pop();
    return tok;
  } else if( toks->peek(toks) == PARSERTOK_ID ) {
    String s;
    Scope_Push();
    String_init(&s,true);
    toks->tokstr(toks,&s);
    toks->discard(toks);
    int tok = ParserDef_findOrAddSymbolId(parser,toks,&s,SymbolTypeNonterminal);
    Scope_Pop();
    return tok;
  }
  return -1;
}

static int ParseSymbol(Tokenizer *toks, ParserDef *parser) {
  if( toks->peek(toks) == PARSERTOK_ID ) {
    String s;
    Scope_Push();
    String_init(&s,true);
    toks->tokstr(toks,&s);
    toks->discard(toks);
    int tok = ParserDef_findOrAddSymbolId(parser,toks,&s,SymbolTypeUnknown);
    Scope_Pop();
    return tok;
  } else if ( toks->peek(toks) == PARSERTOK_ERROR ) {
    toks->discard(toks);
    int tok = ParserDef_findOrAddSymbolIdChars(parser,toks,"PARSE_ERROR",SymbolTypeNonterminal);
    return tok;
  }
  return -1;
}

static void ParseSymbols(Tokenizer *toks, ParserDef *parser, VectorAny *symbolsOut) {
  int s = ParseSymbol(toks,parser);
  if( s == -1 )
    error(toks,"expected symbol");
  VectorAny_push_back(symbolsOut,&s);
  while( (s = ParseSymbol(toks,parser)) != -1 )
    VectorAny_push_back(symbolsOut,&s);
}

static void ParseAction(Tokenizer *toks, ParserDef *parser, VectorAny /*<ActionItem>*/ *action) {
  if( toks->peek(toks) != PARSERTOK_LBRACE )
    return;
  else {
    ActionItem item;
    Scope_Push();
    ActionItem_init(&item,true);
    item.m_actiontype = ActionTypeSrc;
    toks->tokstr(toks,&item.m_str);
    VectorAny_push_back(action,&item);
    toks->discard(toks);
    Scope_Pop();
  }
  while( toks->peek(toks) != -1 && toks->peek(toks) != PARSERTOK_RBRACE ) {
    if( toks->peek(toks) == PARSERTOK_LBRACE )
      ParseAction(toks,parser,action);
    if( toks->peek(toks) == PARSERTOK_DOLLARDOLLAR ) {
      ActionItem item;
      Scope_Push();
      ActionItem_init(&item,true);
      item.m_actiontype = ActionTypeDollarDollar;
      VectorAny_push_back(action,&item);
      Scope_Pop();
    } else if( toks->peek(toks) == PARSERTOK_DOLLARNUMBER ) {
      ActionItem item;
      String s;
      Scope_Push();
      ActionItem_init(&item,true);
      item.m_actiontype = ActionTypeDollarNumber;
      String_init(&s,true);
      toks->tokstr(toks,&s);
      item.m_dollarnum = atoi(String_Chars(&s)+1);
      VectorAny_push_back(action,&item);
      Scope_Pop();
    } else if( toks->peek(toks) == PARSERTOK_DOLLAREXTRA ) {
      ActionItem item;
      Scope_Push();
      ActionItem_init(&item,true);
      item.m_actiontype = ActionTypeDollarExtra;
      VectorAny_push_back(action,&item);
      Scope_Pop();
    } else {
      ActionItem item;
      Scope_Push();
      ActionItem_init(&item,true);
      item.m_actiontype = ActionTypeSrc;
      toks->tokstr(toks,&item.m_str);
      VectorAny_push_back(action,&item);
      Scope_Pop();
    }
    toks->discard(toks);
  }
  if( toks->peek(toks) != PARSERTOK_RBRACE )
    error(toks,"Missing '}'");
  ActionItem item;
  Scope_Push();
  ActionItem_init(&item,true);
  item.m_actiontype = ActionTypeSrc;
  toks->tokstr(toks,&item.m_str);
  VectorAny_push_back(action,&item);
  toks->discard(toks);
  Scope_Pop();
}

static void ParseProduction(Tokenizer *toks, ParserDef *parser, VectorAny /*<Production*>*/ *productionsOut) {
  VectorAny /*<int>*/ symbols;
  VectorAny /*<ActionItem>*/ action;
  String filename;
  String productionName;

  Scope_Push();
  VectorAny_init(&symbols,getIntElement(),true);
  VectorAny_init(&action,&ActionItemElement,true);
  String_init(&filename,true);
  String_init(&productionName,true);

  bool rejectable = false;
  if( toks->peek(toks) == PARSERTOK_REJECTABLE ) {
    rejectable = true;
    toks->discard(toks);
  }
  int nt = ParseNonterminal(toks,parser);
  if( nt == -1 )
    error(toks,"Expected nonterminal");
  if( toks->peek(toks) != PARSERTOK_COLON )
    error(toks,"Expected ':'");
  toks->discard(toks);
  while(true) {
    VectorAny_clear(&symbols);
    VectorAny_clear(&action);
    String_AssignChars(&productionName,"");
    String_AssignChars(&filename,"");
    if( toks->peek(toks) == PARSERTOK_ID || toks->peek(toks) == PARSERTOK_ERROR )
      ParseSymbols(toks,parser,&symbols);
    int lineno = toks->line(toks);
    toks->filename(toks,&filename);
    if( toks->peek(toks) == PARSERTOK_LBRACE )
      ParseAction(toks,parser,&action);
    char buf[64];
    sprintf(buf,"Production-%d",VectorAny_size(&parser->m_productions)+1);
    String_AssignChars(&productionName,buf);
    Production *p = (Production*)zalloc(sizeof(Production));
    Production_init(p,rejectable,nt,&symbols,&action,lineno,&filename,false);
    VectorAny_push_back(productionsOut,&p);
    if( toks->peek(toks) != PARSERTOK_VSEP )
      break;
    toks->discard(toks);
  }
  Scope_Pop();
}

static int ParseSymbolOrDottedSymbol(Tokenizer *toks, ParserDef *parser, bool *dotted) {
  int symbol = -1;
  *dotted = false;
  if(  toks->peek(toks) == PARSERTOK_DOT ) {
    toks->discard(toks);
    *dotted = true;
    symbol = ParseSymbol(toks,parser);
  } else
    symbol = ParseSymbol(toks,parser);
  return symbol;
}

static ProductionDescriptor *ParseProductionDescriptor(Tokenizer *toks, ParserDef *parser, ProductionDescriptor *pd) {
  if( toks->peek(toks) != PARSERTOK_LBRKT )
    error(toks,"Expected '[' to start production descriptor");
  toks->discard(toks);
  int nt = ParseNonterminal(toks,parser);
  if( nt == -1 )
    error(toks,"expected nonterminal");
  if( toks->peek(toks) != PARSERTOK_COLON )
    error(toks,"Expected ':' in production descriptor");
  toks->discard(toks);
  bool dotted = false;
  int symbol = ParseSymbolOrDottedSymbol(toks,parser,&dotted);
  if( symbol == -1 )
    error(toks,"expected symbol or '.' symbol ");
  VectorAny /*<int>*/ symbols;
  Scope_Push();
  VectorAny_init(&symbols,getIntElement(),true);
  while( symbol != -1 ) {
    if( dotted ) {
      int md = MARKER_DOT;
      VectorAny_push_back(&symbols,&md);
    }
    VectorAny_push_back(&symbols,&symbol);
    symbol = ParseSymbolOrDottedSymbol(toks,parser,&dotted);
  }
  if( toks->peek(toks) != PARSERTOK_RBRKT )
    error(toks,"Expected ']' to end production descriptor");
  toks->discard(toks);
  if( ! pd ) {
    pd = (ProductionDescriptor*)zalloc(sizeof(ProductionDescriptor));
    ProductionDescriptor_init(pd,false);
  }
  ProductionDescriptor_setinfo(pd,nt,&symbols);
  Scope_Pop();
  return pd;
}

static String *ParseSquarePairTokType(Tokenizer* toks, String *toktype) {
  String tok;
  Scope_Push();
  String_init(&tok, true);
  if (toks->peek(toks) != PARSERTOK_LBRKT)
    error(toks, "expected '['");
  toks->tokstr(toks, &tok);
  toks->discard(toks);
  toktype = String_AddString(toktype, &tok);
  // just real simple, one level deep.  if we need more later we'll augment the parser
  while (toks->peek(toks) != PARSERTOK_RBRKT) {
    toks->tokstr(toks, &tok);
    toktype = String_AddString(toktype, &tok);
    toks->discard(toks);
  }
  toks->tokstr(toks, &tok);
  toktype = String_AddString(toktype, &tok);
  toks->discard(toks);
  Scope_Pop();
  return toktype;
}

static void ParseTypedefRule(Tokenizer *toks, ParserDef *parser) {
  if( toks->peek(toks) != PARSERTOK_TYPEDEFTOK )
    return;
  toks->discard(toks);
  if( toks->peek(toks) != PARSERTOK_ID )
    error(toks,"expected id after typedef");
  String toktype;
  String tok;
  Scope_Push();
  String_init(&toktype,true);
  String_init(&tok,true);
  toks->tokstr(toks,&toktype);
  toks->discard(toks);
  while( toks->peek(toks) == PARSERTOK_DOT ) {
    toks->tokstr(toks,&tok);
    String *s = String_AddString(&toktype, &tok);
    String_AssignString(&toktype,s);
    toks->discard(toks);
    if( toks->peek(toks) != PARSERTOK_ID )
      error(toks,"expected id after '.'");
    toks->tokstr(toks,&tok);
    s = String_AddString(&toktype, &tok);
    toks->discard(toks);
    String_AssignString(&toktype,s);
  }
  if(toks->peek(toks) == PARSERTOK_LBRKT) {
    String_AssignString(&toktype,ParseSquarePairTokType(toks, &toktype));
  }
  while( toks->peek(toks) == PARSERTOK_STAR ) {
    String tok;
    Scope_Push();
    String_init(&tok,true);
    toks->tokstr(toks,&tok);
    String_AddString(&toktype,&tok);
    toks->discard(toks);
    Scope_Pop();
  }
  int nt = ParseNonterminalOrExtra(toks,parser);
  while( nt != -1 ) {
    SymbolDef *symboldef = &MapAny_findT(&parser->m_tokdefs,&nt,SymbolDef);
    if( symboldef->m_symboltype != SymbolTypeNonterminal ) {
      String err;
      Scope_Push();
      String_init(&err,true);
      String_AssignChars(&err,"TYPEDEF may only be applied to nonterminals, attempted to TYPEDEF ");
      String_AddStringInPlace(&err,&symboldef->m_name);
      errorString(toks, &err);
      Scope_Pop();
    }
    if( String_length(&symboldef->m_semantictype) != 0 ) {
      String err;
      Scope_Push();
      String_init(&err,true);
      String_AssignChars(&err,"TYPEDEF can only be assigned once per type.  Attempated to reassign TYPEDEF of ");
      String_AddStringInPlace(&err,&symboldef->m_name);
      errorString(toks, &err);
      Scope_Pop();
    }
    String_AssignString(&symboldef->m_semantictype,&toktype);
    nt = ParseNonterminalOrExtra(toks,parser);
  }
  Scope_Pop();
}

static ProductionDescriptors *ParseProductionDescriptors(Tokenizer *toks, ParserDef *parser, ProductionDescriptors *pds) {
  ProductionDescriptor pd;
  Scope_Push();
  ProductionDescriptor_init(&pd,true);
  ParseProductionDescriptor(toks,parser,&pd);
  ProductionDescriptors_insert(pds,&pd,0);
  while( toks->peek(toks) == PARSERTOK_LBRKT ) {
    ProductionDescriptor_clear(&pd);
    ParseProductionDescriptor(toks,parser,&pd);
    ProductionDescriptors_insert(pds,&pd,0);
  }
  Scope_Pop();
  return pds;
}

static ProductionDescriptors *ParseProductions(Tokenizer *toks, ParserDef *parser);

static void checkProductionDescriptorsSameNonterminal(Tokenizer *toks, ProductionDescriptors *p, int lastnt, const char *name) {
  // Check all parts have the same nonterminal.
  int nt = ProductionDescriptors_getByIndexConst(p,0).m_nt;
  if( lastnt != -1 && nt != lastnt ) {
    String err;
    Scope_Push();
    String_init(&err,true);
    String_AssignChars(&err,"The dotted nonterminals of a ");
    String_AddCharsInPlace(&err,name);
    String_AddCharsInPlace(&err," must be the same as the previous ");
    String_AddCharsInPlace(&err,name);
    String_AddCharsInPlace(&err,"'s nonterminal");
    errorString(toks,&err);
    Scope_Pop();
  }
  for( int cur = 0, end = ProductionDescriptors_size(p); cur != end; ++cur ) {
    const ProductionDescriptor *pd = &ProductionDescriptors_getByIndexConst(p,cur);
    if( nt != pd->m_nt ) {
      String err;
      Scope_Push();
      String_init(&err,true);
      String_AssignChars(&err,"All production descriptors in an ");
      String_AddCharsInPlace(&err,name);
      String_AddCharsInPlace(&err," must have the same nonterminal");
      errorString(toks,&err);
      Scope_Pop();
    }
  }
}
#if 0
static int checkProductionDescriptorsSameNonterminalAndDotsAgree(Tokenizer *toks, ProductionDescriptors *p, int lastnt, const char *name) {
	VectorAny /*<int>*/ symbolsAtDots;
  Scope_Push();
  VectorAny_init(&symbolsAtDots,getIntElement(),true);
  checkProductionDescriptorsSameNonterminal(toks,p,lastnt,name);
  // Check all parts have the same nonterminal.
  int dottednt = -1;
  for( int cur = 0, end = ProductionDescriptors_size(p); cur != end; ++cur ) {
    const ProductionDescriptor *pd = &ProductionDescriptors_getByIndexConst(p,cur);
    // Check that all dotted nonterminals are the same (and that there is at least 1)
    VectorAny_clear(&symbolsAtDots);
    ProductionDescriptor_symbolsAtDots(pd,&symbolsAtDots);
	  int dotcnt = VectorAny_size(&symbolsAtDots);
    for( int cur = 0, end = VectorAny_size(&symbolsAtDots); cur != end; ++cur ) {
		  if( dottednt == -1 )
			  dottednt = VectorAny_ArrayOpConstT(&symbolsAtDots,cur,int);
		  else if( VectorAny_ArrayOpConstT(&symbolsAtDots,cur,int) != dottednt ) {
			  String err;
        Scope_Push();
        String_init(&err,true);
        String_AssignChars(&err,"All dots in a ");
			  String_AddCharsInPlace(&err,name);
			  String_AddCharsInPlace(&err," must have the same nonterminal");
			  errorString(toks,&err);
        Scope_Pop();
		  }
    }
    if( dotcnt == 0 ) {
			String err;
      Scope_Push();
      String_init(&err,true);
      String_AssignChars(&err,"Each part of a ");
			String_AddCharsInPlace(&err,name);
      String_AssignChars(&err," must have at least one dot");
			errorString(toks,&err);
      Scope_Pop();
    }
  }
  Scope_Pop();
  return dottednt;
}
#endif
static AssociativeRule *ParseAssociativeRule(Tokenizer *toks, ParserDef *parser) {
  if( toks->peek(toks) != PARSERTOK_LEFTASSOC && toks->peek(toks) != PARSERTOK_RIGHTASSOC && toks->peek(toks) != PARSERTOK_NONASSOC )
    error(toks,"expected LEFT-ASSOCIATIVE or RIGHT-ASSOCITIVE or NON-ASSOCIATIVE");
  int iassoc = toks->peek(toks);
  Assoc assoc = AssocNull;
  if( iassoc == PARSERTOK_LEFTASSOC )
    assoc = AssocLeft;
  else if( iassoc == PARSERTOK_RIGHTASSOC )
    assoc = AssocRight;
  if( iassoc == PARSERTOK_NONASSOC )
    assoc = AssocNon;
  toks->discard(toks);
  ProductionDescriptors *p = ParseProductions(toks,parser);
  checkProductionDescriptorsSameNonterminal(toks,p,-1,"associative rule");
  AssociativeRule *r = (AssociativeRule*)zalloc(sizeof(AssociativeRule));
  AssociativeRule_set(r,assoc,p);
  return r;
}

static PrecedenceRule *ParsePrecedenceRule(Tokenizer *toks, ParserDef *parser) {
  if( toks->peek(toks) != PARSERTOK_PRECEDENCE )
    return 0;
  toks->discard(toks);
  PrecedenceRule *rule = (PrecedenceRule*)zalloc(sizeof(PrecedenceRule));
  PrecedenceRule_init(rule,false);
  ProductionDescriptors *part = ParseProductions(toks,parser);
  checkProductionDescriptorsSameNonterminal(toks,part,-1,"precedence rule group");
  PrecedenceRule_push_back(rule,part);
  while( toks->peek(toks) == PARSERTOK_GT ) {
    toks->discard(toks);
    ProductionDescriptors *part = ParseProductions(toks,parser);
    checkProductionDescriptorsSameNonterminal(toks,part,-1,"precedence rule group");
    PrecedenceRule_push_back(rule,part);
  }
  return rule;
}

static ProductionDescriptors *ParseProductions(Tokenizer *toks, ParserDef *parser) {
  VectorAny /*<int>*/ symbols;
  Scope_Push();
  VectorAny_init(&symbols, getIntElement(),true);
  if( toks->peek(toks) != PARSERTOK_LBRKT )
    error(toks,"Expected '[' to start production description");
  ProductionDescriptors *ret = (ProductionDescriptors*)zalloc(sizeof(ProductionDescriptors));
  ProductionDescriptors_init(ret,false);
  while( toks->peek(toks) == PARSERTOK_LBRKT ) {
    toks->discard(toks);
    int nt = ParseNonterminal(toks,parser);
    if( nt == -1 )
      error(toks,"expected nonterminal");
    if( toks->peek(toks) != PARSERTOK_COLON )
      error(toks,"Expected ':' in production descriptor");
    toks->discard(toks);
    int symbol = ParseSymbol(toks,parser);
    if( symbol == -1 )
      error(toks,"expected symbol");
    VectorAny_clear(&symbols);
    while( symbol != -1 ) {
      VectorAny_push_back(&symbols,&symbol);
      symbol = ParseSymbol(toks,parser);
    }
    if( toks->peek(toks) != PARSERTOK_RBRKT )
      error(toks,"Expected ']' to end production description");
    toks->discard(toks);
    ProductionDescriptor pd;
    Scope_Push();
    ProductionDescriptor_init(&pd,true);
    ProductionDescriptors_insert(ret,ProductionDescriptor_setinfo(&pd,nt,&symbols),0);
    Scope_Pop();
  }
  Scope_Pop();
  return ret;
}

static bool ParseRepeat(Tokenizer *toks, int *low, int *high);
static ProductionRegex *ParseRxSimple(Tokenizer *toks, ParserDef *parser);
static ProductionRegex *ParseRxMany(Tokenizer *toks, ParserDef *parser);
static ProductionRegex *ParseRxConcat(Tokenizer *toks, ParserDef *parser);
static ProductionRegex *ParseRxOr(Tokenizer *toks, ParserDef *parser);
static ProductionRegex *ParseRx(Tokenizer *toks, ParserDef *parser);
static ProductionRegex *ParseRxSimple(Tokenizer *toks, ParserDef *parser);

static ProductionRegex *rxpd(ProductionDescriptor *pd) {
  ProductionRegex *rx = (ProductionRegex*)zalloc(sizeof(ProductionRegex));
  ProductionRegex_init(rx, false);
  rx->m_t = RxType_Production;
  rx->m_pd = pd;
  return rx;
}

static ProductionRegex *rxbinary(BinaryOp op, ProductionRegex *lhs, ProductionRegex *rhs) {
  if (!lhs && !rhs)
    return 0;
  if (!lhs)
    return rhs;
  if (!rhs)
    return lhs;
  ProductionRegex *rx = (ProductionRegex*)zalloc(sizeof(ProductionRegex));
  ProductionRegex_init(rx, false);
  rx->m_t = RxType_BinaryOp;
  rx->m_op = op;
  rx->m_lhs = lhs;
  rx->m_rhs = rhs;
  return rx;
}

static ProductionRegex *rxmany(ProductionRegex *lhs, int low, int high) {
  ProductionRegex *rx = (ProductionRegex*)zalloc(sizeof(ProductionRegex));
  ProductionRegex_init(rx, false);
  rx->m_t = RxType_Many;
  rx->m_lhs = lhs;
  rx->m_low = low;
  rx->m_high = high;
  return rx;
}

static bool ParseRepeat(Tokenizer *toks, int *plow, int *phigh) {
  int low = -1, high = -1;
  if (toks->peek(toks) != PARSERTOK_LT)
    return false;
  toks->discard(toks);
  if (toks->peek(toks) == PARSERTOK_INT) {
    String s;
    Scope_Push();
    String_init(&s, true);
    toks->tokstr(toks, &s);
    low = atoi(String_Chars(&s));
    Scope_Pop();
    toks->discard(toks);
  }
  if (toks->peek(toks) == PARSERTOK_COMMA) {
    toks->discard(toks);
    if (toks->peek(toks) == PARSERTOK_INT) {
      String s;
      Scope_Push();
      String_init(&s, true);
      toks->tokstr(toks, &s);
      high = atoi(String_Chars(&s));
      Scope_Pop();
      toks->discard(toks);
    }
  }
  else
    high = low;
  if (toks->peek(toks) != PARSERTOK_GT)
    error(toks, "expected closing '>' on range");
  if (low == high && low == -1)
    error(toks, "no range values provided");
  *plow = low;
  *phigh = high;
  toks->discard(toks);
  return true;
}

static ProductionRegex *ParseRxSimple(Tokenizer *toks, ParserDef *parser) {
  int c = toks->peek(toks);

  if( c == -1 || c == PARSERTOK_SEMI || c == PARSERTOK_PLUS || c == PARSERTOK_STAR || c == PARSERTOK_QUESTION || c == PARSERTOK_RPAREN || c == PARSERTOK_ARROW || c == PARSERTOK_NOTARROW )
    return 0;

  if( c == PARSERTOK_LPAREN ) {
    toks->discard(toks);
    ProductionRegex *r = ParseRx(toks,parser);
    c = toks->peek(toks);
    if( c != PARSERTOK_RPAREN )
      error(toks, "unbalanced '('");
    toks->discard(toks);
    r->m_paren = true;
    return r;
  } else {
    ProductionDescriptor *pd = ParseProductionDescriptor(toks, parser, 0);
    return rxpd(pd);
  }

  error(toks, "parse error");
  return 0;
}

static ProductionRegex *ParseRxMany(Tokenizer *toks, ParserDef *parser) {
  ProductionRegex *r = 0;
  int low, high;
  r = ParseRxSimple(toks,parser);
  if (!r)
    return 0;
  int c = toks->peek(toks);

  while (c != -1) {
    if (c == PARSERTOK_PLUS) {
      r = rxmany(r, 1, -1);
      toks->discard(toks);
      c = toks->peek(toks);
    }
    else if (c == PARSERTOK_STAR) {
      r = rxmany(r, 0, -1);
      toks->discard(toks);
      c = toks->peek(toks);
    }
    else if (c == PARSERTOK_QUESTION) {
      r = rxmany(r, 0, 1);
      toks->discard(toks);
      c = toks->peek(toks);
    }
    else if (ParseRepeat(toks, &low, &high)) {
      r = rxmany(r, low, high);
      c = toks->peek(toks);
    }
    else {
      break;
    }
  }
  return r;
}

static ProductionRegex *ParseRxConcat(Tokenizer *toks, ParserDef *parser) {
  ProductionRegex *lhs = ParseRxMany(toks,parser);
  if (!lhs)
    return 0;
  int c = toks->peek(toks);
  while( c != -1 && c != PARSERTOK_SEMI && c != PARSERTOK_VSEP ) {
    ProductionRegex *rhs = ParseRxMany(toks,parser);
    if (!rhs)
      break;
    lhs = rxbinary(BinaryOp_Concat, lhs, rhs);
    c = toks->peek(toks);
  }
  return lhs;
}

static ProductionRegex *ParseRxOr(Tokenizer *toks, ParserDef *parser) {
  ProductionRegex *lhs = ParseRxConcat(toks,parser);
  if (!lhs)
    return 0;
  int c = toks->peek(toks);
  while (c == PARSERTOK_VSEP) {
    toks->discard(toks);
    ProductionRegex *rhs = ParseRxConcat(toks,parser);
    lhs = rxbinary(BinaryOp_Or, lhs, rhs);
    c = toks->peek(toks);
  }
  return lhs;
}

static ProductionRegex *ParseRx(Tokenizer *toks, ParserDef *parser) {
  return ParseRxOr(toks,parser);
}

static RestrictRule *ParseRestrictRule(Tokenizer *toks, ParserDef *parser) {
  RestrictRule *rr = (RestrictRule*)zalloc(sizeof(RestrictRule));
  rr->m_restriction = (Restriction*)zalloc(sizeof(Restriction));
  Restriction_init(rr->m_restriction,false);
  if( toks->peek(toks) != PARSERTOK_RESTRICT )
    return 0;
  toks->discard(toks);
  if( toks->peek(toks) != PARSERTOK_ARROW )
    rr->m_rx = ParseRx(toks, parser);
  if( toks->peek(toks) != PARSERTOK_ARROW )
    error(toks,"Expected -->");
  toks->discard(toks);
  ParseProductionDescriptors(toks, parser, &rr->m_restriction->m_at);
  if( toks->peek(toks) != PARSERTOK_NOTARROW )
    error(toks,"Expected -/->");
  toks->discard(toks);
  ParseProductionDescriptors(toks, parser, &rr->m_restriction->m_restricted);
  return rr;
}

static bool ParseParsePart(Tokenizer *toks, ParserDef *parser) {
  if( toks->peek(toks) == PARSERTOK_TYPEDEFTOK ) {
    ParseTypedefRule(toks,parser);
  } else if( toks->peek(toks) == PARSERTOK_LEFTASSOC || toks->peek(toks) == PARSERTOK_RIGHTASSOC || toks->peek(toks) == PARSERTOK_NONASSOC ) {
    AssociativeRule *ar = ParseAssociativeRule(toks,parser);
    VectorAny_push_back(&parser->m_assocrules,&ar);
  } else if( toks->peek(toks) == PARSERTOK_PRECEDENCE ) {
    PrecedenceRule *p = ParsePrecedenceRule(toks,parser);
    VectorAny_push_back(&parser->m_precrules,&p);
  } else if( toks->peek(toks) == PARSERTOK_RESTRICT ) {
    RestrictRule *rr = ParseRestrictRule(toks,parser);
    ParserDef_addRestrictRule(parser,rr);
  } else if( toks->peek(toks) == PARSERTOK_REJECTABLE  || toks->peek(toks) == PARSERTOK_START || toks->peek(toks) == PARSERTOK_ID ) {
    VectorAny /*<Production*>*/ productions;
    Scope_Push();
    VectorAny_init(&productions,getPointerElement(),true);
    ParseProduction(toks,parser,&productions);
    for( int cur = 0, end = VectorAny_size(&productions); cur != end; ++cur )
      ParserDef_addProduction(parser,toks,VectorAny_ArrayOpT(&productions,cur,Production*));
    Scope_Pop();
  } else
    return false;
  if( toks->peek(toks) != PARSERTOK_SEMI )
    error(toks,"Expected ';'");
  toks->discard(toks);
  return true;
}

static void ParseStart(Tokenizer *toks, ParserDef *parser) {
  if( ! ParseParsePart(toks,parser) )
    error(toks,"expected production, precedence rule, or restrict rule");
  while( true ) {
    if( ! ParseParsePart(toks,parser) )
      break;
  }
}

void ParseParser(TokBuf *tokbuf, ParserDef *parser, FILE *vout, int verbosity) {
  TokenInfo parsertok_pptokinfo;
  parsertok_pptokinfo.m_tokenCount       = PARSERTOK_tokenCount;
  parsertok_pptokinfo.m_sectionCount     = PARSERTOK_sectionCount;
  parsertok_pptokinfo.m_tokenaction      = PARSERTOK_tokenaction;
  parsertok_pptokinfo.m_tokenstr         = PARSERTOK_tokenstr;
  parsertok_pptokinfo.m_isws             = PARSERTOK_isws;
  parsertok_pptokinfo.m_stateCount       = PARSERTOK_stateCount;
  parsertok_pptokinfo.m_transitions      = PARSERTOK_transitions;
  parsertok_pptokinfo.m_transitionOffset = PARSERTOK_transitionOffset;
  parsertok_pptokinfo.m_tokens           = PARSERTOK_tokens;

  TokBufTokenizer toks;
  Scope_Push();
  TokBufTokenizer_init(&toks,tokbuf,&parsertok_pptokinfo,true);
  ParserDef_findOrAddSymbolIdChars(parser,&toks.m_tokenizer,"START",SymbolTypeNonterminal);
  ParserDef_findOrAddSymbolIdChars(parser,&toks.m_tokenizer,"PARSE_ERROR",SymbolTypeNonterminal);
  ParseStart(&toks.m_tokenizer,parser);
  if( toks.m_tokenizer.peek(&toks.m_tokenizer) != -1 )
    error(&toks.m_tokenizer,"Expected EOF");
  for( int cur = 0, end = MapAny_size(&parser->m_tokdefs); cur != end; ++cur ) {
    int *tok = 0;
    SymbolDef *curdef = 0;
    MapAny_getByIndex(&parser->m_tokdefs,cur,(const void**)&tok,(void**)&curdef);
    if( curdef->m_symboltype == SymbolTypeUnknown )
      curdef->m_symboltype = SymbolTypeTerminal;
  }
  Scope_Pop();
}

void state_t_print(const state_t *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens) {
  for( int cur = 0, end = SetAny_size(&This->m_productionStates); cur != end; ++cur ) {
    const ProductionState *curps = &SetAny_getByIndexConstT(&This->m_productionStates,cur,ProductionState);
    Production_print(curps->m_p,out,tokens,curps->m_pos,curps->m_restrictstate);
    fputs("\n",out);
  }
}

void state_t_init(state_t *This, bool onstack) {
  SetAny_init(&This->m_productionStates,&ProductionStateElement,onstack);
}

void state_t_destroy(state_t *This) {
  SetAny_destroy(&This->m_productionStates);
}

bool state_t_LessThan(const state_t *lhs, const state_t *rhs) {
  return SetAny_LessThan(&lhs->m_productionStates,&rhs->m_productionStates);
}

bool state_t_Equal(const state_t *lhs, const state_t *rhs) {
  return SetAny_Equal(&lhs->m_productionStates,&rhs->m_productionStates);
}

void state_t_Assign(state_t *lhs, const state_t *rhs) {
  SetAny_Assign(&lhs->m_productionStates,&rhs->m_productionStates);
}

bool productionandrestrictstate_t_LessThan(const productionandrestrictstate_t *lhs, const productionandrestrictstate_t *rhs) {
  if( lhs->production->m_pid < rhs->production->m_pid )
    return true;
  else if( lhs->production->m_pid > rhs->production->m_pid )
    return false;
  return lhs->restrictstate < rhs->restrictstate;
}

bool productionandrestrictstate_t_Equal(const productionandrestrictstate_t *lhs, const productionandrestrictstate_t *rhs) {
  return lhs->production->m_pid == rhs->production->m_pid && lhs->restrictstate == rhs->restrictstate;
}

