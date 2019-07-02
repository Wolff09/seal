grammar CoLa;


/* Parser rules */

program : opts* struct_decl* var_decl* function* EOF ;

opts : '#' ident=Identifier str=String  #option ;

struct_decl : ('struct' || 'class') name=Identifier '{' field_decl* '}' (';')? ;

typeName : VoidType    #nameVoid
         | BoolType    #nameBool
         | IntType     #nameInt
         | DataType    #nameData
         | Identifier  #nameIdentifier
         ;

type : name=typeName      #typeValue
     | name=typeName '*'  #typePointer
     ;

field_decl : type names+=Identifier (',' names+=Identifier)* ';' ;

var_decl : type names+=Identifier (',' names+=Identifier)* ';' ;

/* Type check for inline function calls:
 *   - start function call with empty type environment (maybe retain locals if not passed)
 *   - end of function call takes type of returned pointer(s) from function environment (+ maybe retained locals)
 */

function : (modifier=Inline)? returnType=type name=Identifier '(' args=argDeclList ')' body=scope //'{' var_decl* statement* '}' // TODO: use scope
         | modifier=Extern returnType=type name=Identifier '(' args=argDeclList ')' ';'
         ;

argDeclList : (argTypes+=type argNames+=Identifier (',' argTypes+=type argNames+=Identifier)*)? ;

block : statement  #blockStmt
      | scope      #blockScope
      ;

scope : '{' var_decl* statement* '}' ;

statement : 'choose' scope+                                                           #stmtChoose
          | 'loop' scope                                                              #stmtLoop
          | annotation? 'atomic' body=block                                           #stmtAtomic
          | annotation? 'if' '(' expr=expression ')' bif=block ('else' belse=block)?  #stmtIf
          | annotation? 'while' '(' expr=expression ')' body=block                    #stmtWhile
          | annotation? 'do' body=block 'while' '(' expr=expression ')' ';'           #stmtDo
          | annotation? command ';'                                                   #stmtCom
          ;

annotation : '@invariant' '('  invariant ')';

/* Simplifying expression may not yield the desired result.
 * It may not correlate temporary variables of @invariant and subsequent assume.
 * 
 * Another problem: if @invariant does not occur atomically together with assume, then the @invariant guarantees get lost.
 * ==> interpret @invariant as atomically attached to a command?
 *
 * Splitting is problematic with invariant, because one needs to spam it for all produced assumes
 * ==> just one global variable per expression? then all assumes can be done together.
 */

command : 'skip'                               #cmdSkip
        | lhs=expression '=' rhs=expression    #cmdAssign
        | lhs=Identifier '=' 'malloc'          #cmdMalloc
        | 'assume' '(' expr=expression ')'     #cmdAssume
        | 'assert' '(' expr=invariant ')'      #cmdAssert
        | 'angel' '(' expr=angelexpr ')'       #cmdAngel
        | name=Identifier '(' argList ')'      #cmdCall // TODO: support enter/exit
        | 'continue'                           #cmdContinue
        | 'break'                              #cmdBreak
        | 'return' expr=expression?            #cmdReturn
        | cas                                  #cmdCas
        ;

argList : (arg+=expression (',' arg+=expression)*)? ;

cas : 'CAS' '(' dst+=expression ',' cmp+=expression ',' src+=expression ')'
    | 'CAS' '(' '<' dst+=expression (',' dst+=expression)* '>' ','
                '<' cmp+=expression (',' cmp+=expression)* '>' ','
                '<' src+=expression (',' src+=expression)* '>' ',' ')'
    ;

binop : Eq   #opEq
      | Neq  #opNeq
      | Lt   #opLt
      | Lte  #opLte
      | Gt   #opGt
      | Gte  #opGte
      | And  #opAnd
      | Or   #opOr
      ;

value : Null    #valueNull
      | True    #valueTrue
      | False   #valueFalse
      | Ndet    #valueNDet
      | Empty   #valueEmpty
      | Maxval  #valueMax
      | Minval  #valueMin
      ;

expression : name=Identifier                        #exprIdentifier
           | value                                  #exprValue
           | '(' expr=expression ')'                #exprParens
           | Neg expr=expression                    #exprNegation
           | expr=expression '->' field=Identifier  #exprDeref
           | lhs=expression binop rhs=expression    #exprBinary
           | cas                                    #exprCas
           ;

invariant : 'active' '(' expr=expression ')'  #invActive
          | expr=expression                   #invExpr
          ;

angelexpr : 'choose'                            #angelChoose
          | 'active'                            #angelActive
          | 'contains' '(' name=Identifier ')'  #angelContains
          ;

/* Lexer rules */

Inline : 'inline' ;
Extern : 'extern' ;

VoidType : 'void' ;
DataType : 'data_t' ;
BoolType : 'bool' ;
IntType  : 'int' ;

Null   : 'NULL' ;
Empty  : 'EMPTY' ;
True   : 'true' ;
False  : 'false' ;
Ndet   : '*' ;
Maxval : 'MAX_VAL' ;
Minval : 'MIN_VAL' ;

Eq  : '==' ;
Neq : '!=' ;
Lt  : '<' ;
Lte : '<=' ;
Gt  : '>' ;
Gte : '>=' ;
And : '&&' ;
Or  : '||' ;

Neg : '!' ;

Identifier : [a-zA-Z][a-zA-Z0-9]* ;

String : '"' ~[\r\n]* '"'
       | '\'' ~[\r\n]* '\'' ;

WS    : [ \t\r\n]+ -> skip ; // skip spaces, tabs, newlines

COMMENT      : '/*' .*? '*/' -> channel(HIDDEN) ;
LINE_COMMENT : '//' ~[\r\n]* -> channel(HIDDEN) ;

// UNMATCHED          : .  -> channel(HIDDEN);
