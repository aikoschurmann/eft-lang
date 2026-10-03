# Eft TODO (final)

All decisions are settled and folded in. Spec references are to the current spec's section numbers.

## 0. Settled decisions (reference)

| Topic | Decision |
|---|---|
| `t.0.1` | After a `.`, the lexer reads a maximal run of digits as `INT` and never a float or suffix. `t.0.1` lexes as `t . 0 . 1`. |
| Number suffixes | `10u8` and `1.5f32` are single tokens that carry suffix text. Validity of the suffix is checked in semantic analysis ("unknown literal suffix `foo`"). |
| `1e5` | Allowed. `Float` gains an exponent form without a dot. `0x1e5` stays hex. |
| `_` | Ordinary `IDENT`. The parser treats text `_` as the wildcard in patterns, and as an error in expressions ("`_` is only valid in patterns"). |
| Struct literal vs block | No bare struct literals in the heads of `if`, `while`, `for … in` and `match` unless parenthesized. |
| `pub` | Allowed on `import`, `fn`, `struct`, `enum`, `concept`, `const`, `type`. Not allowed on `impl`, `test`, `extern`, `import_c`. |
| Floats | Operators on floats are IEEE. Concepts split into `PartialEq`, `PartialOrd`, `Eq`, `Ord`. |
| Misc | `Self` and `self` are valid in paths and expressions. Builtin type names are ordinary identifiers. `own` and `sealed` are contextual. `Parse` uses `fmt.Error`. `.[` is one token. Block comments nest. Lexer errors are real errors. |

## 1. Spec edits

### 1.1 Type syntax (prefix modifiers, bracket-wrapped elements)

- [ ] §3: add the reading rule. Types read left to right, outermost first. `T!E` is always last and wraps everything to its left.
- [ ] Rewrite every occurrence:

| Old | New |
|---|---|
| `T[N]` | `[T; N]` |
| `T[..]` | `[T]` |
| `mut T[..]` | `[mut T]` |
| `T?`, `*T?` | `?T`, `?*T` |
| `(*Shape)[..]` | `[*Shape]` |

- [ ] Sections to sweep:
  - §3: type table, slice-mutability paragraph, `String` description (read-only `[u8]`), `try_from` results (`?u8`, `?Rune`)
  - §4: `checked_add -> ?T`
  - §5: `first_even`
  - §7: ABI paragraph
  - §9: `next: ?*Node`, prose
  - §10: heading prose, `find`, `mut p: ?*Node`, "`*T` is never null; `?*T` may be", "the `?T` type provides…"
  - §11: `write_file`
  - §13: "Mutable place" bullets
  - §14, §16, §18: `[S]`, `[T]`, `[*Shape]`, `[*(Shape + Display)]`
  - §15: `Hasher.write` and its table row
  - §19: `[i32; 4]`, `[String; 3]`, `[Button; 2]` (two places)
  - §20: every `X?` return becomes `?X`, the prose becomes "`?i32` yields `??i32`", and `xs: [i32; 3]`
  - §21: `buf: [mut u8]`, `data: [u8]`, `precision: ?usize`
  - §22: `-> [mut T]!AllocError`, `slice: [mut T]`
  - §24: `data: [T; N]`
  - §25: `malloc -> ?*mut u8`
  - §27: `keys`, `vals`, `occupied`, `get -> ?*V`, and the comment
- [ ] Result types are unchanged: `Config!IoError`, `[mut T]!AllocError`, `-> !fmt.Error`.
- [ ] Expressions don't change: `xs[1..3]`, `nums[..]`, `e?`, `e!`.
- [ ] §4: next to the `>>` split note, add that `??` is split into two `?` in type position (for `??i32`).
- [ ] §17: "A generic argument is parsed as a `Type` first. An array literal is never a valid argument, so `Vec<[i32]>` is unambiguous."
- [ ] §29 EBNF: delete `TypeAtom` and `TypeSuffix`, and replace `Type` with:

