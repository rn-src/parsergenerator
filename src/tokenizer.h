#ifndef __tokenizer_h
#define __tokenizer_h

#include <stdio.h>
#include <stdint.h>
#include "tinytemplates.h"
#include "parsecommon.h"
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct TokStream;
typedef struct TokStream TokStream;

void TokStream_init(TokStream *This, FILE *in, const char *file, bool onstack);
void TokStream_destroy(TokStream *This);
bool TokStream_fill(TokStream *This);
int TokStream_peekc(TokStream *This, int n /*= 0*/);
bool TokStream_peekstr(TokStream *This, const char *s);
int TokStream_readc(TokStream *This);
void TokStream_discard(TokStream *This, int n /*=1*/);
int TokStream_line(TokStream *This);
int TokStream_col(TokStream *This);
const char *TokStream_file(TokStream *This);

struct TokStream {
  char *m_buf, *m_next;
  size_t m_buflen;
  size_t m_buffill, m_bufpos, m_pos, m_line, m_col;
  FILE *m_in;
  char *m_file;
};

struct CharRange;
typedef struct CharRange CharRange;
CharRange *CharRange_SetRange(CharRange *This, uint32_t low, uint32_t high);
bool CharRange_ContainsChar(const CharRange *This, uint32_t i);
bool CharRange_ContainsCharRange(const CharRange *This, const CharRange *rhs);
bool CharRange_OverlapsRange(const CharRange *This, uint32_t low, uint32_t high);
bool CharRange_LessThan(const CharRange *lhs, const CharRange *rhs);

struct CharRange {
  uint32_t m_low, m_high;
};

struct CharSet;
typedef struct CharSet CharSet;

void CharSet_init(CharSet *This, bool onstack);
void CharSet_destroy(CharSet *This);
bool CharSet_LessThan(const CharSet *lhs, const CharSet *rhs);
bool CharSet_Equal(const CharSet *lhs, const CharSet *rhs);
void CharSet_Assign(CharSet *lhs, const CharSet *rhs);
bool CharSet_ContainsCharRange(const CharSet *This, const CharRange *range);
bool CharSet_has(const CharSet *This, uint32_t c);
void CharSet_clear(CharSet *This);
bool CharSet_empty(const CharSet *This);
void CharSet_addChar(CharSet *This, uint32_t i);
void CharSet_addCharRange(CharSet *This, uint32_t low, uint32_t high);
void CharSet_addCharSet(CharSet *This, const CharSet *rhs);
void CharSet_negate(CharSet *This);
int CharSet_size(const CharSet *This);
const CharRange *CharSet_getRange(const CharSet *This, int i);
typedef void (*ComboBreakerCB)(const CharSet *charset, VectorAny /*<int>*/ *charset_indexes, void *vpcb);
void CharSet_combo_breaker(const VectorAny /*<CharSet>*/ *charsets, ComboBreakerCB cb, void *vpcb);

struct CharSet {
  VectorAny /*CharRange*/ m_ranges;
};

struct Transition;
typedef struct Transition Transition;

void Transition_init(Transition *This, bool onstack);
Transition *Transition_SetFromTo(Transition *This, int from, int to);
bool Transition_LessThan(const Transition *lhs, const Transition *rhs);
bool Transition_Equal(const Transition *lhs, const Transition *rhs);

struct Transition {
  int m_from, m_to;
};

enum TokenAction {
  ActionNone,
  ActionPush,
  ActionPop,
  ActionGoto
};
typedef enum TokenAction TokenAction;

struct Action;
typedef struct Action Action;

struct Action {
  TokenAction m_action;
  int m_actionarg;
};

struct Token;
typedef struct Token Token;

void Token_init(Token *This, bool onstack);
void Token_destroy(Token *This);
bool Token_LessThan(const Token *lhs, const Token *rhs);
bool Token_Equal(const Token *lhs, const Token *rhs);
void Token_Assign(Token *lhs, const Token *rhs);

