grammar CoLa;


/* Parser rules */

program : struct_decl* var_decl* function* EOF ;

struct_decl : ('struct' || 'class') name=Identifier '{' field_decl* '}' ';' ;

typeName : VoidType    #nameVoid
         | BoolType    #nameBool
         | IntType     #nameInt
         | DataType    #nameData
         | Identifier  #nameIdentifier
         ;

type : name=typeName      #typeValue
     | name=typeName '*'  #typePointer
     ;

field_decl : type names+=Identifier (',' names+=Identifier) ';' ;

var_decl : type names+=Identifier (',' names+=Identifier) ';' ;

/* Type check for inline function calls:
 *   - start function call with empty type environment (maybe retain locals if not passed)
 *   - end of function call takes type of returned pointer(s) from function environment (+ maybe retained locals)
 */

function : (modifier=Inline)? returnType=type name=Identifier '(' args=argDeclList ')' '{' var_decl* statement* '}'
         | modifier=Extern returnType=type name=Identifier '(' args=argDeclList ')' ';'
         ;

argDeclList : (argTypes+=type argNames+=Identifier (',' argTypes+=type argNames+=Identifier))? ;

block : statement           #blockStmt
      | '{' statement* '}'  #blockBlock
//      | '{' var_decl* statement* '}'  #blockBlock
      ;

statement : 'if' '(' expression ')' block ('else' block)?  #stmtIf
          | 'while' '(' expression ')' block               #stmtWhile
          | 'do' block 'while' '(' expression ')' ';'      #stmtDo
          | 'choose' block block?                          #stmtChoose
          | 'loop' block                                   #stmtLoop
          | 'atomic' block                                 #stmtAtomic
          | command ';'                                    #stmtCom
          ;

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
        | lhs=Identifier '=' 'malloc'          #cmdMallo
        | 'assume' '(' expr=expression ')'     #cmdAssume
        | 'assert' '(' expr=invariant ')'      #cmdAssert
        | '@invariant' '(' expr=invariant ')'  #cmdInvariant
        | name=Identifier '(' argList ')'      #cmdCall
        | 'continue'                           #cmdContinue
        | 'break'                              #cmdBreak
        | 'return' expr=expression?            #cmdReturn
        | cas                                  #cmdCas
        ;

argList : arg+=Identifier (',' arg+=Identifier)*;

cas : 'CAS' '(' dst=expression ',' cmp=expression ',' src=expression ')' ;
// TODO: dCAS, or general cas with multiple params

binop : Eq   #opEq
      | Neq  #opNeq
      | Lt   #opLt
      | Lte  #opLte
      | Gt   #opGt
      | Gte  #opGte
      | And  #opAnd
      | Or   #opOr
      ;

value : Null   #valueNull
      | True   #valueTrue
      | False  #valueFalse
      | Ndet   #valueNDet
      | Empty  #valueEmpty
      ;

expression : name=Identifier                        #exprIdentifier
           | value                                  #exprValue
           | '(' expr=expression ')'                #exprParens
           | Neg expr=expression                    #exprNegation
           | lhs=expression binop rhs=expression    #exprBinary
           | expr=expression '->' field=Identifier  #exprDeref
           | cas                                    #exprCas
           ;

invariant : expr=expression                   #invExpr
          | 'active' '(' expr=expression ')'  #invActive 
          ;


/* Lexer rules */

Inline : 'inline' ;
Extern : 'extern' ;

VoidType : 'void' ;
DataType : 'data_t' ;
BoolType : 'bool' ;
IntType  : 'int' ;

Null  : 'NULL' ;
Empty : 'EMPTY' ;
True  : 'true' ;
False : 'false' ;
Ndet  : '*' ;

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

WS    : [ \t\r\n]+ -> skip ; // skip spaces, tabs, newlines

COMMENT      : '/*' .*? '*/' -> channel(HIDDEN) ;
LINE_COMMENT : '//' ~[\r\n]* -> channel(HIDDEN) ;

UNMATCHED          : .  -> channel(HIDDEN);
