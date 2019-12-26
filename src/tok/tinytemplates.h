#ifndef __tinytemplates_h
#define __tinytemplates_h
// Some templates so the executable size can shrink

#ifdef __cplusplus
#include <new>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <setjmp.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef void (*vpstack_destroyer)(void *vpstack);
void Push_Destroy(void *vpstack, vpstack_destroyer destroyer);
void Scope_Push();
void Scope_Pop();
#define Scope_SetJmp(ret) Scope_Push_SetJmp(); ret = setjmp((*getJmpBuf())); Scope_DestroyScopeTail(ret);
void Scope_LongJmp(int ret);
// Don't use these directly, just for Scope_SetJmp {
void Scope_Push_SetJmp();
void Scope_DestroyScopeTail(int ret);
jmp_buf *getJmpBuf();
// } Don't use these directly, just for Scope_SetJmp

struct String;
typedef struct String String;
struct strkernel;
typedef struct strkernel strkernel;

void String_init(String *This, bool onstack);
String *String_new();
String *String_FromChars(const char *s, int len /*=-1*/);
String *String_FromString(const String *s);
void String_clear(String *This);
void String_AssignString(String *This, const String *s);
void String_AssignNChars(String *This, const char *s, int n);
void String_AssignChars(String *This, const char *s);
void String_AddCharsInPlace(String *This, const char *s);
void String_AddCharInPlace(String *This, char c);
void String_AddStringInPlace(String *This, const String *rhs);
String *String_AddChars(const String *lhs, const char *rhs);
String *String_AddChar(const String *lhs, char rhs);
String *String_AddString(const String *lhs, const String *rhs);
int String_length(const String *This);
const char *String_Chars(const String *This);
bool String_Equal(const String *This, const String *rhs);
bool String_NotEqual(const String *This, const String *rhs);
bool String_LessThan(const String *This, const String *rhs);
String *String_substr(const String *This, int first, int cnt);
String *String_slice(const String *This, int first, int last);
void String_ReplaceAll(String *This, const char *s, const char *with);
void String_ReplaceAt(String *This, int at, int len, const char *with);
void String_vFormatString(String *This, const char *fmt, va_list args);
String *String_FormatString(const char *fmt, ...);
void String_ReFormatString(String *This, const char *fmt, ...);

struct String {
  strkernel *m_p;
};

struct ElementOps;
typedef struct ElementOps ElementOps;

const ElementOps *getIntElement();
const ElementOps *getPointerElement();
const ElementOps *getVectorAnyElement();
const ElementOps *getSetAnyElement();
const ElementOps *getMapAnyElement();
const ElementOps *getStringElement();

// Do any constructor actions.
// Memory is zeroed out before init.
// Default does nothing.
typedef void (*elementInit)(void *e, bool onstack);

// Do any destructor actions.
// Default does nothing.
typedef void (*elementDestroy)(void *e);

// Test if lhs < rhs.
// Default uses elementSize/isInteger/isSigned as follows:
// If elementSize is the size of an architecture integer value, then a cast is
// made to an appropriate type, taking isSigned into account, and the values
// are compared.
// Otherwise, Default is memcmp.
typedef bool (*elementLessThan)(const void *lhs, const void *rhs);

// Test if lhs == rhs.  Default implementation is memcmp.
typedef bool (*elementEqual)(const void *lhs, const void *rhs);

// Assign will copy from rhs to lhs, cloning what is must.
// Default implementation simply copies the memory.
typedef void (*elementAssign)(void *lhs, const void *rhs);

// Move is effectively memcopy, it overwrites lhs without respect for what was there before.
// It is assumed that anything in lhs has already undergone destroy.
// It is assumed that rhs is destroyed during the move process.
// lhs and rhs are guaranteed not to overlap.
// Default implementation is memmove
typedef void (*elementCopy)(void *lhs, void *rhs, int count);

struct ElementOps {
  int elementSize;
  bool isInteger;
  bool isSigned;
  elementInit init;
  elementDestroy destroy;
  elementLessThan lessthan;
  elementEqual equal;
  elementAssign assign;
  elementCopy copy;
};

struct VectorAny;
typedef struct VectorAny VectorAny;
struct VectorAnyIterator;
typedef struct VectorAnyIterator VectorAnyIterator;