struct Token {
  int m_token;
  bool m_isws;
  String m_name;
  MapAny /*<int,Action>*/ m_actions;
};

struct Nfa;
typedef struct Nfa Nfa;

void Nfa_init(Nfa *This, bool onstack);
void Nfa_setTokenDef(Nfa *This, const Token *tok);
int Nfa_stateCount(const Nfa *This);
int Nfa_addState(Nfa *This);
void Nfa_addStartState(Nfa *This, int startstate);
void Nfa_addEndState(Nfa *This, int endstate);
void Nfa_addTransition(Nfa *This, int from, int to, int symbol);
void Nfa_addTransitionCharSet(Nfa *This, int from, int to, const CharSet *charset);
void Nfa_addEmptyTransition(Nfa *This, int from, int to);
bool Nfa_hasTokenDef(const Nfa *This, int token);
void Nfa_setTokenDef(Nfa *This, const Token *tok);
bool Nfa_setTokenAction(Nfa *This, int token, int section, TokenAction action, int actionarg);
int Nfa_getSections(const Nfa *This);
void Nfa_setSections(Nfa *This, int sections);
const Token *Nfa_getTokenDef(const Nfa *This, int token);
bool Nfa_stateHasToken(const Nfa *This, int state);
int Nfa_getStateToken(const Nfa *This, int state);
void Nfa_setStateToken(Nfa *This, int state, int token);
void Nfa_clear(Nfa *This);
void Nfa_closure(const MapAny /*<int,Set<int>>*/ *emptytransitions, SetAny /*<int>*/ *states);
void Nfa_stateTransitions(const Nfa *This, const SetAny /*<int>*/ *states, VectorAny /*<int>*/ *transitions, VectorAny /*<CharSet>*/ *transition_symbols);
bool Nfa_hasEndState(const Nfa *This, const SetAny /*<int>*/ *states);
int Nfa_lowToken(const Nfa *This, const SetAny /*<int>*/ *states);
void Nfa_toDfa_NonMinimal(const Nfa *This, Nfa *dfa);
void Nfa_Reverse(Nfa *This, bool reverse_numbers);
void Nfa_toDfa(const Nfa *This, Nfa *dfa);

struct Nfa {
  int m_nextState;
  SetAny /*<int>*/ m_startStates;
  SetAny /*<int>*/ m_endStates;
  MapAny /*<Transition,CharSet>*/ m_transitions;
  SetAny /*<Transition>*/ m_emptytransitions;
  MapAny /*<int,int>*/ m_state2token;
  MapAny /*<int,Token>*/ m_tokendefs;
  int m_sections;
};

struct Rx;
typedef struct Rx Rx;

void Rx_init(Rx *This, bool onstack);
Rx *Rx_new();
void Rx_AsC(Rx *This, char c);
void Rx_AsCharSet(Rx *This, const CharSet *charset);
void Rx_AsBinaryOp(Rx *This, BinaryOp op,Rx *lhs, Rx *rhs);
void Rx_AsRxCount(Rx *This, Rx *rx, int count);
void Rx_AsRxCountRange(Rx *This, Rx *rx, int low, int high);
Rx *Rx_clone(const Rx *This);
int Rx_addToNfa(const Rx *This, Nfa *nfa, int startState);
Nfa *Rx_toNfa(const Rx *This);

struct Rx {
  RxType m_t;
  BinaryOp m_op;
  CharSet m_charset;
  Rx *m_lhs, *m_rhs;
  int m_low, m_high;
};

Nfa *ParseTokenizerFile(TokStream *s, bool verbose);
void OutputTokenizerSource(FILE *out, const Nfa *dfa, LanguageOutputOptions *options);
ElementOps *getTokenElement();
ElementOps *getCharSetElement();
void Nfa_getTokenDefs(const Nfa *This, VectorAny /*<Token>*/ *tokendefs);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __tokenizer_h */
