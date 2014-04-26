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

start ::= stmts(s). {
  ctx->root = s;
}

stmts(ls) ::= stmts(rs) query(q). {
  ls = mndb_ast_add_child(rs, q);
}

stmts(ls) ::= stmts(rs) retract(r). {
  ls = mndb_ast_add_child(rs, r);
}

stmts(ls) ::= stmts(rs) assert(a). {
  ls = mndb_ast_add_child(rs, a);
}

stmts(s) ::= . {
  s = mndb_ast_new();
}




const(c) ::= ATOM(t). {
  c = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(c, t);
}

const(c) ::= FLOAT(t). {
  c = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(c, t);
}

const(c) ::= INT(t). {
  c = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(c, t);
}

const(c) ::= REGEXP(t). {
  c = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(c, t);
}

const(c) ::= DBLQ_STR(t). {
  c = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(c, t);
}

const(c) ::= SNGLQ_STR(t). {
  c = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(c, t);
}




term_list(ll) ::= term_list(rl) COMMA term. {
  ll = mndb_ast_add_child(rl, t);
}

term_list(l) ::= term(t). {
  l = mndb_ast_new(MNDB_AST_TYPE_INVALID /* change uptree */);
  l = mndb_ast_add_child(l, t);
}




list(l) ::= RBRACK LBRACK. {
  l = mndb_ast_new(MNDB_AST_TYPE_LIST);
}

list(l) ::= RBRACK term_list(tl) LBRACK. {
  l = tl;
  mndb_ast_set_type(l, MNDB_AST_TYPE_LIST);
}

list(l) ::= RBRACK term_list(tl) BAR(b) VAR(v) LBRACK. {
  l = tl;
  mndb_ast_set_type(l, MNDB_AST_TYPE_LIST);

  mndb_ast_t *ba = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(ba, b);

  mndb_ast_t *va = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(va, a);

  l = mndb_ast_add_child(l, ba);
  l = mndb_ast_add_child(l, va);
}




pair(p) ::= term(k) COLON term(v). {
  p = mndb_ast_new(MNDB_AST_TYPE_PAIR);

  p = mndb_ast_add_child(p, k);
  p = mndb_ast_add_child(p, v);
}




pair_list(lpl) ::= pair_list(rpl) COMMA pair(p). {
  lpl = mndb_ast_add_child(rpl, p);
}

pair_list(pl) ::= pair(p). {
  pl = mndb_ast_new(MNDB_AST_TYPE_INVALID /* change uptree */);
  pl = mndb_ast_add_child(pl, p);
}




map(m) ::= RBRACK pair_list(pl) LBRACK. {
  m = pl;
  mndb_ast_set_type(l, MNDB_AST_TYPE_MAP);
}

map(m) ::= RBRACK pair_list(pl) BAR(b) VAR(v) LBRACK. {
  m = pl;
  mndb_ast_set_type(m, MNDB_AST_TYPE_MAP);

  mndb_ast_t *ba = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(ba, b);

  mndb_ast_t *va = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(va, a);

  m = mndb_ast_add_child(m, ba);
  m = mndb_ast_add_child(m, va);
}

term(t) ::= const(c). {
 t = c;
}

term(t) ::= VAR(v). {
  mndb_ast_t *t = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(t, v);
}

term(t) ::= list(l). {
 t = l;
}

term(t) ::= map(m). {
 t = m;
}

is_expr(ie) ::= INT(i). {
  mndb_ast_t *ie = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(ie, i);
}

is_expr(ie) ::= FLOAT(f). {
  mndb_ast_t *ie = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(ie, f);
}

is_expr(ie) ::= is_expr(lo) PLUS(o) is_expr(ro). {
  mndb_ast_t *ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= is_expr(lo) MINUS(o) is_expr(ro).{
  mndb_ast_t *ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= is_expr(lo) DIV(o) is_expr(ro).{
  mndb_ast_t *ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= is_expr(lo) MUL(o) is_expr(ro).{
  mndb_ast_t *ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= is_expr(lo) MOD(o) is_expr(ro).{
  mndb_ast_t *ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= MINUS(o) is_expr(op).{
  mndb_ast_t *ie = mndb_ast_new(MNDB_AST_TYPE_UNARY_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, op);
}

is_expr(ie) ::= is_expr(op) POW(o). {
  mndb_ast_t *ie = mndb_ast_new(MNDB_AST_TYPE_UNARY_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, op);
}

is_expr(lie) ::= LPAREN is_expr(rie) RPAREN. {
  mndb_ast_t *lie = rie;
}

unif(u) ::= term(lt) UNIFY term(rt). {
  mndb_ast_t *u = mndb_ast_new(MNDB_AST_TYPE_UNIFY);

  u = mndb_ast_add_child(u, lt);
  u = mndb_ast_add_child(u, rt);
}

fact(f) ::= ATOM LPAREN term_list RPAREN. {
  mndb_ast_t *u = mndb_ast_new(MNDB_AST_TYPE_FACT);

  u = mndb_ast_add_child(u, lt);
  u = mndb_ast_add_child(u, rt);
}

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

