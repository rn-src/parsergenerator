# parsergenerator
A tokenizer and parser generator, implements infinite sub-parse tree restrictions

## Building - Windows
_If you don't like VS 14 or 32 bit, you can use whatever windows platform you prefer_
```
%comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
cd src/tok
nmake -f nMakefile.msc
cd ../parse
nmake -f nMakefile.msc
```

## Building - Linux
```
cd src/tok
make
cd ../parse
make
```

## Creating a Complete Parser
* Create a tokenizer file;  see [The Tokenizer File](#the-tokenizer-file) for information about the tokenizer file format.
* Create a parer file; see [The Grammar File](#the-grammar-file) for more information about the grammar file format.
* Run `tokenizer` and `parser` on the the files to generate the output headers.

The output of both `tokenizer` and `parser` are header files with state that can be used in tokenizer and parser algorihtms.
See _Creating a parser_ for the details of how to use the outputs. `parser` also outputs a `reduce` function that contains
the semantic actions provided in the grammar file.  To put it all together, several steps are needed.

* Take a look at [the calculator sample](samples/calc/calculator.cpp) to see how it is done;it only takes 40 lines.
* Or better yet, copy and modify.

## Running the tokenizer

`tokenizer` _lexer.L_

The output file will be _lexerL.h_

```
--py             Output tokenizer code in Python
--c              Output tokenizer code in C
```

# Running the parser generator

`parser` _parser.G_

The output file will be _parserG.h_

```
-v               Verbose output. Prints the state graph, transitions, and reductions in a readable form.
-vv              Really verboe output.
-vvv             Super verbose output. Prints internal details of the algorithms are output as they run.
--no-pound-line  Omit the line directives from the C output, to debug the C out code instead of the G file.
--minnt=X        Make the first token value X.  Useful if you find the token values not to your liking.
--timed          Output some timing statistics.
```

## The Tokenizer File

See [The tokenizer sample](samples/L/toktest.L) or [the calculator example](samples/calc/calculator.L) for examples.

* Tokens are one to a line, but can be divided into sections.
* The format is action, indicator, name, regex.  Action and indicdator are optional.
* Indicator can be one of
  * `~` : The token is whitespace; the parser knows to ignore these tokens.
  * `=` : The token is not output to the parser, rather it is a named regular expression that can be used in other regular expressions using {}.
* Actions are taken when the token is parsed, it is one of
  * `push` _state_ : The parser saves the current section, and uses the tokens from section _state_.
  * `pop`  _state_ : The parser returns to the previous saved section.
  * `goto` _state_ : The parser starts using section _state_; the current section is not saved.
* The default section is used initially by the parser; therefore it must have at least one token or the tokenizer is invalid.
* `section` _name_ declared a named section.  All tokens are part of this section until the next section declaration.

## The Grammar File
Grammar files consist of a number of rules.  Each rule is either a typedef rule, a grammar rule, or a restrict rule.  For examples, take a
look at [the calculator sample](samples/calc/calculator.G), [the finite grammar](samples/G/finite.G), [the infinite grammar](samples/G/infinite.G).

### Grammar - Typedef rules
Typedef rules declare nonterminal types in the semantic actions of the grammar rules.  Since the output C, types must be a C/C++ types.

* `TYPEDEFTOK` _Type_ nonterminal [nonterminal...] `;`

### Grammar - Grammar rules
Grammar rules are CFG rules in BNF form, the format is:

[`REJECTABLE`] _nonterminal_ `:` [_terminal_  [_terminal_...]] [`{ _semantic action_ }`] `;`

Empty productions are grammar rules that have no terminals.  Empty productions are technically possible, but discouraged for all but the experts because of the problems you will encounter attempting to produce a working grammar.

`REJECTABLE` supresses reporting of reduce/reduce conflicts if all reduce productions are marked `REJECTABLE`.
At runtime, reductions are made by running the semantic actions of each production.  They can be rejected though; the first
to return a nonzero value from its semantic action is reduced, and the remaining reduces are ignored.  The productions are
tried in the order they are declared. If all producions return zero, the result is a parser error.

The semantic action is expected in the output language, which is C.  If the semantic action returns zero, the reduction of the production
is aborted and a parse error is produced.  Within the semantic action code, certain symbols get substituted:
* `$$` : This is a variable of the nonterminal; the type is the type declared in `TYPEDEFTOK`.
* `$`_N_ : _N_ is an integer.  This is a variable of the _N_th symbol of the terminal.  For nonterminals; the type is the type
declared in `TYPEDEFTOK`.  For terminals, the type is `token_t`. See [the calculator sample](samples/calc/calculator.G) for information
on how to get the string from the token.

### Grammar - Restrict rules

Restrict rules are optional, but they make working with grammars much more pleasant if you understand them.  If you already
understand sub-parse trees and parse forests, you can think of it as a way to eliminate sub-parse trees from a nondeterministic
parse, except an algorithm figures take a look at how to produce a deterministic LR table instead.  It was my masters thesis,
so I won't bother to explain how it works here here, but I will provide a link once one becomes available.

* Associativity and Precedence : Look at [the calculator sample](samples/calc/calculator.G), [the finite grammar](samples/G/finite.G).
* Restrict rules: Look at [the finite grammar](samples/G/finite.G).
* Infinite restrict rules : Mainly used for disambiguating the danglig-else case, take a look at [the infinite grammar](samples/G/infinite.G).
