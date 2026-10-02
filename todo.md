# Eft Compiler To-Do

## 1. Lexer Implementation
Implement the scanner and emit the following `TokenKind` list:

### Keywords & Types
- [ ] Declarations: `fn`, `struct`, `enum`, `impl`, `concept`, `type`
- [ ] Control Flow: `if`, `else`, `for`, `while`, `match`, `break`, `continue`, `return`, `defer`
- [ ] Modifiers: `comptime`, `mut`, `const`, `pub`
- [ ] Bindings: `as`, `in`
- [ ] Modules: `import`
- [ ] Literals: `true`, `false`, `null`
- [ ] Built-ins: `i8`, `i16`, `i32`, `i64`, `isize`, `u8`, `u16`, `u32`, `u64`, `usize`, `f32`, `f64`, `bool`, `void`

### Delimiters & Punctuation
- [ ] Brackets: `LParen` `()`, `LBrace` `{}`, `LBracket` `[]`
- [ ] Punctuation: `Comma` `,`, `Colon` `:`, `Semicolon` `;`, `Dot` `.`
- [ ] Arrows: `Arrow` `->`, `FatArrow` `=>`

### Operators
- [ ] Assignment: `Eq` `=`, `ColonEq` `:=`
- [ ] Arithmetic: `Plus` `+`, `Minus` `-`, `Star` `*`, `Slash` `/`, `Percent` `%`
- [ ] Compound: `PlusEq` `+=`, `MinusEq` `-=`, `StarEq` `*=`, `SlashEq` `/=`, `PercentEq` `%=`
- [ ] Comparison: `EqEq` `==`, `BangEq` `!=`, `Less` `<`, `LessEq` `<=`, `Greater` `>`, `GreaterEq` `>=`
- [ ] Logical: `AndAnd` `&&`, `OrOr` `||`
- [ ] Bitwise: `Ampersand` `&`, `Pipe` `|`, `Caret` `^`, `Tilde` `~`, `Shl` `<<`, `Shr` `>>`
- [ ] Eft-Specific: `Bang` `!` (error types / NOT), `Question` `?` (optionals / early return), `DotDot` `..` (ranges/slices)

### Literals & Identifiers
- [ ] `Ident` (identifiers)
- [ ] `IntLit` (integer literals, handle `_` separators)
- [ ] `FloatLit` (floating point literals)
- [ ] `StringLit` (UTF-8 string literals `""`)
- [ ] `ByteStringLit` (byte string literals `b""`)
- [ ] `RuneLit` (unicode scalar literals `''`)
- [ ] `ByteLit` (byte literals `b''`)

### System
- [ ] `Comment` (ignore or attach to AST)
- [ ] `Error` (invalid characters)
- [ ] `EOF` (end of file)
