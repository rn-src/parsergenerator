#include "tinytemplates.h"

enum nodetype {
  NODE_ROOT,
  NODE_SETJMP,
  NODE_SCOPE,
  NODE_DESTROYER
};
typedef enum nodetype nodetype;

struct DestructorNode;
typedef struct DestructorNode DestructorNode;
struct DestructorNode {
  DestructorNode *prev;
  nodetype nodeType;
  void *vpstack;
  vpstack_destroyer destroyer;
};

DestructorNode g_root = {0,NODE_ROOT,0,0};
DestructorNode *g_tail = 0;

static DestructorNode *getDestroyerTail() {
  if( ! g_tail )
    g_tail = &g_root;
  return g_tail;
}

static void setDestroyerTail(DestructorNode *tail) {
  g_tail = tail;
}

void Scope_Push_Node(nodetype nodeType, void *vpstack, vpstack_destroyer destroyer) {
  DestructorNode *tail = getDestroyerTail();
  DestructorNode *node = (DestructorNode*)malloc(sizeof(DestructorNode));
  node->nodeType = nodeType;
  node->prev = tail;
  node->vpstack = vpstack;
  node->destroyer = destroyer;
  setDestroyerTail(node);
}

void Push_Destroy(void *vpstack, vpstack_destroyer destroyer) {
  Scope_Push_Node(NODE_DESTROYER, vpstack, destroyer);
}

void Scope_Push() {
  Scope_Push_Node(NODE_SCOPE, 0, 0);
}

void Scope_Push_SetJmp() {
  void *vpbuf = malloc(sizeof(jmp_buf));
  memset(vpbuf, 0, sizeof(jmp_buf));
  Scope_Push_Node(NODE_SETJMP, vpbuf, 0);
}

jmp_buf *getJmpBuf() {
  return (jmp_buf*)getDestroyerTail()->vpstack;
}

void Scope_Pop() {
  DestructorNode *node = getDestroyerTail();
  DestructorNode *prev = 0;
  while( node->nodeType != NODE_SCOPE && node->nodeType != NODE_ROOT) {
    prev = node->prev;
    if( node->nodeType == NODE_DESTROYER ) {
      node->destroyer(node->vpstack);
    } else {
      free(node->vpstack);
    }
    free(node);
    node = prev;
  }
  if( node->nodeType == NODE_SCOPE ) {
    prev = node->prev;
    free(node);
    node = prev;
  }
  setDestroyerTail(node);
}

void Scope_DestroyScopeTail(int ret) {
  if (!ret)
    return;
  DestructorNode *node = getDestroyerTail();
  DestructorNode *prev = node->prev;
  free(node->vpstack);
  free(node);
  setDestroyerTail(prev);
}

void Scope_LongJmp(int ret) {
  DestructorNode *tail = getDestroyerTail();
  DestructorNode *node = tail;
  while( node->nodeType != NODE_SETJMP && node->nodeType != NODE_ROOT )
    node = node->prev;
  if( node->nodeType == NODE_ROOT )
    exit(1);
  while (tail != node) {
    // Scope_SetJmp will clean up just the NODE_SETJMP
    if (tail->nodeType == NODE_DESTROYER && tail->destroyer)
      tail->destroyer(tail->vpstack);
    DestructorNode *prev = tail->prev;
    free(tail);
    tail = prev;
  }
  setDestroyerTail(node);
  longjmp(*getJmpBuf(), ret);
}

struct strkernel {
  char *m_s;
  int m_refs;
  int m_len;
};
typedef struct strkernel strkernel;

void String_init(String *This,bool onstack) {
  memset(This,0,sizeof(String));
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)String_clear);
}

String *String_new() {
  String *This = (String*)malloc(sizeof(String));
  memset(This,0,sizeof(String));
  This->m_p = 0;
  return This;
}

String *String_FromChars(const char *s, int len /*=-1*/) {
  String *This = String_new();
  String_AssignNChars(This,s,len);
  return This;
}

String *String_FromString(const String *s) {
  String *This = String_new();
  String_AssignString(This,s);
  return This;
}

void String_clear(String *This) {
  if (This->m_p) {
    if (--(This->m_p->m_refs) == 0) {
      free(This->m_p->m_s);
      free(This->m_p);
    }
    This->m_p = 0;
  }
}

void String_AssignString(String *This, const String *s) {
  if( s->m_p )
    s->m_p->m_refs++;
  String_clear(This);
  This->m_p = s->m_p;
}