```
Type     = TypeCore [ "!" TypeCore ] | "!" TypeCore ;
TypeCore = "?" TypeCore
         | "*" [ "mut" ] ( TypeCore | "(" Bounds ")" )
         | "[" [ "mut" ] Type "]"
         | "[" Type ";" Expr "]"
         | "(" [ Type { "," Type } [ "," ] ] ")"
         | "fn" "(" [ Type { "," Type } ] ")" [ "->" Type ]
         | ( "Fn" | "FnMut" ) "(" [ Type { "," Type } ] ")" [ "->" Type ]
         | "DynFn" "<" "(" [ Type { "," Type } [ "," ] ] ")" [ "->" Type ] ">"
         | Path [ GenericArgs ] ;
Cast     = Unary { "as" TypeCore } ;
```

- [ ] `as` takes `TypeCore`, so `x as T!E` isn't ambiguous with postfix `e!`. Say so in §4.
- [ ] Parse error for a stray `mut`: "`mut` applies to `*` and `[ ]`".

### 1.2 Captures

- [ ] §19: replace "capture by value" with the capture-mode table:

| List | Meaning | Implements | Escape-checked |
|---|---|---|---|
| implicit or `[x]` | read-only copy | `Fn` | n/a |
| `[mut x]`, `[mut n = 0]` | private mutable copy | `FnMut` | n/a |
| `[&x]` | read-only reference | `Fn` | yes |
| `[&mut x]` | mutable reference (`x` must be `mut`) | `Fn` | yes |
| `[p = &mut x]` | raw pointer, unchecked | `Fn` | no |

- [ ] §19: `&` and `&mut` captures auto-deref in the body (`total += x`, not `*p += x`).
- [ ] §19 Rule 1: mutating a by-copy capture without `[mut x]` is an error. "Mutating" means assigning, calling a `mut self` method, or taking `&mut`. Diagnostic:

```
error: `total` is captured by copy, so this assignment would change only the closure's private copy
help: write `[mut total]` to keep a private copy, or `[&mut total]` to mutate the original
```

- [ ] §19 "Mutable captures": rewrite the example with `[mut count]` and show `[&mut count]` for shared state.
- [ ] §19 Compiler Selection: `FnMut` only when a `[mut x]` capture is mutated. `&mut` captures stay `Fn`, because writing through a `*mut` field is legal under a read-only `self`.
- [ ] §19 bullets: replace "capture a raw pointer" with `[&mut x]`, and keep `[p = &mut x]` as the unchecked hatch.
- [ ] §19, optional: `[&big]` replaces the "capture a slice" workaround and the 256-byte warning.
- [ ] §20: add `for_each` as a default consumer.
- [ ] §23: add a Checked row for closures with reference captures of locals, and extend the local escape rule:
  - **Local-bound closure:** it has a `&` or `&mut` capture rooted in a local's own storage. A place reached through a pointer or slice doesn't count.
  - **Local-bound type:** it is a local-bound closure or contains one as a field or type argument (`Map<I, λ>`).
  - **Treatment:** it can't be returned (including via an inferred `=>` return type), can't be stored in an outer scope, and can't be passed to `DynFn.new`.
  - **Not-checked column:** slices, `String` views and raw pointers in by-value captures stay there. Aliasing stays unchecked.
- [ ] §29: `CaptureItem = "mut" Ident [ "=" Expr ] | "&" [ "mut" ] Ident | Ident [ "=" Expr ]`.
- [ ] §4: parsing rule. At an operand-start `[`, try a capture list followed by `|` or `||` and commit to a lambda if it matches. This ambiguity with array literals exists today.
- [ ] Doc note: there is no `impl Fn` return type, so closure return-escape is reachable only through inferred `=>` returns, `DynFn.new` and outer-scope stores.

### 1.3 Vtables, `DynEq` and `*Eq`

- [ ] **New vtable section** (placed after §14, linked from §14 and §18). Cover:
  - what a vtable is
  - fat-pointer layout (data pointer plus vtable pointer)
  - how a call dispatches
  - why a method that takes or returns `Self` (other than as the receiver) can't be in it: the receiver is erased, so a second `Self` has unknown type
  - the exclusion rules, moved here from §18
