#ifndef __parser_h
#define __parser_h

#include "../tok/tok.h"
#include "../tok/tinytemplates.h"
#define EOF_TOK (-1)
#define MARKER_DOT (999999)

struct ProductionRegex;
typedef struct ProductionRegex ProductionRegex;
struct ParserDef;
typedef struct ParserDef ParserDef;
struct LRParserSolution;
typedef struct LRParserSolution LRParserSolution;
struct LLParserSolution;
typedef struct LLParserSolution LLParserSolution;
struct Production;
typedef struct Production Production;
struct RestrictAutomata;
typedef struct RestrictAutomata RestrictAutomata;

struct ParseError;
typedef struct ParseError ParseError;

void setParseError(int line, int col, const String *err);
bool getParseError(const ParseError **err);
void clearParseError();

struct ParseError
{
  int line, col;
  String file;
  String err;
};

enum SymbolType {
  SymbolTypeUnknown,
  SymbolTypeTerminal,
  SymbolTypeNonterminal,
  SymbolTypeNonterminalProduction
};
typedef enum SymbolType SymbolType;

struct SymbolDef;
typedef struct SymbolDef SymbolDef;

void SymbolDef_init(SymbolDef *This, bool onstack);
void SymbolDef_destroy(SymbolDef *This);
bool SymbolDef_LessThan(const SymbolDef *lhs, const SymbolDef *rhs);
bool SymbolDef_Equal(const SymbolDef *lhs, const SymbolDef *rhs);
void SymbolDef_Assign(SymbolDef *lhs, const SymbolDef *rhs);
SymbolDef *SymbolDef_setinfo(SymbolDef *This, int tokid, const String *name, SymbolType symbolType);

struct SymbolDef {
  int m_tokid;
  String m_name;
  SymbolType m_symboltype;
  String m_semantictype;
  Production *m_p;
};

enum ActionType {
  ActionTypeUnknown,
  ActionTypeSrc,
  ActionTypeDollarDollar,
  ActionTypeDollarNumber
};
typedef enum ActionType ActionType;

struct ActionItem;
typedef struct ActionItem ActionItem;

void ActionItem_init(ActionItem *This, bool onstack);
void ActionItem_destroy(ActionItem *This); 
void ActionItem_Assign(ActionItem *lhs, const ActionItem *rhs);

struct ActionItem {
  ActionType m_actiontype;
  String m_str;
  int m_dollarnum;
};

struct Production;
typedef struct Production Production;

void Production_init(Production *This, bool rejectable, int nt, const VectorAny /*<int>*/ *symbols, const VectorAny /*<ActionItem>*/ *action, int lineno, const String *filename, bool onstack);
//void Production_init(Production *This);
void Production_destroy(Production *This);
//void Production_setInfo(Production *This, bool rejectable, int nt, const VectorAny /*<int>*/ *symbols, const VectorAny /*<ActionItem>*/ *action, int lineno, const String *filename);
Production *Production_clone(const Production *This);
bool Production_Equal(const Production *lhs, const Production *rhs);
bool Production_NotEqual(const Production *lhs, const Production *rhs);
bool Production_LessThan(const Production *lhs, const Production *rhs);
void Production_print(const Production *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens, int pos /*=-1*/, int restrictstate /*=0*/);
// for use in qsort
int Production_CompareId(const void *lhs, const void *rhs);

struct Production {
  bool m_rejectable;
  int m_nt;
  int m_pid;
  VectorAny /*<int>*/ m_symbols;
  VectorAny /*<ActionItem>*/ m_action;
  int m_lineno;
  String m_filename;
};

struct ProductionState;
typedef struct ProductionState ProductionState;

ProductionState *ProductionState_set(ProductionState *This, const Production *p, int pos, int restrictstate);
bool ProductionState_LessThan(const ProductionState *lhs, const ProductionState *rhs);
bool ProductionState_Equal(const ProductionState *lhs, const ProductionState *rhs);
int ProductionState_symbol(const ProductionState *This);

struct ProductionState {
  const Production *m_p;
  int m_pos;
  int m_restrictstate;
};

enum Assoc {
  AssocNull,
  AssocLeft,
  AssocRight,
  AssocNon
};
typedef enum Assoc Assoc;

struct ProductionDescriptor;
typedef struct ProductionDescriptor ProductionDescriptor;

