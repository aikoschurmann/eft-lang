<h3 id="std"><span class="num">15</span>Std concepts and refinement </h3>

<h4>The Prelude</h4>
<p>Eft automatically imports a foundational set of concepts and types into every module. You do not need to <code>import</code> core concepts (<code>Eq</code>, <code>Ord</code>, <code>Hash</code>, <code>Clone</code>, <code>Display</code>, <code>Debug</code>, <code>From</code>, <code>Into</code>, <code>Deinit</code>) or common types (<code>String</code>, <code>Vec</code>, <code>Arena</code>). This keeps everyday code uncluttered.</p>
<p>Every operator and piece of syntax in Eft is backed by a concept in std. Built-in types implement them automatically; your types opt in explicitly with <code>impl Type : Concept</code>. This means operator overloading, formatters, and iterators are all <em>the same mechanism</em>—all overloadable syntax is concept-backed.</p>
<div class="wrap"><table><tbody>
<tr><th>Syntax</th><th>Concept</th><th>Required method</th></tr>
<tr><td><code>a == b</code>, <code>a != b</code></td><td><code>Eq</code></td><td><code>eq</code> (<code>ne</code> has a default)</td></tr>
<tr><td><code>a &lt; b</code>, <code>&gt;</code>, <code>&lt;=</code>, <code>&gt;=</code></td><td><code>Ord</code></td><td><code>cmp</code> (returns <code>Ordering</code>)</td></tr>
<tr><td><code>a + b</code>, <code>-</code>, <code>*</code>, <code>/</code>, <code>%</code></td><td><code>Add</code>, <code>Sub</code>, <code>Mul</code>, <code>Div</code>, <code>Rem</code></td><td><code>add(own self, o: Self)</code>, …</td></tr>
<tr><td><code>a += b</code>, <code>-=</code>, …</td><td><code>AddAssign</code>, <code>SubAssign</code>, …</td><td><code>add_assign(mut self, o: Self)</code>, …</td></tr>
<tr><td><code>a &amp; b</code>, <code>a | b</code>, <code>a ^ b</code></td><td><code>BitAnd</code>, <code>BitOr</code>, <code>BitXor</code></td><td><code>bitand(own self, o: Self)</code>, …</td></tr>
<tr><td><code>a &lt;&lt; n</code>, <code>a &gt;&gt; n</code></td><td><code>Shl</code>, <code>Shr</code></td><td><code>shl(own self, o: Self)</code>, …</td></tr>
<tr><td><code>-a</code>, <code>!a</code></td><td><code>Neg</code>, <code>Not</code></td><td><code>neg(own self)</code>, <code>not(own self)</code></td></tr>
<tr><td><code>a[i]</code>, <code>a[i] = v</code></td><td><code>Index</code>, <code>IndexMut</code></td><td><code>index(self, i)</code>, <code>index_mut(mut self, i)</code></td></tr>
<tr><td><code>for x in xs</code></td><td><code>IntoIterator</code></td><td><code>into_iter(own self)</code></td></tr>
<tr><td>looping over an iterator</td><td><code>Iterator</code></td><td><code>next(mut self)</code></td></tr>
<tr><td><code>"&#123;x&#125;"</code>, <code>"&#123;x:?&#125;"</code></td><td><code>Display</code>, <code>Debug</code></td><td><code>fmt</code> / <code>debug_fmt</code>, both <code>(self, w: *mut Writer, spec: fmt.Spec) -&gt; !fmt.Error</code></td></tr>
<tr><td><code>f(args)</code></td><td><code>Fn</code>, <code>FnMut</code></td><td><code>call(self, args)</code>, <code>call_mut(mut self, args)</code></td></tr>
<tr><td><code>defer x.deinit()</code></td><td><code>Deinit</code></td><td><code>deinit(mut self)</code></td></tr>
<tr><td>none (library use)</td><td><code>Default</code>, <code>Clone</code>, <code>Step</code></td><td><code>default</code>, <code>clone</code>, <code>succ(own self)</code></td></tr>
<tr><td>none (library use)</td><td><code>Hash</code>, <code>Hasher</code></td><td><code>hash(self, h: *mut Hasher)</code>, <code>write(mut self, bytes: u8[..])</code></td></tr>
<tr><td><code>T.parse(s)</code></td><td><code>Parse</code></td><td><code>parse(s: String) -&gt; Self!ParseError</code></td></tr>
</tbody></table></div>