- [ ] §14: supertrait upcast. `*C` coerces to `*S` for a supertrait `S`, and `*(A + B)` coerces to `*A` and `*B`. The vtable stores one pointer per supertrait vtable. `DynKey` needs this.
- [ ] §15: add `DynEq: Eq`, `DynOrd: DynEq + Ord` (`dyn_cmp -> ?Ordering`), `DynKey: DynEq + Hash`, and `TypeId` (opaque; `Eq`, `Hash`, `Debug`). Definitions:

```
concept DynEq: Eq {
    sealed fn dyn_type(self) -> TypeId => type_id<Self>();
    sealed fn dyn_eq(self, o: *DynEq) -> bool {
        if o.dyn_type() != type_id<Self>() { return false; }
        unsafe { return self.eq(*(raw_data(o) as *Self)); }
    }
}
```

- [ ] §15: `*DynKey` glue (`impl *DynKey : Eq` via `dyn_eq`, `impl *DynKey : Hash` forwarding). Note that `*Hash` already works, so there is no `DynHash`.
- [ ] §18: replace "rendering `*Eq` unusable" with a pointer to `DynEq`/`DynOrd`. Add the empty-vtable rule: forming `*C` where every method is excluded is an error at that point, listing each excluded method and its reason (for example "`eq` is excluded because it takes `Self` as an argument. Did you mean `DynEq`?").
- [ ] §12 or §18: document the enum alternative (`enum` plus `match` for closed sets).
- [ ] §23 `unsafe` list: add `raw_data`.
- [ ] §24: add a "Compiler intrinsics" note. `type_id<T>()` lowers to the address of a 1-byte per-type static, and works without comptime. `raw_data` is unsafe. These are ordinary names, not keywords.
- [ ] §28: `TypeId` is unique per linked program only. Don't print or persist it, and don't pass `*DynEq` across dynamic-library boundaries.
- [ ] §29: `ConceptItem = "type" Ident [ ":" Bounds ] ";" | [ "sealed" ] FnSig ( ";" | Block | "=>" Expr ";" )`. Sealed methods can't be overridden in an impl. Add `sealed` to the reserved-words note as contextual, like `own`.

### 1.4 Floats and comparison concepts

- [ ] §15: replace the concepts and delete the "Floats and Total Ordering" subsection:

```
concept PartialEq { fn eq(self, o: Self) -> bool; fn ne(self, o: Self) -> bool => !self.eq(o); }
concept PartialOrd: PartialEq { fn partial_cmp(self, o: Self) -> ?Ordering; /* lt, gt, le, ge defaults */ }
concept Eq: PartialEq { }                       // marker: reflexive
concept Ord: Eq + PartialOrd { fn cmp(self, o: Self) -> Ordering; /* partial_cmp default => cmp */ }
```

- [ ] §15 syntax table: `==`, `!=` are backed by `PartialEq`, and `<`, `>`, `<=`, `>=` by `PartialOrd`. Integers and other types implement all four. Floats implement only the partial ones.
- [ ] §15: replace the deleted subsection with two sentences: float operators follow IEEE 754, and `f64.total_cmp(o)` gives a total order. Add an `OrdF64` wrapper (Eq, Ord, Hash through total order) to the minimal std. `HashMap<f64, V>` is a compile error.
- [ ] §15: `clamp<T: PartialOrd>` in the example.
- [ ] §4: level-6 note "backed by `PartialEq` and `PartialOrd`".
- [ ] §16: `Number` becomes `PartialOrd + Add<Output = Self> + Mul<Output = Self> + Default`. Update the summary row if it mentions `Eq`.
- [ ] §18: examples that say "no `Eq` bound" use `PartialEq` for `==`.
- [ ] `Hash` still pairs with `Eq` in `HashMap` bounds (§27 stays `K: Hash + Eq`).
- [ ] `DynEq` stays on `Eq`, and `DynOrd` follows `Ord`.