void ProductionDescriptor_init(ProductionDescriptor *This, bool onstack);
void ProductionDescriptor_destroy(ProductionDescriptor *This);
ProductionDescriptor *ProductionDescriptor_setinfo(ProductionDescriptor *This, int nt, const VectorAny /*<int>*/ *symbols);
void ProductionDescriptor_setInfoWithDots(ProductionDescriptor *This, int nt, const VectorAny /*<int>*/ *symbols, int dots, int dotcnt);
bool ProductionDescriptor_hasDotAt(const ProductionDescriptor *This, int pos);
bool ProductionDescriptor_matchesProduction(const ProductionDescriptor *This, const Production *p);
bool ProductionDescriptor_matchesProductionAndPosition(const ProductionDescriptor *This, const Production *p, int pos);
// Check if, ignoring dots, the productions are the same
bool ProductionDescriptor_hasSameProductionAs(const ProductionDescriptor *This, const ProductionDescriptor *rhs);
// Add dots from rhs to this descriptor.  Fail if not same production.
bool ProductionDescriptor_addDotsFrom(ProductionDescriptor *This, const ProductionDescriptor *rhs);
void ProductionDescriptor_addDots(ProductionDescriptor *This, int dots);
void ProductionDescriptor_removeDots(ProductionDescriptor *This, int dots);
int ProductionDescriptor_appearancesOf(const ProductionDescriptor *This, int sym, int *at);
bool ProductionDescriptor_LessThan(const ProductionDescriptor *lhs, const ProductionDescriptor *rhs);
void ProductionDescriptor_Assign(ProductionDescriptor *lhs, const ProductionDescriptor *rhs);
ProductionDescriptor *ProductionDescriptor_clone(const ProductionDescriptor *This);
void ProductionDescriptor_UnDottedProductionDescriptor(const ProductionDescriptor *This, ProductionDescriptor *out);
void ProductionDescriptor_DottedProductionDescriptor(const ProductionDescriptor *This, ProductionDescriptor *out, int nt, Assoc assoc);
void ProductionDescriptor_symbolsAtDots(const ProductionDescriptor *This, VectorAny /*<int>*/ *symbols);
bool ProductionDescriptor_Equal(const ProductionDescriptor *lhs, const ProductionDescriptor *rhs);
bool ProductionDescriptor_NotEqual(const ProductionDescriptor *lhs, const ProductionDescriptor *rhs);
void ProductionDescriptor_print(const ProductionDescriptor *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens);
void ProductionDescriptor_clear(ProductionDescriptor *This);

struct ProductionDescriptor {
  VectorAny /*<int>*/ m_symbols;
  int m_dots;
  int m_dotcnt;
  int m_nt;
};

struct ProductionDescriptors;
typedef struct ProductionDescriptors ProductionDescriptors;

void ProductionDescriptors_init(ProductionDescriptors *This, bool onstack);
void ProductionDescriptors_destroy(ProductionDescriptors *This);
ProductionDescriptors *ProductionDescriptors_UnDottedProductionDescriptors(const ProductionDescriptors *This, ProductionDescriptors *psundottedOut);
ProductionRegex *ProductionDescriptors_ProductionRegex(const ProductionDescriptors *This, Assoc assoc);
ProductionDescriptors *ProductionDescriptors_clone(const ProductionDescriptors *This);
void ProductionDescriptors_print(const ProductionDescriptors *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens);
bool ProductionDescriptors_LessThan(const ProductionDescriptors *lhs, const ProductionDescriptors *rhs);
bool ProductionDescriptors_Equal(const ProductionDescriptors *lhs, const ProductionDescriptors *rhs);
bool ProductionDescriptors_NotEqual(const ProductionDescriptors *lhs, const ProductionDescriptors *rhs);
void ProductionDescriptors_Assign(ProductionDescriptors *lhs, const ProductionDescriptors *rhs);
bool ProductionDescriptors_matchesProduction(const ProductionDescriptors *This, const Production *lhs);
bool ProductionDescriptors_matchesProductionAndPosition(const ProductionDescriptors *This, const Production *lhs, int pos);
//#define ProductionDescriptors_getByIndex(This,i) SetAny_getByIndexT(&((This)->m_productionDescriptors),i,ProductionDescriptor);
#define ProductionDescriptors_getByIndexConst(This,i) SetAny_getByIndexConstT(&((This)->m_productionDescriptors),i,ProductionDescriptor)
#define ProductionDescriptors_insert(This,p,pfound) SetAny_insert(&((This)->m_productionDescriptors),p,pfound)
#define ProductionDescriptors_insertMany(This,p,count) SetAny_insertMany(&((This)->m_productionDescriptors),p,count)
#define ProductionDescriptors_ptr(This) SetAny_ptr(&((This)->m_productionDescriptors))
#define ProductionDescriptors_ptrConst(This) SetAny_ptrConst(&((This)->m_productionDescriptors))
#define ProductionDescriptors_size(This) SetAny_size(&((This)->m_productionDescriptors))