void String_AssignNChars(String *This, const char *s, int n) {
  if( This->m_p && This->m_p->m_refs == 1 ) {
    This->m_p->m_len = n;
    This->m_p->m_s = (char*)realloc(This->m_p->m_s,This->m_p->m_len+1);
    strncpy(This->m_p->m_s,s,n);
    This->m_p->m_s[n] = 0;
  } else {
    String_clear(This);
    This->m_p = 0;
    if( s && *s) {
      This->m_p = (strkernel*)malloc(sizeof(strkernel));
      This->m_p->m_len = n;
      This->m_p->m_s = (char*)malloc(This->m_p->m_len+1);
      strncpy(This->m_p->m_s,s,n);
      This->m_p->m_s[n] = 0;
      This->m_p->m_refs = 1;
    }
  }
}

void String_AssignChars(String *This, const char *s) {
  int n = strlen(s);
  String_AssignNChars(This,s,n);
}

void String_AddCharsInPlace(String *This, const char *s) {
  if( ! s || ! *s )
    return;
  if( This->m_p && This->m_p->m_refs == 1 ) {
    This->m_p->m_len += strlen(s);
    This->m_p->m_s = (char*)realloc(This->m_p->m_s,This->m_p->m_len+1);
    strcat(This->m_p->m_s,s);
  } else {
    strkernel *newp = (strkernel*)malloc(sizeof(strkernel));
    newp->m_len = String_length(This)+strlen(s);
    newp->m_s = (char*)malloc(newp->m_len+1);
    *newp->m_s = 0;
    if( This->m_p && This->m_p->m_s )
      strcat(newp->m_s,This->m_p->m_s);
    strcat(newp->m_s,s);
    newp->m_refs = 1;
    String_clear(This);
    This->m_p = newp;
  }
}

void String_AddCharInPlace(String *This, char c) {
  if( ! c )
    return;
  char cc[2];
  cc[0] = c;
  cc[1] = 0;
  String_AddCharsInPlace(This,cc);
}

void String_AddStringInPlace(String *This, const String *rhs) {
  String_AddCharsInPlace(This, String_Chars(rhs));
}

String *String_AddChars(const String *lhs, const char *rhs) {
  String *This = String_new();
  String_AssignString(This,lhs);
  String_AddCharsInPlace(This,rhs);
  return This;
}

String *String_AddChar(const String *lhs, char rhs) {
  String *This = String_new();
  String_AssignString(This,lhs);
  String_AddCharInPlace(This,rhs);
  return This;
}

String *String_AddString(const String *lhs, const String *rhs) {
  String *This = String_new();
  String_AssignString(This,lhs);
  String_AddStringInPlace(This,rhs);
  return This;
}

int String_length(const String *This) {
  if( This->m_p )
    return This->m_p->m_len;
  return 0;
}

const char *String_Chars(const String *This) {
  if( This->m_p )
    return This->m_p->m_s;
  return 0;
}

bool String_Equal(const String *This, const String *rhs) {
  if( String_length(This) != String_length(rhs) )
    return false;
  if( String_length(This) == 0 )
    return true;
  const char *s1 = This->m_p->m_s, *s2 = rhs->m_p->m_s;
  while( *s1 && *s2 ) {
    if( *s1 != *s2 )
      return false;
    ++s1;
    ++s2;
  }
  if( *s1 != *s2 )
    return false;
  return true;
}

bool String_NotEqual(const String *This, const String *rhs) {
  return ! String_Equal(This,rhs);
}

bool String_LessThan(const String *This, const String *rhs) {
  if( ! This->m_p || ! rhs->m_p ) {
    if( ! This->m_p && rhs->m_p != 0 )
      return true;
    return false;
  }
  const char *s1 = This->m_p->m_s, *s2 = rhs->m_p->m_s;
  while( *s1 && *s2 ) {
    if( *s1 < *s2 )
      return true;
    else if( *s1 > *s2 )
      return false;
    ++s1;
    ++s2;
  }
  if( ! *s1 && ! *s2 )
    return false;
  else if( ! *s1 )
    return true;
  return false;
}

String *String_substr(const String *This, int first, int cnt) {
  size_t len = String_length(This);
  if( first >= len || cnt <= 0 )
    return String_new();
  if( cnt > len )
    cnt = len;
  String *ret = String_new();
  String_AssignNChars(ret,String_Chars(This)+first,cnt);
  return ret;
}

String *String_slice(const String *This, int first, int last) {
  size_t len = String_length(This);
  if( first < 0 )
    first += len;
  if( last < 0 )
    last += len;
  return String_substr(This,first,last-first);
}

