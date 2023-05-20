#!/usr/bin/env python3
# Provided by parsergenerator - free to use under MIT license
from typing import Sequence, Callable, IO, Protocol, Any, Optional
from tok import ParsePos, Tokenizer, Token

ACTION_SHIFT: int = 0
ACTION_REDUCE: int = 1
ACTION_STOP: int = 2

class ParseInfo:
  __slots__ = ('nstates','actions','actionstart','prod0','nproductions','productions','productionstart','start','parse_error','nonterminals','reduce','extra_t','stack_t')
  def __init__(self, ns: Any):
    self.nstates: int = ns.nstates
    self.actions: Sequence[int] = ns.actions
    self.actionstart: Sequence[int] = ns.actionstart
    self.prod0: int = ns.PROD_0
    self.nproductions: int = ns.nproductions
    self.productions: Sequence[int] = ns.productions
    self.productionstart: Sequence[int] = ns.productionstart
    self.start: int = ns.START
    self.parse_error: int = ns.PARSE_ERROR
    self.nonterminals: Sequence[str] = ns.nonterminals
    self.reduce: Callable = ns.reduce
    self.extra_t: type = ns.extra_t
    self.stack_t: type = ns.stack_t

class Parser:
  __slots__ = ('parseinfo','extra','verbosity','reduce','fout')
  def __init__(self, parseinfo: ParseInfo, verbosity: int, fout: Optional[IO]):
    self.parseinfo: ParseInfo = parseinfo
    self.verbosity: int = verbosity
    self.extra = parseinfo.extra_t()
    self.reduce: Callable = parseinfo.reduce
    self.fout: Optional[IO] = fout

  def parse(self, toks: Tokenizer) -> bool:
    states: list[int] = []
    values: list[Any] = []
    inputqueue: list[tuple[int,Any]] = []
    states.append(0)
    values.append(self.parseinfo.stack_t())
    tok: int = -1
    inputnum: int = 0
    err: Optional[str] = None
    while len(states) > 0:
      if self.verbosity and self.fout:
        self.fout.write("lrstates = {states}\n".format(states=states))
      stateno = states[-1]
      if len(inputqueue) == 0:
        inputnum += 1
        nxt: int = toks.peek()
        t = self.parseinfo.stack_t()
        t.tok = Token(toks.tokstr(),toks.filename(),toks.wslines(), toks.wscols(),toks.wslen(),toks.line(),toks.col(),nxt)
        toks.discard()
        inputqueue.append( (nxt,t) )
      tok = inputqueue[-1][0]
      if self.verbosity and self.fout:
        t = inputqueue[-1][1]
        if t.tok.s:
          self.fout.write("input (# {inputnum}) = {tok} {tokstr} = \"{tokname}\" - {filename}({line}:{col})\n".format(inputnum=inputnum, tok=tok, tokstr=toks.tokid2str(tok), tokname=t.tok.s, filename=t.tok.filename, line=t.tok.line, col=t.tok.col))
      actions: Sequence[int] = self.parseinfo.actions
      firstaction: int = self.parseinfo.actionstart[stateno]
      lastaction: int = self.parseinfo.actionstart[stateno+1]
      while firstaction != lastaction:
        action: int = actions[firstaction]
        didcaction: bool = False
        nextaction: int = firstaction
        nsymbols: int = 0
        if action == ACTION_SHIFT:
          shiftto: int = actions[firstaction+1];
          nsymbols = actions[firstaction+2];
          nextaction = firstaction+3+nsymbols;
          if tok in actions[firstaction+3:nextaction]:
            if self.verbosity and self.fout:
              self.fout.write("shift to {shiftto}\n".format(shiftto=shiftto))
            states.append(shiftto)
            values.append(inputqueue[-1][1])
            del inputqueue[-1]
            didaction = True
        elif action == ACTION_REDUCE or action == ACTION_STOP:
          reduceby: int = actions[firstaction+1];
          reducecount: int = actions[firstaction+2];
          nsymbols = actions[firstaction+3];
          nextaction = firstaction+4+nsymbols;
          if tok in actions[firstaction+4:nextaction]:
            reducebyp: int = reduceby-self.parseinfo.prod0
            if self.verbosity and self.fout:
              self.fout.write("reduce {reducecount} states by rule {ruleno} [".format(reducecount=reducecount, ruleno=reducebyp+1))
              production: Sequence[int] = self.parseinfo.productions[self.parseinfo.productionstart[reducebyp]:self.parseinfo.productionstart[reducebyp+1]]
              first: bool = True
              for sym in production:
                self.fout.write(" ");
                if sym >= self.parseinfo.start:
                  self.fout.write(self.parseinfo.nonterminals[sym-self.parseinfo.start])
                else:
                  self.fout.write(toks.tokid2str(sym))
                if first:
                  first = False
                  self.fout.write(" :");
              self.fout.write(" ] ? ... ")
            output = self.parseinfo.stack_t()
            output.tok = Token('',toks.filename(),toks.wslines(),toks.wscols(),toks.wslen(),toks.line(),toks.col(),self.parseinfo.productions[self.parseinfo.productionstart[reducebyp]])
            inputs = values[-reducecount:]
            if self.reduce(self.extra,reduceby,inputs,output,self.fout):
              if self.verbosity and self.fout:
                self.fout.write("YES\n");
              del states[len(states)-reducecount:]
              del values[len(values)-reducecount:]
              inputqueue.append( (reduceby,output) )
              didaction = True
            else:
              if self.verbosity and self.fout:
                self.fout.write("NO\n")
          if action == ACTION_STOP and didaction:
            if self.verbosity and self.fout:
              self.fout.write("ACCEPT\n")
            return True
        if didaction:
          break
        firstaction = nextaction
      if firstaction == lastaction:
        if tok == self.parseinfo.parse_error:
          if len(states) == 0:
            if self.verbosity and self.fout:
              self.fout.write("NO ACTION AVAILABLE, FAIL\n")
            return False
          else:
            if self.verbosity and self.fout:
              self.fout.write('during error handling, popping lr state {state}\n'.format(state=states[-1]))
            del states[-1]
        else:
          # report the error for verbose output, but add error to the input, hoping for a handler, and carry on
          posstack = toks.getparsepos()
          t = inputqueue[-1][1]
          if err and self.fout:
            self.fout.write("{filename}({line}:{col}) : error : {err}\n".format(filename=t.tok.filename,line=t.tok.line,col=t.tok.col,err=err))
            for pos in posstack:
              self.fout.write("  at {filename}({line}:{col})\n".format(filename=pos.filename,line=pos.line,col=pos.col))
          elif self.fout:
            self.fout.write("{filename}({line}:{col}) : parse error : input (# {inputnum}) = {tok} {tokstr} = \"{tokname}\" - \n".format(filename=t.tok.filename, line=t.tok.line, col=t.tok.col, inputnum=inputnum, tok=tok, tokstr=toks.tokid2str(tok), tokname=t.tok.s))
            for pos in posstack:
              self.fout.write("  at {filename}({line}:{col})\n".format(filename=pos.filename, line=pos.line, col=pos.col))
          t = self.parseinfo.stack_t()
          t.tok = Token('','',0,0,0,0,0,self.parseinfo.parse_error)
          inputqueue.append( (self.parseinfo.parse_error,t) )
          toks.discard()
    return True