struct ProductionDescriptors {
  SetAny /*<ProductionDescriptor>*/ m_productionDescriptors;
};

struct PrecedenceRule;
typedef struct PrecedenceRule PrecedenceRule;

void PrecedenceRule_init(PrecedenceRule *This, bool onstack);
void PrecedenceRule_destroy(PrecedenceRule *This);
void PrecedenceRule_print(const PrecedenceRule *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens);
#define PrecedenceRule_size(This) VectorAny_size((VectorAny*)This)
#define PrecedenceRule_ArrayOp(This,i) VectorAny_ArrayOpT(&This->m_productionDescriptors,i,ProductionDescriptors*)
#define PrecedenceRule_ArrayOpConst(This,i) VectorAny_ArrayOpConstT(&This->m_productionDescriptors,i,ProductionDescriptors*)
void PrecedenceRule_push_back(PrecedenceRule *This, ProductionDescriptors *part);

struct PrecedenceRule {
  VectorAny /*<ProductionDescriptors*>*/ m_productionDescriptors;
};

enum RxType {
  RxType_None = 0,
  RxType_Production = 1,
  RxType_BinaryOp = 2,
  RxType_Many = 3
};
typedef enum RxType RxType;

enum BinaryOp {
  BinaryOp_None = 0,
  BinaryOp_Or = 1,
  BinaryOp_Concat = 2
};
typedef enum BinaryOp BinaryOp;

void ProductionRegex_init(ProductionRegex *This, bool onstack);
void ProductionRegex_destroy(ProductionRegex *This);
int RestrictRegex_addToNfa(ProductionRegex *This, RestrictAutomata *nrestrict, int startState);
void ProductionRegex_print(ProductionRegex *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens);

struct ProductionRegex {
  RxType m_t;
  BinaryOp m_op;
  ProductionDescriptor *m_pd;
  ProductionRegex *m_lhs, *m_rhs;
  int m_low, m_high;
  bool m_paren;
};

struct RestrictRule;
typedef struct RestrictRule RestrictRule;

void RestrictRule_print(const RestrictRule *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens);

struct RestrictRule {
  ProductionRegex *m_rx;
  ProductionDescriptors *m_restricted;
};

struct AssociativeRule;
typedef struct AssociativeRule AssociativeRule;

AssociativeRule *AssociativeRule_set(AssociativeRule *This, Assoc assoc, ProductionDescriptors *pds);
void AssociativeRule_print(const AssociativeRule *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens);

struct AssociativeRule {
  Assoc m_assoc;
  ProductionDescriptors *m_pds;
};

struct Restriction;
typedef struct Restriction Restriction;

void Restriction_init(Restriction *This, bool onstack);
void Restriction_destroy(Restriction *This);
bool Restriction_LessThan(const Restriction *lhs, const Restriction *rhs);
bool Restriction_Equal(const Restriction *lhs, const Restriction *rhs);
void Restriction_Assign(Restriction *lhs, const Restriction *rhs);
void Restriction_clear(Restriction *This);
void Restriction_print(const Restriction *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens);

struct Restriction {
  ProductionDescriptor m_restricted;
  ProductionDescriptor m_at;
};