void String_ReplaceAll(String *This, const char *s, const char *with) {
  if( ! s || ! *s )
    return;
  int len0 = strlen(s);
  int len1 = strlen(with);
  const char *src = String_Chars(This);
  const char *nxt = 0;
  int finallen = 0;
  while( (nxt = strstr(src,s)) != 0) {
    finallen += nxt-src;
    finallen += len1;
    src = nxt+len0;
  }
  finallen += strlen(src);
  char *buf = (char*)malloc(finallen+1);
  char *dst = buf;
  src = String_Chars(This);
  while( (nxt = strstr(src,s)) != 0) {
    int leadlen = nxt-src;
    strncpy(dst,src,leadlen);
    dst += leadlen;
    if( len1 ) {
      strncpy(dst,with,len1);
      dst += len1;
    }
    src = nxt+len0;
  }
  int taillen = strlen(src);
  if( taillen ) {
    strncpy(dst,src,taillen);
    dst += taillen;
  }
  *dst = 0;
  String_AssignChars(This,buf);
  free(buf);
}

void String_ReplaceAt(String *This, int at, int len, const char *with) {
  if( at < 0 )
    at = String_length(This)+at;
  if( at < 0 || at > String_length(This) )
    return;
  int finallen = at+strlen(with)+(String_length(This)-len-at);
  char *buf = (char*)malloc(finallen+1);
  char *dst = buf;
  if( at > 0 ) {
    strncpy(dst,String_Chars(This),at);
    dst += at;
  }
  if( strlen(with) > 0 ) {
    strncpy(dst,with,strlen(with));
    dst += strlen(with);
  }
  int taillen = String_length(This)-len-at;
  if( taillen > 0 ) {
    strncpy(dst,String_Chars(This)+at+len,taillen);
    dst += taillen;
  }
  *dst = 0;
  String_AssignChars(This,buf);
  free(buf);
}

void String_vFormatString(String *This, const char *fmt, va_list args) {
  // TODO : premeasure length instead of a fixed length
  char buf[3200];
  vsprintf(buf,fmt,args);
  String_AssignChars(This,buf);
}

String *String_FormatString(const char *fmt, ...) {
  String *This = String_new();
  va_list args;
  va_start(args,fmt);
  String_vFormatString(This, fmt, args);
  va_end(args);
  return This;
}

void String_ReFormatString(String *This, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  String_vFormatString(This, fmt, args);
  va_end(args);
}

ElementOps IntElement = {sizeof(int), true, true, 0, 0, 0, 0, 0, 0};
ElementOps PointerElement = {sizeof(void*), true, false, 0, 0, 0, 0, 0, 0};
ElementOps VectorAnyElement = {sizeof(VectorAny), false, false, 0, (elementDestroy)VectorAny_destroy, (elementLessThan)VectorAny_LessThan, (elementEqual)VectorAny_Equal, (elementAssign)VectorAny_Assign, 0};
ElementOps SetAnyElement = {sizeof(SetAny), false, false, 0, (elementDestroy)SetAny_destroy, (elementLessThan)SetAny_LessThan, (elementEqual)SetAny_Equal, (elementAssign)SetAny_Assign, 0};
ElementOps MapAnyElement = {sizeof(MapAny), false, false, 0, (elementDestroy)MapAny_destroy, (elementLessThan)MapAny_LessThan, (elementEqual)MapAny_Equal, (elementAssign)MapAny_Assign, 0};
ElementOps StringElement = {sizeof(String), false, false, (elementInit)String_init, (elementDestroy)String_clear, (elementLessThan)String_LessThan, (elementEqual)String_Equal, (elementAssign)String_AssignString, 0};

const ElementOps *getIntElement() {
  return &IntElement;
}

const ElementOps *getPointerElement() {
  return &PointerElement;
}

const ElementOps *getVectorAnyElement() {
  return &VectorAnyElement;
}

const ElementOps *getSetAnyElement() {
  return &SetAnyElement;
}

const ElementOps *getMapAnyElement() {
  return &MapAnyElement;
}

const ElementOps *getStringElement() {
  return &StringElement;
}

static void VectorAny_destroyElements(VectorAny *This, int offset, int count) {
  if( count <= 0 || ! This->m_ops->destroy )
    return;
  elementDestroy destroy = This->m_ops->destroy;
  int elementSize = This->m_ops->elementSize;
  void *ptr = ((char*)This->m_p) + (offset*elementSize);
  for( int i = 0; i < count; ++i ) {
    destroy(ptr);
    ptr = (char*)ptr+elementSize;
  }
}

static void VectorAny_initElements(VectorAny *This, int offset, int count, const void *defaultValue) {
  if( count <= 0 )
    return;
  int elementSize = This->m_ops->elementSize;
  void *ptr = ((char*)This->m_p) + (offset*elementSize);
  if( This->m_ops->init ) {
    elementInit init = This->m_ops->init;
    for( int i = 0; i < count; ++i ) {
      init(ptr,false);
      ptr = (char*)ptr+elementSize;
    }
  } else {
    memset(ptr,0,elementSize*count);
  }
  if( defaultValue ) {
    ptr = ((char*)This->m_p) + (offset*elementSize);
    if( This->m_ops->assign ) {
      elementAssign assign = This->m_ops->assign;
      for( int i = 0; i < count; ++i ) {
        assign(ptr,defaultValue);
        ptr = (char*)ptr+elementSize;
      }
    } else {
      for( int i = 0; i < count; ++i ) {
        memcpy(ptr,defaultValue,elementSize);
        ptr = (char*)ptr+elementSize;
      }
    }
  }
}

