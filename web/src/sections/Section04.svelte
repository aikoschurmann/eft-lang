<h3 id="ops"><span class="num">4</span>Operators and expressions </h3>
<p>Almost everything is an expression: <code>if</code>, <code>match</code>, blocks and calls all produce values. From loosest to tightest binding:</p>
<div class="wrap"><table><tbody>
<tr><th>Level</th><th>Operators</th><th>Notes</th></tr>
<tr><td>1</td><td><code>=  +=  -=  *=  /=  %=  &amp;=  |=  ^=  &lt;&lt;=  &gt;&gt;=</code></td><td>assignment, right associative, a statement rather than a value</td></tr>
<tr><td>2</td><td><code>..  ..=</code></td><td>exclusive and inclusive ranges</td></tr>
<tr><td>3</td><td><code>??</code></td><td>default for an optional: <code>find(xs, 7) ?? 0</code></td></tr>
<tr><td>4</td><td><code>||</code></td><td>short-circuit or</td></tr>
<tr><td>5</td><td><code>&amp;&amp;</code></td><td>short-circuit and</td></tr>
<tr><td>6</td><td><code>==  !=  &lt;  &gt;  &lt;=  &gt;=</code></td><td>comparison, backed by <code>Eq</code> and <code>Ord</code></td></tr>
<tr><td>7</td><td><code>|</code></td><td>bitwise or, backed by <code>BitOr</code></td></tr>
<tr><td>8</td><td><code>^</code></td><td>bitwise xor, backed by <code>BitXor</code></td></tr>
<tr><td>9</td><td><code>&amp;</code></td><td>bitwise and, backed by <code>BitAnd</code></td></tr>
<tr><td>10</td><td><code>&lt;&lt;  &gt;&gt;</code></td><td>shifts, backed by <code>Shl</code>, <code>Shr</code></td></tr>
<tr><td>11</td><td><code>+  -</code></td><td>backed by <code>Add</code>, <code>Sub</code></td></tr>
<tr><td>12</td><td><code>*  /  %</code></td><td>backed by <code>Mul</code>, <code>Div</code>, <code>Rem</code></td></tr>
<tr><td>13</td><td><code>as</code></td><td>explicit conversion</td></tr>
<tr><td>14</td><td><code>-  !  &amp;  &amp;mut  *  comptime</code> (prefix)</td><td>negate, not (bitwise complement on integers), take address, dereference</td></tr>
<tr><td>15</td><td><code>f(x)  a[i]  a.b  a.[k]  e?  e!</code> (postfix)</td><td>call, index, field or method, comptime field, propagate, assert present</td></tr>
</tbody></table></div>
<p>Assignment is a statement and cannot appear inside an expression.</p>
<p><code>??</code> binds looser than comparison and arithmetic. <code>x ?? 0 == 5</code> parses as <code>x ?? (0 == 5)</code> and is a type error. Write <code>(x ?? 0) == 5</code>.</p>
<p>Operators on your own types are ordinary concept methods: <code>a + b</code> means <code>a.add(b)</code> once <code>impl T : Add</code> exists. The table in Std concepts lists which concept backs which syntax.</p>
<h4>Bitwise operators</h4>
<p>Bitwise operators bind tighter than comparison, so <code>flags &amp; MASK == 0</code> means <code>(flags &amp; MASK) == 0</code>, unlike C. There is no <code>~</code>: <code>!x</code> is the complement of an integer and the logical not of a <code>bool</code>. The parser tells the overloaded symbols apart by position, so no new keywords or context flags are needed:</p>
<ul>
<li><code>&amp;</code> is address-of only in prefix position (where an operand is expected) and bitwise and only after a complete operand. <code>a &amp; &amp;b</code> is legal.</li>
<li><code>|</code> starts a lambda only where an operand is expected, and is bitwise or after one. <code>||</code> likewise: an empty lambda at the start of an operand, logical or after one. In a <code>match</code> arm, <code>|</code> between patterns is a separator, because patterns are not expressions.</li>
<li><code>&gt;&gt;</code> closing nested generics (<code>Vec&lt;Vec&lt;i32&gt;&gt;</code>) is split by the same rule that already governs <code>GenericArgs</code>.</li>
</ul>
<h4>Integer overflow</h4>
<p><code>+ - *</code> and unary <code>-</code> panic on overflow in debug builds and wrap (two's complement) in release builds. It is never undefined behavior. Division or remainder by zero, and <code>MIN / -1</code>, always panic. A shift by the type's bit width or more panics in debug and masks the count in release. When you mean a specific behavior, say so with a method:</p>
<pre><code class="eft">a.wrapping_add(b)     // always wraps
a.checked_add(b)      // T?, null on overflow
a.saturating_add(b)   // clamps to MIN or MAX
// same for sub, mul, neg, shl, shr</code></pre>