### 1.5 Grammar and lexical edits

- [ ] `Item` (§29):

```
Item = [ "pub" ] ( Import | Fn | Struct | Enum | Concept | ConstDecl | TypeAlias )
     | Impl | Test | ExternBlock | ImportC | "comptime" Block ;
```

This fixes the `pub impl`/`pub test` mismatch and the top-level `comptime { … }` in §24. Mark individual declarations in an `extern` block instead of the block.

- [ ] `ExprStmt`: allow `"comptime" Block` as well as `"comptime" BlockLike`, so `comptime { … }` works inside functions.
- [ ] `Float` (§29): `Digit { Digit | "_" } ( "." Digit { Digit | "_" } [ Exponent ] | Exponent ) [ Ident ]`.
- [ ] §29 / §1: after a `.` in postfix position, an `Int` is a plain digit run (no suffix, no `_`, no float).
- [ ] §29 / §2: state that `Int`/`Float` suffixes are lexed as part of the literal, and that `_` is an `Ident` that is a wildcard only in patterns.
- [ ] §5 and EBNF note: no bare `StructLit` in `Cond`, `For` iterator expressions, or `Match` scrutinee unless parenthesized. The restriction is lifted inside parentheses, brackets, and call arguments.
- [ ] `Primary`/`Path`: allow `Self` as a `Path` head (`Self.Item`, `Self.Output`, `Self.Item.default()`), and `self` and `Self` as primaries.
- [ ] State that builtin type names (`i32`, `u8`, `void`, …) are ordinary identifiers. Verify the reserved-words count when you edit it. The spec's list has 37 words by my count, and the parser review said 38, so one of them is off.
- [ ] `PlaceExpr` is left-recursive despite the "no left recursion" title. Adopt `lang.bnf`'s iterative form.
- [ ] Tighten `FieldPat` to match `lang.bnf` (`[COMMA DOT_DOT] | DOT_DOT`).
- [ ] Make `.[` a single token in the grammar notes.
- [ ] Update `lang.bnf`: remove UNDERSCORE, and add the new `Item`, `Float`, `Type`/`TypeCore`, `CaptureItem` and `sealed` rules so it matches the spec.

### 1.6 Spec bugs

- [ ] §27: `key.hash() as usize` is wrong, because `hash` takes a hasher and returns nothing. Add a helper and use it in `put` and `get`:

```
fn slot(key: K, cap: usize) -> usize {
    mut h := Fnv.new();
    key.hash(&mut h);
    return h.finish() as usize % cap;
}
```

- [ ] `Parse`: change the §15 table row to `parse(s: String) -> Self!fmt.Error`, to match §21.
- [ ] §1: add the sentence "block comments nest, and an unterminated block comment is an error".

## 2. Lexer

- [ ] Nested block comments. Error on unterminated.
- [ ] UTF-8 rune literals (`'é'`), and decode `\u{…}` in both runes and strings.
- [ ] Closed escape set: `n t r 0 \\ " ' u{…}`. Reject others.
- [ ] `0o17` octal.
- [ ] Join integer and float suffixes into one token with suffix text (`10u8`, `1.5f32`).
- [ ] Exponent floats without a dot (`1e5`). Hex is lexed first, so `0x1e5` stays an int.
- [ ] After a `.` token, lex a digit run as `INT` only (`t.0.1` → `t . 0 . 1`).
- [ ] Lex `.[` as one token.
- [ ] Remove `~` (`TK_TILDE`) and `b"…"` (`BYTE_STR_LIT`). The spec has `b'…'` and `c"…"` only.
- [ ] Use the result of `lexer_lex_char`, so `b'ab'`, `b'é'`, and unterminated `b'`, `b"` and `c"` are errors.
- [ ] Builtin type names lex as plain `IDENT` so `i32.parse(s)` and `Rune.try_from(c)` work.
- [ ] `_` lexes as `IDENT`.
- [ ] Don't make `own` or `sealed` keywords. Keep exactly the spec's reserved words.
- [ ] Report `TK_UNKNOWN` as a lexer error. Remove or use `TK_ERROR` and `TK_COMMENT`.
- [ ] Keep `??`, `>>` and `>>=` as single tokens. The parser splits them in type position.
- [ ] Regression tests in `test/test_lexer.c` for every item above.