static void VectorAny_moveElements(VectorAny *This, int dst, int src, int count) {
  if( dst == src || count <= 0 )
    return;
  int elementSize = This->m_ops->elementSize;
  elementCopy copy = This->m_ops->copy;
  if( copy ) {
    if( dst < src ) {
      int blk = src-dst;
      if( blk > count )
        blk = count;
      char *vpdst = ((char*)This->m_p) + (dst*elementSize);
      char *vpsrc = ((char*)This->m_p) + (src*elementSize);
      int i = 0;
      while( i+blk <= count ) {
        copy(vpdst,vpsrc,blk);
        vpdst += elementSize*blk;
        vpsrc += elementSize*blk;
        i+=blk;
      }
      if( i < count )
       copy(vpdst,vpsrc,count-i);
    } else {
      int blk = dst-src;
      if( blk > count )
        blk = count;
      char *vpdst = ((char*)This->m_p) + ((dst+count-blk)*elementSize);
      char *vpsrc = ((char*)This->m_p) + ((src+count-blk)*elementSize);
      int i = 0;
      while( i+blk <= count ) {
        copy(vpdst,vpsrc,blk);
        vpdst -= elementSize*blk;
        vpsrc -= elementSize*blk;
        i+=blk;
      }
      if( i < count )
       copy(vpdst,vpsrc,count-i);
    }
  } else {
    char *vpdst = ((char*)This->m_p) + (dst*elementSize);
    char *vpsrc = ((char*)This->m_p) + (src*elementSize);
    memmove(vpdst,vpsrc,elementSize*count);
  }
}

void VectorAny_destroy(VectorAny *This) {
  VectorAny_clear(This);
  if (This->m_p)
    free(This->m_p);
  This->m_p = 0;
  This->m_capacity = 0;
}

void VectorAny_capacity(VectorAny *This, size_t mincap) {
  if( mincap <= This->m_capacity )
    return;
  if( This->m_capacity == 0 )
    This->m_capacity = 8;
  while( This->m_capacity < mincap ) {
    This->m_capacity *= 2;
  }
  if( ! This->m_p )
    This->m_p = malloc(This->m_ops->elementSize*This->m_capacity);
  else
    This->m_p = realloc(This->m_p,This->m_ops->elementSize*This->m_capacity);
}

VectorAny *VectorAny_assign(VectorAny *lhs, const VectorAny *rhs) {
  VectorAny_clear(lhs);
  VectorAny_insertMany(lhs,lhs->m_size,rhs->m_p,rhs->m_size);
  return lhs;
}

void VectorAny_init(VectorAny *This, const ElementOps *ops, bool onstack) {
  This->m_p = 0;
  This->m_size = 0;
  This->m_capacity = 0;
  This->m_ops = ops;
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)VectorAny_destroy);
}

bool VectorAny_empty(const VectorAny *This) {
  return VectorAny_size(This) == 0;
}

void *VectorAny_ArrayOp(VectorAny *This, int i) {
  if( i < 0 )
    i += This->m_size;
  return ((char*)(This->m_p))+(This->m_ops->elementSize*i);
}

const void *VectorAny_ArrayOpConst(const VectorAny *This, int i) {
  if( i < 0 )
    i+= This->m_size;
  if (i < 0 || i >= This->m_size)
    return 0;
  return ((const char*)(This->m_p))+(This->m_ops->elementSize*i);
}

void VectorAny_clear(VectorAny *This) {
  VectorAny_destroyElements(This,0,This->m_size);
  This->m_size = 0;
}

void VectorAny_set(VectorAny *This, int i, const void *value) {
  if( i < 0 )
    i += This->m_size;
  void *dst = (char*)This->m_p+i*This->m_ops->elementSize;
  if( This->m_ops->assign )
    This->m_ops->assign(dst,value);
  else
    memcpy(dst,value,This->m_ops->elementSize);  
}

void VectorAny_push_back(VectorAny *This, const void *ptrvalue) {
  VectorAny_capacity(This,This->m_size+1);
  void *dst = (char*)This->m_p+(This->m_size*This->m_ops->elementSize);
  memset(dst,0,This->m_ops->elementSize);
  if( This->m_ops->init )
    This->m_ops->init(dst,false);
  if( This->m_ops->assign )
    This->m_ops->assign(dst,ptrvalue);
  else
    memcpy(dst,ptrvalue,This->m_ops->elementSize);
  ++This->m_size;
}

