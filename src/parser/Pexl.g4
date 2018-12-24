grammar Pexl;


/* Parser rules */

program : option* var_decl* function* EOF;

// TODO: untagged and unmarked pointers
// struct Node {
//     key_t key;
//     boolean mark;
//     tag_t tag;
//     Node* next;
// };

option : '@name' name=Identifier   #optionName
       | '@linearizable'           #optionLinearizable
       | '@notlinearizable'        #optionNotLinearizable
       | '@taggedpointers'         #optionTaggedPointers
       | '@markedpointers'         #optionMarkedPointers
       | '@GC'                     #optionGC
       | '@MM'                     #optionMM
       | '@stack'                  #optionStack
       | '@queue'                  #optionQueue
       | '@set'                    #optionSet
       | '@smr hazard'             #optionHazardPointers
       | '@smr epoch'              #optionEpochBased
       | '@smr free'               #optionFreeList
       ;

var_decl : type name=Identifier ';' #variableDeclaration ;
type     : PointerType        #typePointer
         | BoolType           #typeBool
         | TagType            #typeTag
         | DataType           #typeData
         | VoidType           #typeVoid
         ;

function  : (isSummary='@summary')? returnType=type name=Identifier '(' (paramType=type paramName=Identifier)? ')' block ;

block     : '{' var_decl* statement* '}' ;

statement  : block                                                            #stmtBlock
           | Atomic block                                                     #stmtAtomic
           | While '(' True ')' block                                         #stmtWhile
           | If '(' condition ')' ifBlock=block ('else' elseBlock=block)?     #stmtIfThenElse
           | If '(' cas ')' ifBlock=block ('else' elseBlock=block)?           #stmtIfCasElse
           | Return value=var_or_imm ';'                                      #stmtReturnVarImm
           | Return ';'                                                       #stmtReturnVoid
           | Assume '(' condition ')' ';'                                     #stmtAssume
           | Continue ';'                                                     #stmtContinue
           | Break ';'                                                        #stmtBreak
           | Skip ';'                                                         #stmtSkip
           | linp                                                             #stmtLinearizationPoint
           | lhs=Identifier '=' Malloc ';'                                    #stmtMalloc
           | lhs=Identifier '=' Free ';'                                      #stmtFree
           | cas ';'                                                          #stmtCas
           | lhs=Identifier '=' rhs=var_or_imm ';'                            #stmtAssignToVarFromVarImm
           | lhs=Identifier '->' selector=node_field '=' rhs=var_or_imm ';'   #stmtAssignToSelFromVarImm
           | lhs=Identifier '=' rhs=Identifier '->' selector=node_field ';'   #stmtAssignToVarFromSel
           | lhs=identList '=' rhs=identList ';'                              #stmtAssignMultipleToVarFromVar
           | lhs=identList '=' rhs=Identifier '->' selector=fieldList ';'     #stmtAssignMultipleToVarFromSel
//         | lhs=Identifier '->' selector=fieldList '=' rhs=identList ';'     #stmtAssignMultipleToSelFromVar
           | lhs=Identifier '=' Havoc ';'                                     #stmtAssignToVarFromHavoc
           ;
cas        : Cas '(' dst=identList ',' cmp=casList ',' src=casList linp? ')'                               #casMultipleVar
           | Cas '(' dst=Identifier '->' selector=fieldList ',' cmp=casList ',' src=casList linp? ')'      #casMultipleSel
           ;

identList  : ident+=Identifier | '<' ident+=Identifier (',' ident+=Identifier)+ '>';
fieldList  : field+=node_field | '<' field+=node_field (',' field+=node_field)+ '>';
casList    : expr+=var_or_imm | '<' expr+=var_or_imm (',' expr+=var_or_imm)+ '>';
var_or_imm : ident=Identifier           #varimmVar
           | ident=Identifier '+' '1'   #varimmVarPlusOne
           | immediate                  #varimmImm
           ;
immediate  : True    #immediateTrue
           | False   #immediateFalse
           | Null    #immediateNull
           | Empty   #immediateEmpty
           ;
node_field : PointerField   #fieldPointer
           | DataField      #fieldDate
           | TagField       #fieldTag
           | MarkField      #fieldMark
           ;

condition  : cmp=comparison                         #conditionComparison
           | cmp+=comparison (And cmp+=comparison)+ #conditionConjunction
           | cmp+=comparison (Or cmp+=comparison)+  #conditionDisjunction
           ;
comparison : var=Identifier                                                  #conparisonVar
           | lhs=var_or_imm compop rhs=var_or_imm                            #conparisonVarImmVarImm
           | lhs=Identifier '->' selector=node_field compop rhs=var_or_imm   #conparisonSelVarImm
           | lhs=var_or_imm compop rhs=Identifier '->' selector=node_field   #conparisonVarImmSel
           ;
compop     : Eq    #cmpopEq
           | Neq   #cmpopNeq
           | Lt    #cmpopLt
           | Lte   #cmpopLte
           | Gt    #cmpopGt
           | Gte   #cmpopGte
           ;

linp : '@' ('[' cond=condition ']')? '{' func=Identifier(param=var_or_imm) '}' ;


/* Lexer rules */

PointerType   : 'Node*' ;
VoidType      : 'void' ;
DataType      : 'data_t' ;
BoolType      : 'bool' ;
TagType       : 'tag_t' ;

Malloc   : 'malloc' ;
Free     : 'free' ;
Return   : 'return' ;
While    : 'while' ;
If       : 'if' ;
Else     : 'else' ;
Atomic   : 'atomic' ;
Assume   : 'assume' ;
Continue : 'continue' ;
Break    : 'break' ;
Skip     : 'skip' ;
Cas      : 'CAS' ;

Null  : 'NULL' ;
Empty : 'EMPTY' ;
Havoc : 'HAVOC' ;
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

PointerField   : 'ptr' ;
TagField       : 'tag' ;
MarkField      : 'mark' ;
DataField      : 'data' ;

Identifier : [a-zA-Z][a-zA-Z0-9]* ;

WS    : [ \t\r\n]+ -> skip ; // skip spaces, tabs, newlines

COMMENT      : '/*' .*? '*/' -> channel(HIDDEN) ;
LINE_COMMENT : '//' ~[\r\n]* -> channel(HIDDEN) ;

UNMATCHED          : .  -> channel(HIDDEN);
