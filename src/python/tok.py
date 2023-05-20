#!/usr/bin/env python3
# Provided by parsergenerator - free to use under MIT license
from typing import Protocol, Sequence, Callable, Optional, IO, Any
from array import array
from io import StringIO

MAX_BUFFER_ACCUM: int = 128
BUFFER_TRIM_KEEP: int = 16

class TokenInfo:
  __slots__ = ('tokenCount','sectionCount','tokenaction','tokenstr','isws','stateCount','transitions','transitionOffset','tokens')
  def __init__(self, ns: Any):
    self.tokenCount: int = ns.tokenCount
    self.sectionCount: int = ns.sectionCount
    self.tokenaction: Sequence[int] = ns.tokenaction
    self.tokenstr: Sequence[str] = ns.tokenstr
    self.isws: Sequence[bool] = ns.isws
    self.stateCount: int = ns.stateCount
    self.transitions: Sequence[int] = ns.transitions
    self.transitionOffset: Sequence[int] = ns.transitionOffset
    self.tokens: Sequence[int] = ns.tokens

class ParsePos:
  __slots__ = ('filename','line','col')
  def __init__(self, filename: str, line: int, col: int):
    self.filename: str = filename
    self.line: int = line
    self.col: int = col

class TokBuf(Protocol):
  def peekc(self, n: int) -> int:
    """get the next character after n discarding n characters, without discarding the returned character"""
    pass
  def discard(self, n: int) -> None:
    """discard a number of characters"""
    pass
  def line(self) -> int:
    """get the current line number"""
    pass
  def col(self) -> int:
    """get the current column number"""
    pass
  def filename(self) -> str:
    """get the name of the tokbuf, usually a filename, but sometimes it will be something like <string>"""
    pass
  def getparsepos(self) -> Sequence[ParsePos]:
    """get the full input stack, tracking the place in the input stream"""
    pass

class StackTokBufItem:
  __slots__ = ('tokbuf','startline','lineno','filename')
  def __init__(self, tokbuf: TokBuf, filename: str = '', startline: int = -1, lineno: int = -1):
    self.tokbuf: TokBuf = tokbuf
    self.filename: str = filename
    self.startline: int = startline
    self.lineno: int = lineno

class Token:
  __slots__ = ('s','filename','wslines','wscols','wslen','line','col','sym')
  def __init__(self, s: str = '', filename: str = '', wslines: int = 0, wscols: int = 0, wslen: int = 0, line: int = 0, col: int = 0, sym: int = -1):
    self.s: str = s
    self.filename: str = filename
    self.wslines: int = wslines
    self.wscols: int = wscols
    self.wslen: int = wslen
    self.line: int = line
    self.col: int = col
    self.sym: int = sym

StackEventCallback = Callable[[TokBuf],None]

class StackTokBuf:
  """A TokBuf implementation that keeps a stack of token buffers, wish callbacks during push/pop events"""
  __slots__ = ('stack','endtok','onstartbuf','onendbuf')
  def __init__(self, onstartbuf: Optional[StackEventCallback], onendbuf: Optional[StackEventCallback]):
    self.stack: list[StackTokBufItem] = []
    self.endtok: Token = Token()
    self.onstartbuf: Optional[StackEventCallback] = onstartbuf
    self.onendbuf: Optional[StackEventCallback] = onendbuf

  def push(self, tokbuf: TokBuf) -> None:
    """start reading from a different file"""
    item = StackTokBufItem(tokbuf)
    self.stack.append(item)
    if self.onstartbuf:
      self.onstartbuf(tokbuf)
    # if empty file, pop immediately
    if tokbuf.peekc(0) == -1:
      self.discard(0)

  def peekc(self, n: int) -> int:
    if len(self.stack):
      return self.stack[-1].tokbuf.peekc(n);
    return -1;

  def discard(self, n: int) -> None:
    if len(self.stack) == 0:
      return
    tokbuf = self.stack[-1].tokbuf
    tokbuf.discard(n)
    if tokbuf.peekc(0) == -1:
      del self.stack[-1]
      if self.onendbuf:
        self.onendbuf(tokbuf)
      if len(self.stack) == 0:
        self.endtok.filename = tokbuf.filename()
        self.endtok.line = tokbuf.line()
        self.endtok.col = tokbuf.col()

  def line(self) -> int:
    if len(self.stack):
      item: StackTokBufItem = self.stack[-1]
      tokbuf = item.tokbuf
      if item.lineno > 0:
        return item.lineno + (tokbuf.line()-item.startline)
      return tokbuf.line();
    return self.endtok.line

  def col(self) -> int:
    if len(self.stack):
      item = self.stack[-1]
      tokbuf = item.tokbuf
      return tokbuf.col()
    return self.endtok.col

  def filename(self) -> str:
    if len(self.stack):
      item = self.stack[-1]
      tokbuf = item.tokbuf
      if len(item.filename):
        return item.filename
      return tokbuf.filename()
    return self.endtok.filename

  def setline(self, lineno: int, filename: str) -> None:
    if len(self.stack):
      item = self.stack[-1]
      tokbuf = item.tokbuf
      item.startline = tokbuf.line()
      item.lineno = lineno
      item.filename = filename

  def getparsepos(self) -> Sequence[ParsePos]:
    items: list[ParsePos] = []
    for item in reversed(self.stack):
      items += item.tokbuf.getparsepos()
    return items