void VectorAny_resizeWithDefault(VectorAny *This, size_t newsize, const void *def) {
  if( newsize < This->m_size ) {
    VectorAny_destroyElements(This,newsize,This->m_size-newsize+1);
    This->m_size = newsize;
  } else if( newsize > This->m_size ) {
    VectorAny_capacity(This,newsize);
    VectorAny_initElements(This,This->m_size,newsize-This->m_size+1,def);
    This->m_size = newsize;
  }
}

void VectorAny_resize(VectorAny *This, size_t newsize) {
  if( newsize < This->m_size ) {
    VectorAny_destroyElements(This,newsize,This->m_size-newsize+1);
    This->m_size = newsize;
  } else if( newsize > This->m_size ) {
    VectorAny_capacity(This,newsize);
    VectorAny_initElements(This,This->m_size,newsize-This->m_size,0);
    This->m_size = newsize;
  }
}

void VectorAny_pop_back(VectorAny *This) {
  if( This->m_size == 0 )
    return;
  VectorAny_erase(This,-1);
}

int VectorAny_erase(VectorAny *This, int at) {
  if (at < 0)
    at += This->m_size;
  VectorAny_destroyElements(This,at,1);
  int following = This->m_size-at-1;
  if( following > 0 )
    VectorAny_moveElements(This,at,at+1,following);
  --This->m_size;
  return at;
}

void VectorAny_eraseRange(VectorAny *This, int first, int last) {
  if (first < 0)
    first += This->m_size;
  if (last < 0)
    last += This->m_size;
  int cnt = last-first;
  VectorAny_destroyElements(This,first,last-first+1);
  int following = This->m_size-last-1;
  if (following > 0)
    VectorAny_moveElements(This,first,last,following);
  This->m_size -= cnt;
}

void VectorAny_insert(VectorAny *This, int at, const void *value) {
  VectorAny_capacity(This,This->m_size+1);
  int following = This->m_size-at;
  if (following > 0)
    VectorAny_moveElements(This,at+1,at,following);
  VectorAny_initElements(This,at,1,value);
  ++This->m_size;
}

void VectorAny_insertMany(VectorAny *This, int at, const void *value, size_t count) {
  if (count == 0)
    return;
  VectorAny_capacity(This,This->m_size+count);
  int following = This->m_size-at;
  if (following > 0)
    VectorAny_moveElements(This,at+1,at,following);
  int elementSize = This->m_ops->elementSize;
  for( int i = 0; i < count; ++i )
    VectorAny_initElements(This,at+i,1,(char*)value+(i*elementSize));
  This->m_size += count;
}

static bool lt_schar(const unsigned char *lhs, const unsigned char *rhs) {
  return *lhs < *rhs;
}
static bool lt_sshort(const unsigned short *lhs, const unsigned short *rhs) {
  return *lhs < *rhs;
}
static bool lt_sint(const unsigned int *lhs, const unsigned int *rhs) {
  return *lhs < *rhs;
}
static bool lt_slong(const unsigned long *lhs, const unsigned long *rhs) {
  return *lhs < *rhs;
}
static bool lt_slonglong(const unsigned long long *lhs, const unsigned long long *rhs) {
  return *lhs < *rhs;
}
#if 0
static bool lt_uchar(const unsigned char *lhs, const unsigned char *rhs) {
  return *lhs < *rhs;
}
#endif
static bool lt_ushort(const unsigned short *lhs, const unsigned short *rhs) {
  return *lhs < *rhs;
}
static bool lt_uint(const unsigned int *lhs, const unsigned int *rhs) {
  return *lhs < *rhs;
}
static bool lt_ulong(const unsigned long *lhs, const unsigned long *rhs) {
  return *lhs < *rhs;
}
static bool lt_ulonglong(const unsigned long long *lhs, const unsigned long long *rhs) {
  return *lhs < *rhs;
}

static elementLessThan getNumericComparer(bool isSigned, int elementSize) {
  if( isSigned ) {
    if( elementSize == sizeof(char) )
      return (elementLessThan)lt_schar;
    else if( elementSize == sizeof(short) )
      return (elementLessThan)lt_sshort;
    else if( elementSize == sizeof(int) )
      return (elementLessThan)lt_sint;
    else if( elementSize == sizeof(long) )
      return (elementLessThan)lt_slong;
    else if( elementSize == sizeof(long long) )
      return (elementLessThan)lt_slonglong;
  } else {
    if( elementSize == sizeof(short) )
      return (elementLessThan)lt_ushort;
    else if( elementSize == sizeof(int) )
      return (elementLessThan)lt_uint;
    else if( elementSize == sizeof(long) )
      return (elementLessThan)lt_ulong;
    else if( elementSize == sizeof(long long) )
      return (elementLessThan)lt_ulonglong;
  }
  return 0;
}