void VectorAny_init(VectorAny *This, const ElementOps *ops, bool onstack);
void VectorAny_destroy(VectorAny *This);
void VectorAny_capacity(VectorAny *This, size_t mincap);
void *VectorAny_ArrayOp(VectorAny *This, int i);
const void *VectorAny_ArrayOpConst(const VectorAny *This, int i);
#define VectorAny_ArrayOpT(This,i,T) (*((T*)VectorAny_ArrayOp(This,i)))
#define VectorAny_ArrayOpConstT(This,i,T) (*((const T*)VectorAny_ArrayOpConst(This,i)))
void VectorAny_insert(VectorAny *This, int at, const void *value);
void VectorAny_insertMany(VectorAny *This, int at, const void *value, size_t count);
void VectorAny_clear(VectorAny *This);
void VectorAny_set(VectorAny *This, int i, const void *value);
void VectorAny_push_back(VectorAny *This, const void *value);
void VectorAny_resizeWithDefault(VectorAny *This, size_t newsize, const void *def);
void VectorAny_resize(VectorAny *This, size_t newsize);
void VectorAny_pop_back(VectorAny *This);
int VectorAny_erase(VectorAny *This, int i);
void VectorAny_eraseRange(VectorAny *This, int first, int last);
void *VectorAny_ptr(VectorAny *This);
const void *VectorAny_ptrConst(const VectorAny *This);
bool VectorAny_LessThan(const VectorAny *lhs, const VectorAny *rhs);
bool VectorAny_Equal(const VectorAny *lhs, const VectorAny *rhs);
void VectorAny_Assign(VectorAny *lhs, const VectorAny *rhs);

struct VectorAny {
  void *m_p;
  size_t m_size;
  size_t m_capacity;
  const ElementOps *m_ops;
};

struct SetAny;
typedef struct SetAny SetAny;

void SetAny_init(SetAny *This, const ElementOps *ops, bool onstack);
void SetAny_destroy(SetAny *This);
bool SetAny_LessThan(const SetAny *lhs, const SetAny *rhs);
bool SetAny_Equal(const SetAny *lhs, const SetAny *rhs);
void SetAny_Assign(SetAny *lhs, const SetAny *rhs);
int SetAny_size(const SetAny *This);
const void *SetAny_getByIndexConst(const SetAny *This, int i);
#define SetAny_getByIndexConstT(This,i,T) (*((const T*)SetAny_getByIndexConst(This,i)))
const void *SetAny_findConst(const SetAny *This, const void *value);
#define SetAny_findConstT(This,value,T) (*((const T*)SetAny_findConst(This,value)))
int SetAny_insert(SetAny *This, const void *value, bool *found);
void SetAny_insertMany(SetAny *This, const void *value, int count);
void SetAny_erase(SetAny *This, const void *value);
void SetAny_eraseMany(SetAny *This, const void *value, int count);
void SetAny_eraseAtIndex(SetAny *This, int i);
bool SetAny_contains(const SetAny *This, const void *value);
void SetAny_clear(SetAny *This);
void *SetAny_ptr(SetAny *This);
const void *SetAny_ptrConst(const SetAny *This);
void SetAny_intersection(SetAny *out, const SetAny *lhs, const SetAny *rhs);
bool SetAny_intersects(const SetAny *lhs, const SetAny *rhs);

struct SetAny {
  VectorAny m_values;
};

struct MapAny;
typedef struct MapAny MapAny;

void MapAny_init(MapAny *This, const ElementOps *keyops, const ElementOps *valueops, bool onstack);
void MapAny_destroy(MapAny *This);
int MapAny_size(const MapAny *This);
void MapAny_getByIndex(MapAny *This, int i, const void **key, void **value);
void MapAny_getByIndexConst(const MapAny *This, int i, const void **key, const void **value);
void *MapAny_find(MapAny *This, const void *key);
#define MapAny_findT(This,key,T)  (*((T*)MapAny_find(This,key)))
const void *MapAny_findConst(const MapAny *This, const void *key);
#define MapAny_findConstT(This,key,T) (*((const T*)MapAny_findConst(This,key)))
int MapAny_insert(MapAny *This, const void *key, const void *value);
void MapAny_clear(MapAny *This);
void MapAny_erase(MapAny *This, const void *key);
bool MapAny_contains(const MapAny *This, const void *value);
bool MapAny_LessThan(const MapAny *lhs, const MapAny *rhs);
bool MapAny_Equal(const MapAny *lhs, const MapAny *rhs);
void MapAny_Assign(MapAny *lhs, const MapAny *rhs);

struct MapAny {
  SetAny m_keys;
  VectorAny m_values;
};

#endif
