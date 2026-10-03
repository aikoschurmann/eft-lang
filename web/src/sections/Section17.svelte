<h3 id="generics"><span class="num">17</span>Generics: one definition, many types </h3>
<p>A name in angle brackets stands for a type chosen at the use site.</p>
<pre><code class="eft">struct Pair&lt;T&gt; &#123; a: T, b: T &#125;

fn swap&lt;T&gt;(a: *mut T, b: *mut T) &#123;
    tmp := *a;
    *a = *b;
    *b = tmp;
&#125;

p := Pair&lt;i32&gt; &#123; a: 1, b: 2 &#125;;         // explicit type arguments
mut x := 1; mut y := 2;
swap(&amp;mut x, &amp;mut y);                  // T = i32, inferred from the arguments
n := parse&lt;i32&gt;("42");                 // spell it out when nothing can infer it
type IntPair = Pair&lt;i32&gt;;</code></pre>
<ul>
<li><b>Monomorphized.</b> Each distinct set of type arguments gets its own compiled copy. There is no boxing and no runtime type information. The price is code size, the same as C++ templates or Rust generics.</li>
<li><b>Bounds on structs.</b> A struct's bounds are not inherited. An <code>impl</code> for a bounded struct, and any function that names it, must restate them in its <code>where</code> clause: <code>impl&lt;K, V&gt; HashMap&lt;K, V&gt; where K: Hash + Eq</code>. Omitting them is an error reported at the impl header or the function signature. Note that <code>Vec&lt;T&gt;</code> has no struct bounds, which is why its impls (like <code>impl&lt;T&gt; Vec&lt;T&gt; : Deinit</code>) do not restate any.</li>

<li><b>An unbounded <code>T</code> is opaque.</b> You can move it, copy it, store it, and take its address, but you cannot compare, add or print it. To do more, state what you need as a bound such as <code>T: Ord</code>. That is the subject of the concepts sections.</li>
<li><b>Const parameters</b> take values instead of types: <code>struct Array&lt;T, const N: usize&gt;</code> (see comptime).</li>
<li>In an expression, <code>&lt;</code> after a name is read as type arguments only if the matching <code>&gt;</code> is followed by <code>(</code>, <code>.</code> or <code>&#123;</code>. Otherwise it is a comparison, so <code>a &lt; b</code> works as you expect.</li>
</ul>