void RestrictAutomata_init(RestrictAutomata *This, bool onstack);
void RestrictAutomata_destroy(RestrictAutomata *This);
void RestrictAutomata_Assign(RestrictAutomata *lhs, const RestrictAutomata *rhs);
int RestrictAutomata_newstate(RestrictAutomata *This);
int RestrictAutomata_nstates(const RestrictAutomata *This);
void RestrictAutomata_addEndState(RestrictAutomata *This, int stateNo);
void RestrictAutomata_addEmptyTransition(RestrictAutomata *This, int s0, int s1);
void RestrictAutomata_addTransition(RestrictAutomata *This, int s0, int s1, const Restriction *restriction);
void RestrictAutomata_addMultiStateTransition(RestrictAutomata *This, int s0, const SetAny /*<int>*/ *s1, const Restriction *restriction);
void RestrictAutomata_addRestriction(RestrictAutomata *This, int stateNo, const Restriction *restriction);
// Find *the* state after the current state, assuming this is a deterministic automata
int RestrictAutomata_nextState(const RestrictAutomata *This, const Production *curp, int pos, int stateno, const Production *p);
bool RestrictAutomata_isRestricted(const RestrictAutomata *This, const Production *p, int pos, int restrictstate, const Production *ptest);
void RestrictAutomata_closure(const RestrictAutomata *This, SetAny /*<int>*/ *multistate);
void RestrictAutomata_RestrictsFromStates(const RestrictAutomata *This, const SetAny /*<int>*/ *stateset, SetAny /*<Restriction*>*/ *restrictions);
void RestrictAutomata_SymbolsFromStates(const RestrictAutomata *This, const SetAny /*<int>*/ *stateset, SetAny /*<Restriction>*/ *symbols);
void RestrictAutomata_toDeterministicRestrictAutomata(const RestrictAutomata *This, RestrictAutomata *out);
void RestrictAutomata_print(const RestrictAutomata *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens);

struct RestrictAutomata {
  int m_nextstate;
  int m_startState;
  SetAny /* <int> */ m_endStates;
  MapAny /* <int,Set<int> >*/ m_emptytransitions;
  MapAny /* <int,Map< Restriction, Set<int> > >*/ m_transitions;
  MapAny /* <int,Set< Restriction > > */ m_statetorestrictions;
};

struct productionandrestrictstate_t;
typedef struct productionandrestrictstate_t productionandrestrictstate_t;

productionandrestrictstate_t *productionandrestrictstate_t_set(productionandrestrictstate_t *This, const Production *production, int restrictstate);
bool productionandrestrictstate_t_LessThan(const productionandrestrictstate_t *lhs, const productionandrestrictstate_t *rhs);
bool productionandrestrictstate_t_Equal(const productionandrestrictstate_t *lhs, const productionandrestrictstate_t *rhs);

struct productionandrestrictstate_t {
  const Production *production;
  int restrictstate;
};

struct ProductionsAtKey;
typedef struct ProductionsAtKey ProductionsAtKey;

bool ProductionsAtKey_LessThan(const ProductionsAtKey *lhs, const ProductionsAtKey *rhs);
bool ProductionsAtKey_Equal(const ProductionsAtKey *lhs, const ProductionsAtKey *rhs);
bool ProductionsAtKey_NotEqual(const ProductionsAtKey *lhs, const ProductionsAtKey *rhs);

struct ProductionsAtKey {
  const Production *m_p;
  int m_pos;
  int m_restrictstate;
};

struct ParserDef;
typedef struct ParserDef ParserDef;

void ParserDef_init(ParserDef *This, FILE *vout, int verbosity, bool onstack);
void ParserDef_destroy(ParserDef *This);
void ParserDef_addProduction(ParserDef *This, Tokenizer *toks, Production *p);
bool ParserDef_addProductionWithCheck(ParserDef *This, Production *p);
int ParserDef_findSymbolId(const ParserDef *This, const String *s);
int ParserDef_addSymbolId(ParserDef *This, const String *s, SymbolType stype);
int ParserDef_findOrAddSymbolId(ParserDef *This, Tokenizer *toks, const String *s, SymbolType stype);
int ParserDef_getStartNt(const ParserDef *This);
int ParserDef_getExtraNt(const ParserDef *This);
Production *ParserDef_getStartProduction(ParserDef *This);
const Production *ParserDef_getStartProductionConst(const ParserDef *This);
const VectorAny /*<productionandrestrictstate_t>*/ *ParserDef_productionsAt(ParserDef *This, const Production *p, int pos, int restrictstate);
void ParserDef_computeRestrictAutomata(ParserDef *This);
SymbolType ParserDef_getSymbolType(const ParserDef *This, int tok);
void ParserDef_print(const ParserDef *This, FILE *out);
void ParserDef_addRestrictRule(ParserDef *This, RestrictRule *rule);
void ParserDef_addRestrictRx(ParserDef *This, ProductionRegex *rx);
void ParserDef_expandAssocRules(ParserDef *This);
void ParserDef_expandPrecRules(ParserDef *This);
void ParserDef_getProductionsOfNt(const ParserDef *This, int nt, VectorAny /*<Production*>*/ *productions);