class Tokenizer(Protocol):
  def peek(self) -> int:
    """get the next token without discarding it"""
    pass
  def discard(self):
    """discard the next token"""
    pass
  def tokstr(self) -> str:
    """get the next token text"""
    pass
  def filename(self) -> str:
    """get the filename associated with the current token"""
    pass
  def wslines(self) -> int:
    """get the number of whitespaces lines before the current token"""
    pass
  def wscols(self) -> int:
    """get the number of whitespace columns before the current token"""
    pass
  def wslen(self) -> int:
    """get the total length of the whitespace before the current token"""
    pass
  def line(self) -> int:
    """get the line number of the current token"""
    pass
  def col(self) -> int:
    """get the column number of the current token"""
    pass
  def tokid2str(self, tok: int) -> str:
    """get the name of the current token"""
    pass
  def getparsepos(self) -> Sequence[ParsePos]:
    """get the full stack for the parser"""
    pass

class FileTokBuf:
  """A TokBuf based on an IO interface"""
  __slots__ = ('io','eof','buffer','bufpos','_filename','pos','_line','_col')
  def __init__(self, io: IO, filename: str):
    """io is expected to read binary bytes, not decoded characters"""
    self.io: IO = io
    self.eof = False
    self.buffer: array = array('i')
    self.bufpos = 0
    self._filename: str = filename
    self.pos: int = 0
    self._line: int = 1
    self._col: int = 1

  def addc(self) -> bool:
    if self.eof:
      return False
    c = self.io.read(1)
    if not len(c):
      self.eof = True
      return False
    self.buffer.append(ord(c))
    return True

  def peekc(self, n: int) -> int:
    """Look ahead n characters into the buffer and return the character, without discarding the buffer"""
    while n >= len(self.buffer)-self.bufpos:
      if not self.addc():
        return -1
    return self.buffer[self.bufpos+n]

  def discard(self, n: int) -> None:
    for i in range(n):
      if len(self.buffer)-self.bufpos == 0:
        if not self.addc():
          break
      c = self.buffer[self.bufpos]
      self.bufpos += 1
      if c == ord('\n'):
        self._line += 1
        self._col = 1
      else:
        self._col += 1
    # Chop the buffer once in a while, gradually, not all at once
    if self.bufpos > MAX_BUFFER_ACCUM:
      del self.buffer[:self.bufpos-BUFFER_TRIM_KEEP]
      self.bufpos = BUFFER_TRIM_KEEP

  def line(self) -> int:
    return self._line

  def col(self) -> int:
    return self._col

  def filename(self) -> str:
    return self._filename

  def getparsepos(self) -> Sequence[ParsePos]:
    """where are we?"""
    pos = ParsePos(filename=self._filename,line=self._line,col=self._col)
    return [pos]


