<h3 id="fn"><span class="num">6</span>Functions </h3>
<pre><code class="eft">fn add(a: i32, b: i32) -&gt; i32 &#123; return a + b; &#125;
fn add2(a: i32, b: i32) -&gt; i32 =&gt; a + b;            // expression body
fn hello() &#123; print("hi\n"); &#125;                       // no -&gt; means void

fn divmod(a: i32, b: i32) -&gt; (i32, i32) =&gt; (a / b, a % b);
(q, r) := divmod(17, 5);                            // return several values as a tuple

fn draw(w: Window, scale: f64 = 1.0, vsync: bool = false) &#123; /* ... */ &#125;
draw(win);
draw(win, scale: 2.0, vsync: true);                 // named arguments, any order after positional ones</code></pre>
<ul>
<li><b>Parameters are immutable copies</b>, except <code>self</code> and <code>mut self</code>, which are references (see Receivers). To change the caller's data, take a <code>*mut T</code> and pass <code>&amp;mut x</code> (see Pointers).</li>
<li>Parameter types are always written. Return types must be written (omitting means <code>void</code>), except on <code>=></code> bodies (inferred, unless recursive) and in concept impls (inherited from the concept).</li>
<li>No overloading and no variadics. Use default parameters and slices.</li>
<li>Default values are evaluated at each call, in the callee's scope.</li>
<li>A plain function has a type like <code>fn(i32) -&gt; i32</code>. It captures nothing, so it is just an address. Capturing values is what lambdas are for.</li>
<li><code>pub</code> exports a function from its module. <code>comptime fn</code> runs at compile time (see comptime).</li>
</ul>
<pre><code class="eft">fn twice(f: fn(i32) -&gt; i32, x: i32) -&gt; i32 =&gt; f(f(x));
twice(add_one, 3);          // pass a function by name</code></pre>