bool VectorAny_LessThan(const VectorAny *lhs, const VectorAny *rhs) {
  int len = VectorAny_size(lhs)<VectorAny_size(rhs)?VectorAny_size(lhs):VectorAny_size(rhs);
  if( lhs->m_ops->elementSize != rhs->m_ops->elementSize || lhs->m_ops->isInteger != rhs->m_ops->isInteger || lhs->m_ops->isSigned != rhs->m_ops->isSigned || lhs->m_ops->lessthan != rhs->m_ops->lessthan )
    return false;
  int elementSize = lhs->m_ops->elementSize;
  elementLessThan lessthan = lhs->m_ops->lessthan;
  // For numeric elements with no supplied function, compare using the endian-ness of the system.
  if( ! lessthan && lhs->m_ops->isInteger )
    lessthan = getNumericComparer(lhs->m_ops->isSigned,elementSize);
  if( lessthan ) {
    for( int i = 0, n = len; i < n; ++i ) {
      if( lessthan(VectorAny_ArrayOpConst(lhs,i),VectorAny_ArrayOpConst(rhs,i)) ) {
        return true;
      } else if( lessthan(VectorAny_ArrayOpConst(rhs,i),VectorAny_ArrayOpConst(lhs,i)) ) {
        return false;
      }
    }
  } else {
    int cmp = memcmp(lhs->m_p,rhs->m_p,elementSize*len);
    if( cmp < 0 )
      return true;
    else if( cmp > 0 )
      return false;
  }
  if( VectorAny_size(lhs) < VectorAny_size(rhs) )
    return true;
  return false;
}

bool VectorAny_Equal(const VectorAny *lhs, const VectorAny *rhs) {
  if( VectorAny_size(lhs) != VectorAny_size(rhs) )
    return false;
  if( lhs->m_ops->elementSize != rhs->m_ops->elementSize || lhs->m_ops->isInteger != rhs->m_ops->isInteger || lhs->m_ops->isSigned != rhs->m_ops->isSigned || lhs->m_ops->equal != rhs->m_ops->equal )
    return false;
  if( lhs->m_size == 0 )
    return true;
  int elementSize = lhs->m_ops->elementSize;
  elementEqual equal = lhs->m_ops->equal;
  if( equal ) {
    for( int i = 0, n = VectorAny_size(lhs); i < n; ++i ) {
      if( ! equal(VectorAny_ArrayOpConst(lhs,i),VectorAny_ArrayOpConst(rhs,i)) ) {
        return false;
      }
    }
  } else {
    int cmp = memcmp(lhs->m_p,rhs->m_p,elementSize*lhs->m_size);
    if( cmp != 0 )
      return false;
  }
  return true;
}

void VectorAny_Assign(VectorAny *lhs, const VectorAny *rhs) {
  VectorAny_clear(lhs);
  if (!lhs->m_ops)
    lhs->m_ops = rhs->m_ops;
  else if ( lhs->m_ops != rhs->m_ops && (lhs->m_ops->elementSize != rhs->m_ops->elementSize || lhs->m_ops->isInteger != rhs->m_ops->isInteger || lhs->m_ops->isSigned != rhs->m_ops->isSigned || lhs->m_ops->assign != rhs->m_ops->assign) )
    return;
  VectorAny_resize(lhs,VectorAny_size(rhs));
  int elementSize = lhs->m_ops->elementSize;
  elementAssign assign = lhs->m_ops->assign;
  if( assign ) {
    for( int i = 0, n = VectorAny_size(rhs); i < n; ++i )
      assign(VectorAny_ArrayOp(lhs,i),VectorAny_ArrayOpConst(rhs,i));
  } else if(rhs->m_size ) {
    memcpy(lhs->m_p,rhs->m_p,rhs->m_size*elementSize);
  }
}

int VectorAny_size(const VectorAny *This) {
  return This->m_size;
}

void *VectorAny_ptr(VectorAny *This) {
  return This->m_p;
}

const void *VectorAny_ptrConst(const VectorAny *This) {
  return This->m_p;
}

void SetAny_init(SetAny *This, const ElementOps *ops, bool onstack) {
  VectorAny_init(&This->m_values,ops,onstack);
}

void SetAny_destroy(SetAny *This) {
  VectorAny_destroy(&This->m_values);
}

int SetAny_size(const SetAny *This) {
  return VectorAny_size(&This->m_values);
}

