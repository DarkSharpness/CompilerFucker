parser grammar MxParser;

options {
    tokenVocab = MxLexer;
}


file_input: definition* EOF  ;
definition:
    function_definition # func_def  |
    class_definition    # class_def |
    variable_definition # var_def   ;


/* Class and function part. */
class_definition: 'class' Identifier '{' class_content* '}' ';';
class_content   :
    variable_definition # class_variable    |
    function_definition # class_function    |
    class_ctor_function # class_construct   ;
function_definition : typename Identifier '(' function_param_list? ')' block_stmt;
class_ctor_function : Identifier '(' ')' block_stmt;
function_param_list :
    function_argument (',' function_argument)*;
function_argument: typename Identifier;


/* Some stuff.  */
suite       : stmt | block_stmt ;
block_stmt  : '{' stmt* '}'     ;


/* Basic statement.  */
stmt:
    simple_stmt         # expr      |
    branch_stmt         # branch    |
    loop_stmt           # loop      |
    flow_stmt           # flow      |
    variable_definition # var_def   |
    block_stmt          # block     ;


/* Simple? */
simple_stmt : expr_list? ';'        ;


/* Branch part. */
branch_stmt: if_stmt else_if_stmt* else_stmt?   ;
if_stmt     : 'if'      '(' expression ')' ;
else_if_stmt: 'else if' '(' expression ')' suite;
else_stmt   : 'else'    '(' expression ')' suite;


/* Loop part */
loop_stmt   : for_stmt | while_stmt ;
for_stmt    :
    'for' '(' 
        start       = expression? ';' 
        condition   = expression? ';'
        step        = expression? 
    ')' suite;
while_stmt  : 'while' '(' expression ')' suite  ;


/* Flow control. */
flow_stmt: ('continue' | 'break' | ('return' expression?)) ';';


/* Variable part. */
variable_definition :
    typename init_stmt (',' init_stmt)* ';';
init_stmt: Identifier;


/* Expression part */
expr_list   : expression (',' expression)*  ;
expression  :
  '(' l = expression    op = ')'
	| l = expression    op = '['    v = expr_list   ']'
	| l = expression    op = '('    v = expr_list?  ')'
	| l = expression    op = '.'    v = Identifier
	| l = expression    op = '++'
	| l = expression    op = '--'
    |                   op = 'new'  v = typename
	| <assoc = right>   op = '++'   r = expression
	| <assoc = right>   op = '--'   r = expression
	| <assoc = right>   op = '~'    r = expression
	| <assoc = right>   op = '!'    r = expression
	| <assoc = right>   op = '+'    r = expression
	| <assoc = right>   op = '-'    r = expression
	| l = expression    op = '*'  | '/'  | '%'          r = expression
	| l = expression    op = '+'  | '-'                 r = expression
	| l = expression    op = '<<' | '>>'                r = expression
	| l = expression    op = '<=' | '>=' | '<' | '>'    r = expression
	| l = expression    op = '==' | '!='                r = expression
	| l = expression    op = '&'    r = expression
	| l = expression    op = '^'    r = expression
	| l = expression    op = '|'    r = expression
	| l = expression    op = '&&'   r = expression
	| l = expression    op = '||'   r = expression
	| <assoc = right>   v = expression  op = '?'    l = expression ':'  r = expression
	| <assoc = right>   l = expression  op = '='    r = expression
	| v = literal_constant
	| v = Identifier;


/* Basic part.  */
typename: (BasicTypes | Identifier) ('[' Number ']')*  ('[' ']')* ;
literal_constant: Number | Cstring | Null | True | False;

