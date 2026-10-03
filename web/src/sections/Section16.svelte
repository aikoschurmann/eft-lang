<h3 id="constraints"><span class="num">16</span>Type constraints with concepts </h3>
<p>A bound like <code>T: Display</code> is the simplest constraint. Concepts can express much more. Every form below uses the same two tools: <code>:</code> in the generic list, and a <code>where</code> clause for anything longer. The next section, <a href="#checked">How constraints are checked</a>, explains what the compiler does with them.</p>
<h4>Simple and combined bounds</h4>
<pre><code class="eft">fn show&lt;T: Display&gt;(x: T) &#123; print("&#123;x&#125;\n"); &#125;
fn largest&lt;T: Ord + Display&gt;(xs: T[..]) -&gt; T &#123; /* ... */ &#125;</code></pre>
<h4>Where clauses for readability</h4>
<pre><code class="eft">fn merge&lt;K, V&gt;(a: HashMap&lt;K, V&gt;, b: HashMap&lt;K, V&gt;) -&gt; HashMap&lt;K, V&gt;
where K: Hash + Eq,
      V: Clone + Default
&#123; /* ... */ &#125;</code></pre>
<h4>Constraints on associated types</h4>
<p>Reach through a type variable to constrain what it contains. A concept can also bound its own associated type (<code>type Key: Eq;</code>).</p>
<pre><code class="eft">fn print_all&lt;I: Iterator&gt;(it: I)
where I.Item: Display
&#123;
    for x in it &#123; print("&#123;x&#125;\n"); &#125;
&#125;</code></pre>
<h4>Equality constraints</h4>
<p>Force two types to be the same, either in a bound or in a <code>where</code> clause.</p>
<pre><code class="eft">fn sum_all&lt;T: Add&lt;Output = T&gt; + Default&gt;(xs: T[..]) -&gt; T &#123; /* ... */ &#125;

fn zip_eq&lt;A: Iterator, B: Iterator&gt;(a: A, b: B)
where A.Item == B.Item &#123; /* ... */ &#125;</code></pre>
<h4>Conditional impls</h4>
<p>A generic type gets a concept only when its parameters earn it. This avoids writing a separate impl for every concrete type.</p>
<pre><code class="eft">impl&lt;T&gt; Vec&lt;T&gt; : Display where T: Display &#123;       // Vec&lt;T&gt; is Display only if T is
    fn fmt(self, w: *mut Writer, spec: fmt.Spec) -&gt; !fmt.Error &#123;
        for x in self.iter() &#123; x.fmt(w, spec)?; &#125;
        return ok();
    &#125;
&#125;
impl&lt;A, B&gt; (A, B) : Eq where A: Eq, B: Eq &#123; /* tuples are Eq if both parts are */ &#125;</code></pre>
<h4>Conditional methods</h4>
<p>One method can require more than its type does, without splitting the impl.</p>
<pre><code class="eft">impl&lt;T&gt; Vec&lt;T&gt; &#123;
    fn len(self) -&gt; usize =&gt; self.count;
    fn sum(self) -&gt; T where T: Add&lt;Output = T&gt; + Default &#123; /* only callable when T can add */ &#125;
    fn sorted(self) -&gt; Vec&lt;T&gt; where T: Ord &#123; /* ... */ &#125;
&#125;</code></pre>
<h4>Refinement: bounds that imply bounds</h4>
<p>Std concepts such as <code>Eq</code> and <code>Ord</code> are listed in the previous section; refinement lets you bundle them into one name.</p>
<pre><code class="eft">concept Number: Ord + Add&lt;Output = Self&gt; + Mul&lt;Output = Self&gt; + Default &#123; &#125;     // a bundle with no methods of its own

fn clamp&lt;T: Number&gt;(x: T, lo: T, hi: T) -&gt; T &#123; /* may call .eq, .cmp, + and * */ &#125;</code></pre>
<h4>Concepts with parameters</h4>
<pre><code class="eft">concept Convert&lt;To&gt; &#123; fn convert(own self) -&gt; To; &#125;
fn to_all&lt;T: Convert&lt;f64&gt;&gt;(xs: T[..]) -&gt; f64 &#123; /* sum of x.convert() */ &#125;
impl i32 : Convert&lt;f64&gt; &#123; fn convert(own self) -&gt; f64 =&gt; self as f64; &#125;</code></pre>
<h4>Function-shaped and compile-time constraints</h4>
<pre><code class="eft">fn retry&lt;F: Fn() -&gt; bool&gt;(f: F, n: u32) &#123; /* ... */ &#125;

struct Buf&lt;T, const N: usize&gt; where comptime N &gt; 0 &amp;&amp; size_of(T) * N &lt;= 4096 &#123; data: T[N] &#125;</code></pre>
<h4>Constraints on dynamic pointers</h4>
<p>A fat pointer can require several concepts at once. The vtable carries all of them.</p>
<pre><code class="eft">fn log_shapes(xs: (*(Shape + Display))[..]) &#123;
    for s in xs &#123; print("&#123;s&#125; has area &#123;s.area()&#125;\n"); &#125;
&#125;</code></pre>
<h4>Summary</h4>
<div class="wrap"><table><tbody>
<tr><th>Form</th><th>Syntax</th><th>Meaning</th></tr>
<tr><td>Single / combined</td><td><code>T: A + B</code></td><td>T must satisfy both</td></tr>
<tr><td>Where clause</td><td><code>where T: A, U: B</code></td><td>Same, easier to read when long</td></tr>
<tr><td>Associated type</td><td><code>where I.Item: Display</code></td><td>Constrain a type inside a concept</td></tr>
<tr><td>Equality</td><td><code>Add&lt;Output = T&gt;</code>, <code>A.Item == B.Item</code></td><td>Two types must be identical</td></tr>
<tr><td>Conditional impl</td><td><code>impl X : D where T: C</code></td><td>X&lt;T&gt; is D only when T is C</td></tr>
<tr><td>Conditional method</td><td><code>fn f(self) where T: C</code></td><td>Method exists only when T is C</td></tr>
<tr><td>Refinement</td><td><code>concept Ord: Eq</code></td><td>Ord implies Eq</td></tr>
<tr><td>Parameterized</td><td><code>Convert&lt;f64&gt;</code></td><td>Concept takes types</td></tr>
<tr><td>Compile-time</td><td><code>where comptime N &gt; 0</code></td><td>Any comptime bool predicate</td></tr>
<tr><td>Dynamic</td><td><code>*(A + B)</code></td><td>Fat pointer to several concepts</td></tr>
</tbody></table></div>

