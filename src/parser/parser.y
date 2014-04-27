%token_type {mndb_parser_token_t *}
%default_type {mndb_ast_t *}
%token_prefix MNDB_PARSER_TOKEN_ID_

%extra_argument {_mndb_parser_ctx_t *ctx}

%include {
#include <assert.h>

#include "parser/mndb-parser.h"
#include "parser/mndb-ast.h"
}

%nonassoc SL_COMMENT.
%nonassoc ML_COMMENT.


%left PLUS.
%left MINUS.

%left MOD.
%left DIV.
%left MUL.

%right POW.

%left SEMIC.
%left COMMA.

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
  s = mndb_ast_new(MNDB_AST_TYPE_ROOT);
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

term_list(ll) ::= term_list(rl) COMMA term(t). {
  ll = mndb_ast_add_child(rl, t);
}

term_list(l) ::= term(t). {
  l = mndb_ast_new(MNDB_AST_TYPE_NONE /* change uptree */);
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
  mndb_ast_set_token(va, v);

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
  pl = mndb_ast_new(MNDB_AST_TYPE_NONE /* change uptree */);
  pl = mndb_ast_add_child(pl, p);
}

map(m) ::= RBRACK pair_list(pl) LBRACK. {
  m = pl;
  mndb_ast_set_type(m, MNDB_AST_TYPE_MAP);
}

map(m) ::= RBRACK pair_list(pl) BAR(b) VAR(v) LBRACK. {
  m = pl;
  mndb_ast_set_type(m, MNDB_AST_TYPE_MAP);

  mndb_ast_t *ba = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(ba, b);

  mndb_ast_t *va = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(va, v);

  m = mndb_ast_add_child(m, ba);
  m = mndb_ast_add_child(m, va);
}

term(t) ::= const(c). {
 t = c;
}

term(t) ::= VAR(v). {
  t = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(t, v);
}

term(t) ::= list(l). {
 t = l;
}

term(t) ::= map(m). {
 t = m;
}

is_expr(ie) ::= INT(i). {
  ie = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(ie, i);
}

is_expr(ie) ::= FLOAT(f). {
  ie = mndb_ast_new(MNDB_AST_TYPE_TOKEN);
  mndb_ast_set_token(ie, f);
}

is_expr(ie) ::= is_expr(lo) PLUS(o) is_expr(ro). {
  ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= is_expr(lo) MINUS(o) is_expr(ro).{
  ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= is_expr(lo) DIV(o) is_expr(ro).{
  ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= is_expr(lo) MUL(o) is_expr(ro).{
  ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= is_expr(lo) MOD(o) is_expr(ro).{
  ie = mndb_ast_new(MNDB_AST_TYPE_BIN_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, lo);
  ie = mndb_ast_add_child(ie, ro);
}

is_expr(ie) ::= MINUS(o) is_expr(op).{
  ie = mndb_ast_new(MNDB_AST_TYPE_UNARY_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, op);
}

is_expr(ie) ::= is_expr(op) POW(o). {
  ie = mndb_ast_new(MNDB_AST_TYPE_UNARY_OP);
  mndb_ast_set_token(ie, o);

  ie = mndb_ast_add_child(ie, op);
}

is_expr(lie) ::= LPAREN is_expr(rie) RPAREN. {
  lie = rie;
}

unif(u) ::= term(lt) UNIFY(t) term(rt). {
  u = mndb_ast_new(MNDB_AST_TYPE_NONE /* change uptree */);

  mndb_ast_set_token(u, t);
  u = mndb_ast_add_child(u, lt);
  u = mndb_ast_add_child(u, rt);
}

fact(f) ::= PRED(a) term_list(tl) RPAREN. {
  f = tl;

  mndb_ast_set_type(f, MNDB_AST_TYPE_FACT);
  mndb_ast_set_token(f, a);
}

fact(f) ::= term(t) IS(is) is_expr(ie). {
  f = mndb_ast_new(MNDB_AST_TYPE_FACT);
  mndb_ast_set_token(f, is);

  f = mndb_ast_add_child(f, t);
  f = mndb_ast_add_child(f, ie);
}

fact(f) ::= unif(u). {
  f = u;
  mndb_ast_set_type(f, MNDB_AST_TYPE_FACT);
}

head(h) ::= PRED(a) term_list(tl) RPAREN. {
  h = tl;

  mndb_ast_set_type(h, MNDB_AST_TYPE_HEAD);
  mndb_ast_set_token(h, a);
}

conj(fs) ::= fact(f1) COMMA(t) fact(f2). {
  fs = mndb_ast_new(MNDB_AST_TYPE_FACTS);

  mndb_ast_set_token(fs, t);
  fs = mndb_ast_add_child(fs, f1);
  fs = mndb_ast_add_child(fs, f2);
}

disj(fs) ::= fact(f1) SEMIC(t) fact(f2). {
  fs = mndb_ast_new(MNDB_AST_TYPE_FACTS);

  mndb_ast_set_token(fs, t);
  fs = mndb_ast_add_child(fs, f1);
  fs = mndb_ast_add_child(fs, f2);
}

conj(rfs) ::= conj(lfs) COMMA fact(f). {
  rfs = mndb_ast_add_child(lfs, f);
}

conj(rfs) ::= conj(lfs) COMMA disj(f). {
  rfs = mndb_ast_add_child(lfs, f);
}

disj(rfs) ::= disj(lfs) SEMIC fact(f). {
  rfs = mndb_ast_add_child(lfs, f);
}

disj(rfs) ::= conj(lfs) SEMIC LPAREN conj(f) RPAREN. {
  rfs = mndb_ast_add_child(lfs, f);
}

facts(fs) ::= conj(c). {
  fs = c;
}

facts(fs) ::= disj(d). {
  fs = d;
}

body(b) ::= fact(f). {
  b = f;
}

body(b) ::= facts(fs). {
  b = fs;
}

rule(r) ::= head(h) IMPLY body(b). {
  r = mndb_ast_new(MNDB_AST_TYPE_RULE);

  r = mndb_ast_add_child(r, h);
  r = mndb_ast_add_child(r, b);
}

clause(c) ::= fact(f). {
  c = f;
}

clause(c) ::= facts(fs). {
  c = fs;
}

clause(c) ::= rule(r). {
  c = r;
}

query(q) ::= body(b) QUERY. {
  q = b;
  mndb_ast_set_type(q, MNDB_AST_TYPE_QUERY);
}

retract(r) ::= fact(f) RETRACT. {
  r = f;
  mndb_ast_set_type(r, MNDB_AST_TYPE_RETRACT);
}

assert(a) ::= clause(c) ASSERT. {
  a = mndb_ast_new(MNDB_AST_TYPE_ASSERT);
  a = mndb_ast_add_child(a, c);
}

