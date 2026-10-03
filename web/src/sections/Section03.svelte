<h3 id="types"><span class="num">3</span>Types: numbers, text, tuples, arrays and slices </h3>
<div class="wrap"><table><tbody>
<tr><th style="white-space: nowrap;">Type signature</th><th>What it is</th></tr>
<tr><td><code>i8 i16 i32 i64 isize</code></td><td>signed integers; <code>isize</code> is pointer-sized</td></tr>
<tr><td><code>u8 u16 u32 u64 usize</code></td><td>unsigned integers; <code>usize</code> is used for lengths and indices</td></tr>
<tr><td><code>f32 f64</code></td><td>IEEE floats</td></tr>
<tr><td><code>bool</code></td><td><code>true</code> or <code>false</code>; conditions must be <code>bool</code>, never an integer or pointer</td></tr>
<tr><td><code>Rune</code></td><td>a Unicode scalar value (32 bits, never a surrogate). Literal <code>'a'</code> or <code>'\u&#123;1F600&#125;'</code>. A byte literal <code>b'a'</code> is a <code>u8</code>.</td></tr>
<tr><td><code>String</code></td><td>a non-owning slice type structurally identical to a read-only <code>u8[..]</code>, but guaranteed to contain valid UTF-8. You cannot pass a <code>String</code> where a <code>u8[..]</code> is expected without calling <code>.as_bytes()</code>. To convert bytes back, use <code>std.str.from_utf8(bytes)</code>.</td></tr>

<tr><td><code>(A, B)</code></td><td>tuple, stored inline</td></tr>
<tr><td><code>T[N]</code></td><td>fixed-size array, stored inline</td></tr>
<tr><td><code>T[..]</code></td><td>slice: a pointer plus a length, no ownership</td></tr>
<tr><td><code>mut T[..]</code></td><td>mutable slice: allows modifying elements. The elements can be written to if accessed through a mutable reference (like a <code>*mut</code> receiver).</td></tr>
<tr><td><code>void</code></td><td>no value; the default return type</td></tr>
</tbody></table></div>
<p><code>(A, B)</code> is a tuple. <code>(A,)</code> is a 1-tuple, and the trailing comma is required to tell it from the parenthesized <code>(A)</code>, which is just <code>A</code>. <code>()</code> is the empty tuple and is the same type as <code>void</code>. The same rule applies to tuple expressions and patterns.</p>
<pre><code class="eft">t := (1, "one");              // (i32, String)
(n, label) := t;              // destructure, or read with t.0 and t.1

xs: i32[4] = [1, 2, 3, 4];    // array: the length is part of the type
n := xs.len();                // 4
first := xs[0];
mid := xs[1..3];              // slice of elements 1 and 2, no copy

fn sum(v: i32[..]) -&gt; i32 &#123; /* ... */ &#125;
sum(xs);                      // an array converts to a slice implicitly
sum(mid);</code></pre>
<p>Slices are read-only by default (<code>T[..]</code>). A <code>mut T[..]</code> allows writing its elements. A write <code>s[i] = v</code> is legal only if the type of <code>s</code> is <code>mut T[..]</code> and <code>s</code> is reached through a mutable path. A local binding or parameter of type <code>mut T[..]</code> is itself such a path, because its immutability only prevents reassigning the slice. A field of type <code>mut T[..]</code> is writable only when the struct is reached through a mutable place (a <code>mut</code> binding or a <code>*mut</code> receiver). Through a read-only <code>self</code>, <code>self.vals[i] = x</code> is an error, and <code>&amp;self.vals[i]</code> has type <code>*V</code>, not <code>*mut V</code>.</p>
<p><b>Conversions are explicit.</b> There are no implicit numeric conversions, so mixing <code>i32</code> and <code>f64</code> is an error until you write <code>as</code>.</p>
<pre><code class="eft">i: i32 = 7;
f := i as f64 * 0.5;          // as binds tighter than * and /
b := 300 as u8;               // narrowing truncates (checked: u8.try_from(300) gives u8?)
type Meters = f64;            // an alias, not a distinct type

c := 'é';                     // Rune
code := c as u32;             // Rune to integer is always fine
back := Rune.try_from(code);  // integer to Rune is checked: Rune?
for r in s.runes() &#123; &#125;        // decode UTF-8 into Runes
for b in s.bytes() &#123; &#125;        // raw u8 bytes</code></pre>
<p>Indexing is bounds-checked in debug builds. <code>xs.unchecked[i]</code> skips the check on purpose (see Later).</p>