class TokBufTokenizer:
  """A Tokenizer implementation with TokBuf input"""
  __slots__ = ('tokbuf','tokinfo','tok','_toklen','s','_wslines','_wscols','_wslen','stack')
  def __init__(self, tokbuf: TokBuf, tokinfo: TokenInfo):
    self.tokbuf: TokBuf = tokbuf
    self.tokinfo: TokenInfo = tokinfo
    self.tok: int = -1
    self._toklen: int = 0
    self.s: str = ''
    self._wslines: int = 0
    self._wscols: int = 0
    self._wslen: int = 0
    self.stack: list[int] = [0]

  def getTok(self, state: int) -> int:
    """get the token considered accepted by the state, returns -1 for none"""
    return self.tokinfo.tokens[state]

  def isWs(self, tok: int) -> bool:
    """Let the caller know if the provided token is considered whitespace"""
    return self.tokinfo.isws[tok]

  def nextState(self, state: int, c: int) -> int:
    """Given a state, and a character, find the transition and return the next state, or -1 for none"""
    curTransition: int = self.tokinfo.transitionOffset[state]
    endTransition: int = self.tokinfo.transitionOffset[state+1]
    while curTransition < endTransition:
      to: int = self.tokinfo.transitions[curTransition]
      pairCount: int = self.tokinfo.transitions[curTransition+1]
      for i in range(pairCount):
        low: int = self.tokinfo.transitions[curTransition+2+i*2]
        high: int = self.tokinfo.transitions[curTransition+2+i*2+1]
        if c >= low and c <= high:
          return to
      curTransition += 2+pairCount*2
    return -1

  def push(self, tokset: int) -> None:
    self.stack.append(tokset)

  def pop(self) -> None:
    if not len(self.stack):
      return
    del self.stack[-1]

  def gotots(self, tokset: int) -> None:
    self.stack[-1] = tokset

  def peek(self) -> int:
    if self.tok != -1:
       return self.tok
    while True:
      curSection: int = self.stack[-1]
      if self.tok != -1:
        if self.isWs(self.tok):
          tokstartline: int = self.tokbuf.line()
          tokstartcol: int = self.tokbuf.col()
          self.tokbuf.discard(self._toklen)
          tokendline: int = self.tokbuf.line()
          tokendcol: int = self.tokbuf.col()
          if tokendline > tokstartline:
            self._wslines += (tokendline - tokstartline)
            self._wscols = tokendcol - 1;
          else:
            self._wscols += (tokendcol - tokstartcol)
          self._wslen += self._toklen
        else:
          self.tokbuf.discard(self._toklen)
          self._wslines = 0
          self._wscols = 0
          self._wslen = 0
        self.tok = -1
        self._toklen = 0
      state: int = self.nextState(0, curSection)
      tok: int = -1
      toklen: int = -1
      length: int = 0
      while state != -1:
        t: int = self.getTok(state)
        if t != -1:
          tok = t;
          toklen = length
        c: int = self.tokbuf.peekc(length)
        state = self.nextState(state, c)
        length += 1
      self.tok = tok
      self._toklen = toklen
      if self.tok != -1:
        tokActions: int = (self.tok*self.tokinfo.sectionCount * 2) + curSection * 2
        tokenaction: int = self.tokinfo.tokenaction[tokActions]
        actionarg: int = self.tokinfo.tokenaction[tokActions+1]
        if tokenaction == 1:
          self.push(actionarg)
        elif tokenaction == 2:
          self.pop()
        elif tokenaction == 3:
          self.gotots(actionarg)
      if self.tok == -1 or not self.isWs(self.tok):
        break
    return self.tok

  def discard(self) -> None:
    if self.tok == -1:
      self.peek()
    if self.tok != -1:
      self.tokbuf.discard(self._toklen)
      self.tok = -1
      self._toklen = 0
      self._wslines = 0
      self._wscols = 0
      self._wslen = 0

  def length(self) -> int:
    return self._toklen

  def tokstr(self) -> str:
    if self.tok == -1:
      return ''
    if self.tok != -1:
      s = StringIO()
      for i in range(self._toklen):
        s.write(chr(self.tokbuf.peekc(i)))
      self.s = s.getvalue()
    return self.s

  def wslines(self) -> int:
    return self._wslines

  def wscols(self) -> int:
    return self._wscols;

  def wslen(self) -> int:
    return self._wslen;

  def line(self) -> int:
    return self.tokbuf.line()

  def col(self) -> int:
    return self.tokbuf.col()

  def filename(self) -> str:
    return self.tokbuf.filename()

  def tokid2str(self, tok: int) -> str:
    if tok < 0 or tok >= self.tokinfo.tokenCount:
      return  "?"
    return self.tokinfo.tokenstr[tok]

  def getparsepos(self) -> Sequence[ParsePos]:
    return self.tokbuf.getparsepos()