## 3. Parser

**Bugs in existing expression parsing**
- [ ] `t.0`: `parse_member` must accept `INT` after `.`, including chains like `t.0.1`.
- [ ] `can_start_expr` is missing `self`, `Self`, `[`, `{`, `|`, `||`, `..` and the keyword-led expressions. This is what breaks `0..self.len()`.
- [ ] Prefix `..` is only legal at the Range level, so reject `1 + ..5`.
- [ ] Decode literal values (ints with suffix, floats, strings, runes, bytes) instead of printing `0` and `""`.
- [ ] Give `a.[k]` its own node (for example `AST_COMPTIME_FIELD`) instead of reusing `AST_INDEX_EXPR`.
- [ ] Imports: reject keywords consistently in paths and in `{…}` lists, and strip the quotes from `import_c`.
- [ ] Validate assignment targets against `PlaceExpr`.
- [ ] Wire up `parse_assign_or_expr`.
- [ ] `_` in an expression is an error, and in a pattern it is the wildcard.

**Missing pieces (build in this order)**
1. [ ] `parse_type` for the new `Type`/`TypeCore`: `??` splitting, `T!E` last, `Fn`/`FnMut`/`DynFn`, and the stray-`mut` error. This unblocks `as`.
2. [ ] Expressions: `as` (takes `TypeCore`), array literals, lambdas with capture lists (lookahead per §1.2), struct literals (with the `no_struct_lit` flag), generic arguments, `self`, `Self`, `Path` expressions.
3. [ ] Generic-argument lookahead: `<` after a name starts type arguments only if the matching `>` is followed by `(`, `.` or `{`. Arguments try `Type` first.
4. [ ] `parse_pattern`: `_`, literals, paths, struct, tuple-variant and tuple patterns, `..`, and `|` between patterns in arms.
5. [ ] `parse_block` and `parse_stmt`: let (`:=`, `: T =`, destructuring), const, defer/errdefer, return, break/continue with labels, assert, unsafe.
6. [ ] `if`, `if x := …`, `while`, `for`, `loop`, labels (`@name:`), `match`, `spawn`. Set `no_struct_lit` while parsing their heads.
7. [ ] Items: fn (with `=>` bodies, generics, where, defaults, `self`/`mut self`/`own self`), struct (including `packed`), enum, concept (associated types, default methods, `sealed`), impl, const, type alias, test, extern, top-level `comptime` block. Reject `pub` on `impl`, `test`, `extern` and `import_c`.

## 4. AST and printer

- [ ] AST nodes: patterns, generic params and bounds, enum variants, self-param kind, concept items (with `sealed` flag), extern ABI string, array-length expressions in types, `Fn`/`FnMut`/`DynFn` types, capture items (copy, `mut`, `&`, `&mut`, aliased), comptime field access.
- [ ] `AstTypeKind` needs: optional, pointer (with mut), slice (with mut), array, tuple, result, function, Fn/FnMut/DynFn, path.
- [ ] Printer: add `OP_REF_MUT` (`&mut y` prints `?`), fix deref (`*p` prints `.*`), handle `AST_ARG`, and print `OP_AND` and `OP_OR` as `&&` and `||`.

## 5. Order of work

1. Spec edits §1.5 and §1.6, then §1.1. The lexer and parser depend on them.
2. Lexer (§2).
3. Existing parser bugs, then `parse_type`, expressions, patterns, statements, items (§3).
4. AST and printer fixes alongside the parser (§4).
5. Spec edits §1.2, §1.3 and §1.4 any time. They don't block parsing, except capture lists and `sealed`, which land with lambdas and concepts.