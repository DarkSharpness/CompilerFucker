parser grammar MxParser;

options {
    tokenVocab = MxLexer;
}


file_input: 
    definition* EOF  ;
definition:
    function_definition # func_def  |
    class_definition    # class_def |
    variable_definition # var_def   ;


/* Function part. */
function_definition : function_argument '(' function_param_list? ')' block_stmt;
function_param_list : function_argument (',' function_argument)*;
function_argument   : typename Identifier;

/* Class part.  */
class_definition    : 'class' Identifier '{' class_content* '}' ';';
class_ctor_function : Identifier '(' ')' block_stmt;
class_content       :
    variable_definition # class_variable |
    function_definition # class_function |
    class_ctor_function # class_construct;

/* Basic statement.  */
stmt:
    simple_stmt         # expr      |
    branch_stmt         # branch    |
    loop_stmt           # loop      |
    flow_stmt           # flow      |
    variable_definition # var_defs  |
    block_stmt          # block     ;


/* Simple? */
block_stmt  : '{' stmt* '}' ;
simple_stmt : expr_list? ';';


/* Branch part. */
branch_stmt: if_stmt else_if_stmt* else_stmt?   ;
if_stmt     : 'if'      '(' expression ')' stmt ;
else_if_stmt: 'else if' '(' expression ')' stmt ;
else_stmt   : 'else'                       stmt ;


/* Loop part */
loop_stmt   : for_stmt | while_stmt ;
for_stmt    :
    'for' '(' 
        start       = expression? ';' 
        condition   = expression? ';'
        step        = expression? 
    ')' stmt;
while_stmt  : 'while' '(' expression ')' stmt  ;


/* Flow control. */
flow_stmt: ('continue' | 'break' | ('return' expression?)) ';';


/* Variable part. */
variable_definition :
    typename init_stmt (',' init_stmt)* ';';
init_stmt: Identifier ('=' expression)?;


/* Expression part */
expr_list   : expression /*(',' expression)* */  ;
expression  :
  '(' l = expression    op = ')'
	| l = expression    op = '['    expr_list   ']'
	| l = expression    op = '('    expr_list?  ')'
	| l = expression    op = '.'    Identifier
	| l = expression    op = '++'
	| l = expression    op = '--'
    |                   op = 'new'  typename
	| <assoc = right>   op = '++'   r = expression
	| <assoc = right>   op = '--'   r = expression
	| <assoc = right>   op = '~'    r = expression
	| <assoc = right>   op = '!'    r = expression
	| <assoc = right>   op = '+'    r = expression
	| <assoc = right>   op = '-'    r = expression
	| l = expression    op = ('*'  | '/'  | '%')        r = expression
	| l = expression    op = ('+'  | '-')               r = expression
	| l = expression    op = ('<<' | '>>')              r = expression
	| l = expression    op = ('<=' | '>=' | '<' | '>')  r = expression
	| l = expression    op = ('==' | '!=')              r = expression
	| l = expression    op = '&'    r = expression
	| l = expression    op = '^'    r = expression
	| l = expression    op = '|'    r = expression
	| l = expression    op = '&&'   r = expression
	| l = expression    op = '||'   r = expression
	| <assoc = right>   v = expression  op = '?'    l = expression ':'  r = expression
	| <assoc = right>   l = expression  op = '='    r = expression
	| literal_constant
	| Identifier;


/* Basic part.  */
typename            : (BasicTypes | Identifier) ('[' Number ']')*  ('[' ']')* ;
literal_constant    : Number | Cstring | Null | True | False;

