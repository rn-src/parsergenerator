# ParserGenerator
A parser toolset for C and Python

- Tokenizer
  - Tokens can be marked as whitespace
  - Different tokens accepted depending on state.
  - Reusable component tokens.
  - Output in C or Python

- Parser
  - BNF format input
  - Left/Right/Non Associative disambiguation rules.
  - Precedence disambiguation rules.
  - Arbitrary grammar expansion disambiguation rules, any depth by regex (a solution to the dangling else ambiguity).
  - Nonterminal type-declarations and Semantic actions.
  - Output in C or Python

See [the wiki](https://github.com/rn-src/parsergenerator/wiki) for the documentation.