const void *SetAny_getByIndexConst(const SetAny *This, int i) {
  return VectorAny_ArrayOpConst(&This->m_values,i);
}

bool SetAny_LessThan(const SetAny *lhs, const SetAny *rhs) {
  return VectorAny_LessThan(&lhs->m_values,&rhs->m_values);
}

bool SetAny_Equal(const SetAny *lhs, const SetAny *rhs) {
  return VectorAny_Equal(&lhs->m_values,&rhs->m_values);
}

void SetAny_Assign(SetAny *lhs, const SetAny *rhs) {
  VectorAny_Assign(&lhs->m_values,&rhs->m_values);
}

int SetAny_findIndex(const SetAny *This, const void *key, bool *found) {
  if( VectorAny_size(&This->m_values) == 0) {
    found = false;
    return 0;
  }
  const void *vpbase = This->m_values.m_p;
  int elementSize = This->m_values.m_ops->elementSize;
  int low = 0, high = VectorAny_size(&This->m_values)-1;
  elementLessThan lessthan = This->m_values.m_ops->lessthan;
  if( ! lessthan && This->m_values.m_ops->isInteger )
    lessthan = getNumericComparer(This->m_values.m_ops->isSigned,elementSize);
  if( lessthan ) {
    while( low <= high ) {
      int mid = low+(high-low)/2;
      const void *vpmid = (const char*)vpbase+mid*elementSize;
      if( lessthan(key,vpmid) )
        high = mid-1;
      else if( lessthan(vpmid,key) )
        low = mid+1;
      else {
        *found = true;
        return mid;
      }
    }
  } else {
    while( low <= high ) {
      int mid = low+(high-low)/2;
      const void *vpmid = (const char*)vpbase+mid*elementSize;
      if( memcmp(key,vpmid,elementSize) < 0 )
        high = mid-1;
      else if( memcmp(vpmid,key,elementSize) < 0 )
        low = mid+1;
      else {
        *found = true;
        return mid;
      }
    }
  }
  *found = false;
  return low;
}

const void *SetAny_findConst(const SetAny *This, const void *key) {
  bool found = false;
  int i = SetAny_findIndex(This,key,&found);
  if( ! found )
    return 0;
  return SetAny_getByIndexConst(This,i);
}

int SetAny_insert(SetAny *This, const void *value, bool *pfound) {
  bool found = false;
  int i = SetAny_findIndex(This,value,&found);
  if( ! found )
    VectorAny_insert(&This->m_values,i,value);
  if( pfound )
    *pfound = found;
  return i;
}

void SetAny_insertMany(SetAny *This, const void *value, int count) {
  bool found = false;
  int elementSize = This->m_values.m_ops->elementSize;
  for( int i = 0; i < count; ++i )
    SetAny_insert(This,(char*)value+i*elementSize,&found);
}

void SetAny_clear(SetAny *This) {
  VectorAny_clear(&This->m_values);
}

void SetAny_erase(SetAny *This, const void *value) {
  bool found = false;
  int i = SetAny_findIndex(This,value,&found);
  if( ! found )
    return;
  VectorAny_erase(&This->m_values,i);  
}

void SetAny_eraseMany(SetAny *This, const void *value, int count) {
  int elementSize = This->m_values.m_ops->elementSize;
  for( int i = 0; i < count; ++i )
    SetAny_erase(This,(char*)value+i*elementSize);
}

void SetAny_eraseAtIndex(SetAny *This, int i) {
  VectorAny_erase(&This->m_values,i);  
}

bool SetAny_contains(const SetAny *This, const void *value) {
  bool found = false;
  SetAny_findIndex(This,value,&found);
  return found;
}

void *SetAny_ptr(SetAny *This) {
  return VectorAny_ptr(&This->m_values);
}

const void *SetAny_ptrConst(const SetAny *This) {
  return VectorAny_ptrConst(&This->m_values);
}

bool SetAny_intersects(const SetAny *lhs, const SetAny *rhs) {
  int curlhs = 0, endlhs = SetAny_size(lhs), currhs = 0, endrhs = SetAny_size(lhs);
  elementLessThan lessthan = lhs->m_values.m_ops->lessthan;
  int elementSize = lhs->m_values.m_ops->elementSize;
  if (!lessthan && lhs->m_values.m_ops->isInteger)
    lessthan = getNumericComparer(lhs->m_values.m_ops->isSigned, elementSize);
  while (curlhs < endlhs && currhs < endrhs) {
    const void *lhselem = SetAny_getByIndexConst(lhs, curlhs);
    const void *rhselem = SetAny_getByIndexConst(lhs, curlhs);
    int cmp = 0;
    if (lessthan) {
      if (lessthan(lhselem, rhselem))
        cmp = -1;
      else if (lessthan(rhselem, lhselem))
        cmp = 1;
    }
    else {
      cmp = memcmp(lhselem, rhselem, elementSize);
    }
    if (cmp < 0)
      ++curlhs;
    else if (cmp > 0)
      ++currhs;
    else
      return true;
  }
  return false;
}

