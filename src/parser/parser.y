%token_type {mndb_parser_token_t *}
%token_prefix MNDB_PARSER_TOKEN_ID_

%extra_argument {_mndb_parser_ctx_t *ctx}

%include {
#include <assert.h>

#include "parser/mndb-parser.h"
#include "parser/mndb-ast.h"
}


%nonassoc SL_COMMENT.
%nonassoc ML_COMMENT.
%nonassoc LPAREN.
%nonassoc RPAREN.
%nonassoc ATOM.
%nonassoc VAR.
%nonassoc INT.
%nonassoc FLOAT.

%left PLUS.
%left MINUS.

%left MOD.
%left DIV.
%left MUL.

%right POW.

%token_destructor {
  mndb_parser_token_free($$);
}

%syntax_error {
}

start ::= stmts. {
}

stmts ::= stmts query. {
}

stmts ::= stmts retract. {
}

stmts ::= stmts assert. {
}

stmts ::= .

const ::= ATOM.
const ::= FLOAT.
const ::= INT.
const ::= REGEXP.

term_list ::= term_list COMMA term.
term_list ::= term.

list ::= RBRACK LBRACK.
list ::= RBRACK term_list LBRACK.
list ::= RBRACK term_list BAR VAR LBRACK.

pair ::= term COLON term.

pair_list ::= pair_list COMMA pair.
pair_list ::= pair.

map ::= RBRACK pair_list LBRACK.
map ::= RBRACK pair_list BAR VAR LBRACK.

term ::= const.
term ::= VAR.
term ::= list.
term ::= map.


is_expr ::= INT.
is_expr ::= FLOAT.
is_expr ::= is_expr PLUS is_expr.
is_expr ::= is_expr MINUS is_expr.
is_expr ::= is_expr DIV is_expr.
is_expr ::= is_expr MUL is_expr.
is_expr ::= is_expr MOD is_expr.
is_expr ::= MINUS is_expr.
is_expr ::= POW is_expr.
is_expr ::= LPAREN is_expr RPAREN.

unif ::= term UNIFY term.

fact ::= ATOM LPAREN term_list RPAREN.
fact ::= term IS is_expr.
fact ::= unif.

head ::= fact.

conj ::= fact.
conj ::= conj COMMA fact.

disj ::= conj SEMIC fact.

body ::= conj.
body ::= disj.
body ::= body conj.
body ::= body disj.

rule ::= head IMPLY body.

clause ::= fact.
clause ::= rule.

query ::= body QUERY.
retract ::= fact RETRACT.
assert ::= clause ASSERT.

