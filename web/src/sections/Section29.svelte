<h3 id="ebnf"><span class="num">29</span>EBNF (no left recursion) </h3>
<p>Notation: <code>&#123; x &#125;</code> zero or more, <code>[ x ]</code> optional, <code>( | )</code> grouping. Binary operators use iteration per precedence level instead of left-recursive rules; postfix forms are a loop over a primary. Whitespace and comments may appear between any two tokens.</p>
<pre><code class="ebnf">Program     = &#123; Item &#125; ;
Whitespace  = &#123; " " | "\t" | "\n" | "\r" | Comment &#125; ;
Comment     = "//" &#123; ? any character except newline ? &#125; "\n" | "/*" &#123; ? any character ? | Comment &#125; "*/" ;
Item        = [ "pub" ] ( Import | ExternBlock | ImportC | Fn | Struct | Enum | Concept
                        | Impl | ConstDecl | TypeAlias | Test ) ;
Import      = "import" Path [ "as" Ident | "." ( "*" | "&#123;" ImportItem &#123; "," ImportItem &#125; [ "," ] "&#125;" ) ] ";" ;
ImportItem  = Ident [ "as" Ident ] ;
Path        = Ident &#123; "." Ident &#125; ;
ImportC     = "import_c" String ";" ;
ExternBlock = "extern" String "&#123;" &#123; FnSig ";" &#125; "&#125;" ;
TypeAlias   = "type" Ident [ Generics ] "=" Type ";" ;
ConstDecl   = "const" Ident [ ":" Type ] ":=" Expr ";" ;
Test        = "test" String Block ;
Generics    = "&lt;" GParam &#123; "," GParam &#125; "&gt;" ;
GParam      = Ident [ ":" Bounds ] | "const" Ident ":" Type ;
Bounds      = BoundRef &#123; "+" BoundRef &#125; ;
BoundRef    = Path [ GenericArgs ]
            | ( "Fn" | "FnMut" ) "(" [ Type &#123; "," Type &#125; ] ")" [ "-&gt;" Type ] ;
TypeArg     = [ Ident "=" ] ( Type | Expr ) ;
GenericArgs = "&lt;" TypeArg &#123; "," TypeArg &#125; "&gt;" ;
Where       = "where" Constraint &#123; "," Constraint &#125; [ "," ] ;
Constraint  = Type ":" Bounds | Type "==" Type | "comptime" Expr ;
Fn          = [ "comptime" ] FnSig ( Block | "=&gt;" Expr ";" ) ;
FnSig       = "fn" Ident [ Generics ] "(" [ Params ] ")" [ "-&gt;" Type ] [ Where ] ;
Params      = Param &#123; "," Param &#125; [ "," ] ;
Param       = SelfParam | Ident ":" Type [ "=" Expr ] ;
SelfParam   = [ "mut" | "own" ] "self" ;
Struct      = [ "packed"  ] "struct" Ident [ Generics ] [ Where ] "&#123;" [ Fields ] "&#125;" ;
Fields      = Field &#123; "," Field &#125; [ "," ] ;
Field       = Ident ":" Type [ "=" Expr ] ;
Enum        = "enum" Ident [ Generics ] "&#123;" Variant &#123; "," Variant &#125; [ "," ] "&#125;" ;
Variant     = Ident [ "&#123;" [ Fields ] "&#125;" | "(" Type &#123; "," Type &#125; [ "," ] ")" ] ;
Concept     = "concept" Ident [ Generics ] [ ":" Bounds ] "&#123;" &#123; ConceptItem &#125; "&#125;" ;
ConceptItem = "type" Ident [ ":" Bounds ] ";" | FnSig ( ";" | Block | "=&gt;" Expr ";" ) ;
Impl        = "impl" [ Generics ] Type [ ":" Bounds ] [ Where ] "&#123;" &#123; ImplItem &#125; "&#125;" ;
ImplItem    = [ "pub" ] Fn | "type" Ident "=" Type ";" ;
Type        = [ "mut" ] TypeAtom &#123; TypeSuffix &#125; ;
TypeAtom    = "!" Type
            | "*" [ "mut" ] ( TypeAtom | "(" Bounds ")" )
            | "(" [ Type &#123; "," Type &#125; [ "," ] ] ")"
            | "fn" "(" [ Type &#123; "," Type &#125; ] ")" [ "-&gt;" Type ]
            | ( "Fn" | "FnMut" ) "(" [ Type &#123; "," Type &#125; ] ")" [ "-&gt;" Type ]
            | "DynFn" "&lt;" "(" [ Type &#123; "," Type &#125; [ "," ] ] ")" [ "-&gt;" Type ] "&gt;"
            | Path [ GenericArgs ] ;
TypeSuffix  = "[" ( ".." | Expr | "" ) "]" | "?" | "!" Type ;
Block       = "&#123;" &#123; Stmt &#125; [ Expr ] "&#125;" ;
Stmt        = Let | ConstDecl | AssignStmt | ExprStmt | Defer | Return | Break | Continue | Assert | Unsafe | ";" ;
AssignStmt  = PlaceExpr AssignOp Expr ";" ;
PlaceExpr   = Ident | "*" Unary | PlaceExpr "." ( Ident | Int ) [ GenericArgs ]
            | PlaceExpr "[" Expr "]" | PlaceExpr ".[" Expr "]" ;
Let         = [ "mut" ] ( Ident ( ":=" | ":" Type "=" ) | Pattern ":=" ) Expr ";" ;
ExprStmt    = Expr ";" | BlockLike | "comptime" BlockLike ;
Defer       = ( "defer" | "errdefer" ) ( Expr ";" | Block ) ;
Return      = "return" [ Expr ] ";" ;
Break       = "break" [ Label ] [ Expr ] ";" ;      Continue = "continue" [ Label ] ";" ;
Label       = "@" Ident ;      LabelDef = Label ":" ;
Assert      = "assert" Expr ";" ;
Unsafe      = "unsafe" Block ;
Expr        = Range ;
AssignOp    = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&amp;=" | "|=" | "^=" | "&lt;&lt;=" | "&gt;&gt;=" ;
Range       = Coalesce [ ".." [ Coalesce ] | "..=" Coalesce ] | ".." [ Coalesce ] ;
Coalesce    = Or &#123; "??" Or &#125; ;
Or          = And &#123; "||" And &#125; ;
And         = Cmp &#123; "&amp;&amp;" Cmp &#125; ;
Cmp         = BitOr &#123; ( "==" | "!=" | "&lt;" | "&gt;" | "&lt;=" | "&gt;=" ) BitOr &#125; ;
BitOr       = BitXor &#123; "|" BitXor &#125; ;
BitXor      = BitAnd &#123; "^" BitAnd &#125; ;
BitAnd      = Shift &#123; "&amp;" Shift &#125; ;
Shift       = Add &#123; ( "&lt;&lt;" | "&gt;&gt;" ) Add &#125; ;
Add         = Mul &#123; ( "+" | "-" ) Mul &#125; ;
Mul         = Cast &#123; ( "*" | "/" | "%" ) Cast &#125; ;
Cast        = Unary &#123; "as" Type &#125; ;
Unary       = ( "-" | "!" | "&amp;" [ "mut" ] | "*" | "comptime" ) Unary | Postfix ;
Postfix     = Primary &#123; "(" [ Args ] ")" | "[" Expr "]" | "." ( Ident | Int ) [ GenericArgs ] | ".[" Expr "]"
                      | "?" | "!" &#125; ;
Args        = Arg &#123; "," Arg &#125; [ "," ] ;
Arg         = [ Ident ":" ] Expr ;
Primary     = Literal | CString | Ident [ GenericArgs ] | StructLit | "(" [ Expr &#123; "," Expr &#125; [ "," ] ] ")"
            | "[" [ Expr &#123; "," Expr &#125; ] "]" | Lambda | BlockLike | Block ;
StructLit   = Path [ GenericArgs ] "&#123;" [ Ident ":" Expr &#123; "," Ident ":" Expr &#125; [ "," ] ] "&#125;" ;
Lambda      = [ "[" CaptureList "]" ] ( "|" [ LParam &#123; "," LParam &#125; ] "|" | "||" ) Expr ;
CaptureList = CaptureItem &#123; "," CaptureItem &#125; ;
CaptureItem = Ident "=" Expr | Ident ;
LParam      = Ident [ ":" Type ] ;
BlockLike   = If | While | For | Loop | Match | "spawn"  "(" Expr ")" ;
If          = "if" Cond Block [ "else" ( If | Block ) ] ;
Cond        = Expr | Pattern ":=" Expr ;
While       = [ LabelDef ] "while" Cond Block ;
For         = [ LabelDef ] "for" Pattern "in" Expr Block ;
Loop        = [ LabelDef ] "loop" Block ;
Match       = "match" Expr "&#123;" Arm &#123; "," Arm &#125; [ "," ] "&#125;" ;
Arm         = Pattern &#123; "|" Pattern &#125; [ "if" Expr ] "=&gt;" Expr ;
Pattern     = "_" | Literal | Path | Path "&#123;" [ FieldPat &#123; "," FieldPat &#125; ] [ ".." ] "&#125;"
            | Path "(" [ Pattern &#123; "," Pattern &#125; [ "," ] ] ")"
            | "(" [ Pattern &#123; "," Pattern &#125; [ "," ] ] ")" ;
FieldPat    = Ident [ ":" Pattern ] ;
Literal     = Int | Float | String | Rune | Byte | "true" | "false" | "null" ;
CString     = 'c"' &#123; AnyChar | Escape &#125; '"' ;
Rune        = "'" ( AnyChar | Escape ) "'" ;      Byte = "b'" ( Ascii | Escape ) "'" ;
Ident       = ( Letter | "_" ) &#123; Letter | Digit | "_" &#125; ;
Int         = ( "0x" HexDigit &#123; HexDigit | "_" &#125; | "0b" BinDigit &#123; BinDigit | "_" &#125;
            | "0o" OctDigit &#123; OctDigit | "_" &#125; | Digit &#123; Digit | "_" &#125; ) [ Ident ] ;
Float       = Digit &#123; Digit | "_" &#125; "." Digit &#123; Digit | "_" &#125; [ Exponent ] [ Ident ] ;
Exponent    = ( "e" | "E" ) [ "+" | "-" ] Digit &#123; Digit | "_" &#125; ;
String      = '"' &#123; AnyChar | Escape &#125; '"' ;
Escape      = "\\" ( "n" | "t" | "r" | "0" | "\\" | '"' | "'" | "u&#123;" HexDigit &#123; HexDigit &#125; "&#125;" ) ;
Letter      = "A" .. "Z" | "a" .. "z" ;
Digit       = "0" .. "9" ;
BinDigit    = "0" | "1" ;
OctDigit    = "0" .. "7" ;
HexDigit    = Digit | "A" .. "F" | "a" .. "f" ;
AnyChar     = ? any Unicode scalar except quote and backslash ? ;
Ascii       = ? any 7-bit character except quote and backslash ? ;</code></pre><p>Reserved words: <code>as assert break comptime concept const continue defer else enum errdefer extern false fn for if impl import import_c in loop match mut null packed pub return self Self spawn struct test true type unsafe where while</code>. <code>own</code> is contextual. <code>ok</code>, <code>err</code> and <code>std</code> are ordinary names.</p>
