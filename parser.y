%{
#include "cognac.h"
#include <stdlib.h>

ast* ast_join(ast* a, ast* b)
{
  if (!a) return b;
  ast* ptr = a;
  while (ptr->next) ptr=ptr->next;
  ptr->next = b;
  return a;
}

ast* alloc_ast(token_type type, value_type val_type, void* data, size_t line)
{
  ast* a = malloc(sizeof(*a));
  *a = (ast){.type=type, .val_type=val_type, .data=data, .line=line, .next=NULL};
  return a;
}
%}

%locations

%union {
  char* text;
  struct ast* tree;
  // TODO special type for tokens so we can have line numbers.
}

%token <text> NUMBER
       <text> IDENTIFIER
       <text> STRING
       <text> SYMBOL
       DEFINE
       LET
       SET
       ';'
       '('
       ')'
       ;

%type <tree> STATEMENT
      <tree> EXPRESSION
      <tree> TOKEN
      ;

%start ENTRY;
// TODO: Declarations need to be compiled here.
%%

ENTRY:
    EXPRESSION { full_ast = $1; }
    ;

EXPRESSION:
    STATEMENT ';' EXPRESSION { $$ = ast_join($1, $3); }
  | STATEMENT                { $$ = $1;               }
  ;

STATEMENT:
    TOKEN STATEMENT { $$ = ast_join($2, $1); }
  | %empty          { $$ = NULL;             } ;

TOKEN: // Tokens should be converted to ast nodes in the lexer.
    IDENTIFIER         { $$ = alloc_ast(identifier, any, $1, 0); }
  | '(' EXPRESSION ')' { $$ = alloc_ast(value, block,    $2, 0); }
  | NUMBER             { $$ = alloc_ast(value, number,   $1, 0); }
  | STRING             { $$ = alloc_ast(value, string,   $1, 0); }
  | SYMBOL             { $$ = alloc_ast(value, symbol,   $1, 0); }
  | DEFINE IDENTIFIER  { $$ = alloc_ast(define, any,     $2, 0); }
  | LET IDENTIFIER     { $$ = alloc_ast(let, any,        $2, 0); }
  | SET IDENTIFIER     { $$ = alloc_ast(set, any,        $2, 0); }
  ;

%%