<h4>Operator receivers</h4>
<p>Operators evaluate their operands to values and then call the concept method. The rules:</p>
<ul>
<li><b>Arithmetic and bitwise concepts take the left operand by value.</b> They use <code>fn add(own self, o: Self) -&gt; Self.Output</code>. This makes <code>(a + b) + c</code> work with no temporary rules, matches value semantics, and lets std implement <code>Add</code> for primitives without pointer plumbing.</li>
<li><b>Comparison and read-only concepts take <code>self</code> by reference.</b> That covers <code>Eq</code>, <code>Ord</code>, <code>Hash</code>, <code>Clone</code>, <code>Display</code>, <code>Debug</code>. The right operand stays by value, as in <code>fn eq(self, o: Self) -&gt; bool</code>.</li>
<li><b><code>a == b</code> desugars to <code>Eq.eq(a, b)</code></b> with temporaries materialized for <code>a</code> if it's an rvalue. <code>a &lt; b</code> and the other comparisons use the same path through <code>Ord</code>.</li>
<li><b><code>Step::succ</code>, <code>Default::default</code> and <code>From::from</code></b> are by value or static, so no pointer is involved.</li>
</ul>

<h4>Floats and Total Ordering</h4>
<p>In Eft, operators <code>==</code> and <code>&lt;</code> are backed directly by <code>Eq</code> and <code>Ord</code>. To avoid a split between <code>PartialEq</code> and <code>Eq</code>, floating-point types implement a total ordering (IEEE 754 <code>totalOrder</code>). This means <code>NaN == NaN</code> is true, and floats can be safely used as <code>HashMap</code> keys or sorted. If you need strict IEEE comparison semantics, use the <code>.is_nan()</code> method.</p>

<p><b>Refinement</b> is when one concept inherits the requirements of another using <code>:</code>. <code>Ord: Eq</code> means "to be orderable you must also be equatable." Any bound of <code>Ord</code> automatically implies <code>Eq</code>—you never have to write both.</p>
<p><b>Default methods</b> let a concept provide a fallback implementation. A type only needs to override it if the default is wrong. In the example below, <code>ne</code> is derived automatically from <code>eq</code>, and <code>lt</code>/<code>gt</code>/<code>le</code>/<code>ge</code> are all derived from <code>cmp</code>.</p>
<pre><code class="eft">concept Eq &#123;
    fn eq(self, o: Self) -> bool;
    fn ne(self, o: Self) -> bool => !self.eq(o);  // default method
&#125;

concept Ord: Eq &#123;                // refinement: Ord implies Eq
    fn cmp(self, o: Self) -> Ordering;
    fn lt(self, o: Self) -> bool => self.cmp(o) == Ordering.Less;   // default
    fn gt(self, o: Self) -> bool => self.cmp(o) == Ordering.Greater;
&#125;


// Implementing several concepts at once for Point
struct Point &#123; x: i32 = 0, y: i32 = 0 &#125;

impl Point : Eq + Ord + Default &#123;
    fn eq(self, o: Point)  => self.x == o.x && self.y == o.y;
    fn cmp(self, o: Point) -> Ordering &#123; /* compare x, then y */ &#125;
    fn default()           -> Self     => Point &#123; x: 0, y: 0 &#125;;
&#125;

// Now Point works with ==, &lt;, &gt;, &gt;=, &lt;=, and any generic bound on Ord.
fn clamp&lt;T: Ord&gt;(v: T, lo: T, hi: T) -> T &#123;
    if v.lt(lo) &#123; return lo; &#125;
    if v.gt(hi) &#123; return hi; &#125;
    return v;
&#125;</code></pre>

<h4>Hashing</h4>
<p>To support interchangeable hashing algorithms (like SipHash for security or FNV for speed), types that can be hashed implement <code>Hash</code>, which feeds bytes into a generic <code>Hasher</code>.</p>
<pre><code class="eft">concept Hasher &#123;
    fn write(mut self, bytes: u8[..]);
    fn finish(self) -&gt; u64;
&#125;

concept Hash &#123;
    fn hash(self, h: *mut Hasher);
&#125;</code></pre>