struct ParserDef {
  int m_nextsymbolid;
  Production *m_startProduction;
  FILE *m_vout;
  int m_verbosity;
  MapAny /*<String,int>*/ m_tokens;
  MapAny /*<int,SymbolDef>*/ m_tokdefs;
  VectorAny /*<Production*>*/ m_productions;
  VectorAny /*<RestrictRule*>*/ m_restrictrules;
  VectorAny /*<RestrictRule*>*/ m_restrictrxs;
  VectorAny /*<AssociativeRule*>*/ m_assocrules;
  VectorAny /*<PrecedenceRule*>*/ m_precrules;
  RestrictAutomata m_restrict;
  MapAny /*< ProductionsAtKey,Vector<productionandrestrictstate_t> >*/ productionsAtResults;
};

struct ParserError;
typedef struct ParserError ParserError;

struct ParserError {
  int m_line, m_col;
  String m_filename;
  String m_err;
};

struct state_t;
typedef struct state_t state_t;

void state_t_init(state_t *This, bool onstack);
void state_t_destroy(state_t *This);
void state_t_print(const state_t *This, FILE *out, const MapAny /*<int,SymbolDef>*/ *tokens);
bool state_t_LessThan(const state_t *lhs, const state_t *rhs);
bool state_t_Equal(const state_t *lhs, const state_t *rhs);
void state_t_Assign(state_t *lhs, const state_t *rhs);
#define state_t_size(This) SetAny_size(&((This)->m_productionStates))
#define state_t_getByIndexConst(This,i) SetAny_getByIndexConstT(&((This)->m_productionStates),i,ProductionState)
#define state_t_insert(This,p,pfound) SetAny_insert(&((This)->m_productionStates),p,pfound)
#define state_t_empty(This) (SetAny_size(&((This)->m_productionStates)) == 0)
#define state_t_clear(This) SetAny_clear(&((This)->m_productionStates))

struct state_t {
  SetAny /*<ProductionState>*/ m_productionStates;
};

typedef MapAny /*<int,Set<int> >*/ shifttosymbols_t;
typedef MapAny /*<int,shifttosymbols_t>*/ shiftmap_t;
typedef MapAny /*<Production*,Set<int> >*/ reducebysymbols_t;
typedef MapAny /*<int,reducebysymbols_t>*/ reducemap_t;

struct FirstsAndFollows;
typedef struct FirstsAndFollows FirstsAndFollows;

void FirstsAndFollows_init(FirstsAndFollows *This, bool onstack);
void FirstsAndFollows_destroy(FirstsAndFollows *This);
void FirstsAndFollows_Compute(FirstsAndFollows *This, ParserDef *parser, FILE *vout, int verbosity);

struct FirstsAndFollows {
  MapAny /*< productionandrestrictstate_t, Set<int> >*/ m_firsts;
  MapAny /*< productionandrestrictstate_t, Set<int> >*/ m_follows;
};

void LRParserSolution_init(LRParserSolution *This, bool onstack);
void LRParserSolution_destroy(LRParserSolution *This);

struct LRParserSolution {
  FirstsAndFollows m_firstsAndFollows;
  VectorAny /*<state_t>*/ m_states;
  shiftmap_t m_shifts;
  reducemap_t m_reductions;
};

void LLParserSolution_init(LLParserSolution *This, bool onstack);
void LLParserSolution_destroy(LLParserSolution *This);

struct LLParserSolution {
  FirstsAndFollows m_firstsAndFollows;
};

struct LanguageOutputOptions;
typedef struct LanguageOutputOptions LanguageOutputOptions;

enum ParserType {
  ParserType_LR,
  ParserType_LL
};
typedef enum ParserType ParserType;

enum OutputLanguage {
  OutputLanguage_C,
  OutputLanguage_Python
};
typedef enum OutputLanguage OutputLanguage;

struct LanguageOutputOptions {
  int min_nt_value;
  bool do_pound_line;
  ParserType m_parserType;
  OutputLanguage m_outputLanguage;
  const char *m_lexerName;
  const char **m_extraImports;
};

void ParseParser(TokBuf *tokbuf, ParserDef *parser, FILE *vout, int verbosity);
int LR_SolveParser(ParserDef *parser, LRParserSolution *solution, FILE *vout, int verbosity, int timed);
void OutputLRParserSolution(FILE *out, const ParserDef *parser, const LRParserSolution *solution, LanguageOutputOptions *options);
int LL_SolveParser(ParserDef *parser, LLParserSolution *solution, FILE *vout, int verbosity, int timed);
void OutputLLParserSolution(FILE *out, const ParserDef *parser, const LLParserSolution *solution, LanguageOutputOptions *options);

#endif /* __parser_h */