void SetAny_intersection(SetAny *out, const SetAny *lhs, const SetAny *rhs) {
  int curlhs = 0, endlhs = SetAny_size(lhs), currhs = 0, endrhs = SetAny_size(lhs);
  elementLessThan lessthan = out->m_values.m_ops->lessthan;
  int elementSize = out->m_values.m_ops->elementSize;
  if (!lessthan && out->m_values.m_ops->isInteger)
    lessthan = getNumericComparer(out->m_values.m_ops->isSigned, elementSize);
  SetAny_clear(out);
  while (curlhs < endlhs && currhs < endrhs ) {
    const void *lhselem = SetAny_getByIndexConst(lhs, curlhs);
    const void *rhselem = SetAny_getByIndexConst(lhs, curlhs);
    int cmp = 0;
    if (lessthan) {
      if (lessthan(lhselem, rhselem))
        cmp = -1;
      else if (lessthan(rhselem,lhselem))
        cmp = 1;
    }
    else {
      cmp = memcmp(lhselem, rhselem, elementSize);
    }
    if (cmp < 0)
      ++curlhs;
    else if (cmp > 0)
      ++currhs;
    else {
      VectorAny_push_back(&out->m_values, lhselem);
      ++curlhs;
      ++currhs;
    }
  }
}

void MapAny_init(MapAny *This, const ElementOps *keyops, const ElementOps *valueops, bool onstack) {
  SetAny_init(&This->m_keys,keyops,false);
  VectorAny_init(&This->m_values,valueops,false);
  if( onstack )
    Push_Destroy(This,(vpstack_destroyer)MapAny_destroy);
}

void MapAny_destroy(MapAny *This) {
  SetAny_destroy(&This->m_keys);
  VectorAny_destroy(&This->m_values);
}

int MapAny_size(const MapAny *This) {
  return VectorAny_size(&This->m_values);
}

void MapAny_getByIndex(MapAny *This, int i, const void **key, void **value) {
  *key = SetAny_getByIndexConst(&This->m_keys,i);
  *value = VectorAny_ArrayOp(&This->m_values,i);
}

void MapAny_getByIndexConst(const MapAny *This, int i, const void **key, const void **value) {
  *key = SetAny_getByIndexConst(&This->m_keys,i);
  *value = VectorAny_ArrayOpConst(&This->m_values,i);
}

void *MapAny_find(MapAny *This, const void *key) {
  bool found = false;
  int i = SetAny_findIndex(&This->m_keys,key,&found);
  if( ! found )
    return 0;
  return VectorAny_ArrayOp(&This->m_values,i);
}

const void *MapAny_findConst(const MapAny *This, const void *key) {
  bool found = false;
  int i = SetAny_findIndex(&This->m_keys,key,&found);
  if( ! found )
    return 0;
  return VectorAny_ArrayOpConst(&This->m_values,i);
}

int MapAny_insert(MapAny *This, const void *key, const void *value) {
  bool found = false;
  int i = SetAny_insert(&This->m_keys,key,&found);
  if( ! found )
    VectorAny_insert(&This->m_values,i,value);
  else
    VectorAny_set(&This->m_values,i,value);
  return i;
}

void MapAny_clear(MapAny *This) {
  SetAny_clear(&This->m_keys);
  VectorAny_clear(&This->m_values);
}

void MapAny_erase(MapAny *This, const void *key) {
  bool found = false;
  int i = SetAny_findIndex(&This->m_keys,key,&found);
  if( ! found )
    return;
  SetAny_eraseAtIndex(&This->m_keys,i);
  VectorAny_erase(&This->m_values,i);
}

bool MapAny_contains(const MapAny *This, const void *value) {
  bool found = false;
  SetAny_findIndex(&This->m_keys,value,&found);
  return found;
}

bool MapAny_LessThan(const MapAny *lhs, const MapAny *rhs) {
  return SetAny_LessThan(&lhs->m_keys,&rhs->m_keys);
}

bool MapAny_Equal(const MapAny *lhs, const MapAny *rhs) {
  return SetAny_Equal(&lhs->m_keys,&rhs->m_keys) && VectorAny_Equal(&lhs->m_values,&rhs->m_values);
}

void MapAny_Assign(MapAny *lhs, const MapAny *rhs) {
  SetAny_Assign(&lhs->m_keys,&rhs->m_keys);
  VectorAny_Assign(&lhs->m_values,&rhs->m_values);
}
