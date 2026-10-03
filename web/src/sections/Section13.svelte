<h3 id="recv"><span class="num">13</span>Receivers: read-only vs mutating </h3>
<p>Mutation and ownership are explicitly stated in the method signature. A method receiver dictates how it accesses the value, and the compiler automatically references or copies the caller's value to match.</p>
<ul>
    <li><code>self</code> is a read-only reference (sugar for <code>self: *Self</code>).</li>
    <li><code>mut self</code> is a mutable reference (sugar for <code>self: *mut Self</code>).</li>
    <li><code>own self</code> takes the value by value (a copy) (sugar for <code>self: Self</code>).</li>
</ul>
<pre><code class="eft">impl&lt;T&gt; Vec&lt;T&gt; &#123;
    fn len(self) -&gt; usize =&gt; self.count;            // receives *Vec&lt;T&gt;
    fn push(mut self, x: T) -&gt; !AllocError &#123; /* ... */ &#125;  // receives *mut Vec&lt;T&gt;
    fn new(a: *mut Allocator) -&gt; Vec&lt;T&gt; &#123; /* ... */ &#125;   // no self: a static function
&#125;

mut v := Vec&lt;i32&gt;.new(&amp;mut arena);   // static functions are called through the type
v.push(1)?;                           // ok: v is mut

w := Vec&lt;i32&gt;.new(&amp;mut arena);
w.push(2)?;                           // error: push needs a mutable receiver, w is not mut</code></pre>


<h4>Method calls, auto-ref and auto-deref</h4>
<p>For a call <code>e.m(args)</code> where <code>e</code> has type <code>E</code>:</p>
<ol>
<li><b>Find the method.</b> Let <code>T</code> be <code>E</code> with all outer <code>*</code> and <code>*mut</code> removed. Look up <code>m</code> in the inherent impls of <code>T</code>, then in the concept impls of <code>T</code> (or, for a type parameter, in its bounds). Repeated auto-deref is the only implicit conversion at the receiver.</li>
<li><b>Adapt <code>e</code> to the receiver:</b>
<div class="wrap"><table><tbody>
<tr><th>Receiver</th><th><code>E</code> is <code>T</code></th><th><code>E</code> is <code>*mut T</code></th><th><code>E</code> is <code>*T</code></th></tr>
<tr><td><code>self</code> (<code>*T</code>)</td><td>pass <code>&amp;e</code></td><td>pass <code>e</code> (coerces to <code>*T</code>)</td><td>pass <code>e</code></td></tr>
<tr><td><code>mut self</code> (<code>*mut T</code>)</td><td><code>e</code> must be a <b>mutable place</b>; pass <code>&amp;mut e</code></td><td>pass <code>e</code></td><td><b>error</b>: read-only pointer</td></tr>
<tr><td><code>own self</code> (<code>T</code>)</td><td>copy <code>e</code></td><td>copy <code>*e</code></td><td>copy <code>*e</code></td></tr>
</tbody></table></div>
</li>
<li><b>Mutable place.</b> This means one of:
<ul>
<li>a <code>mut</code> binding;</li>
<li>a field of a mutable place;</li>
<li><code>*p</code> where <code>p: *mut T</code>;</li>
<li>an element of a <code>mut T[..]</code> binding or parameter, an element of a <code>mut T[..]</code> field reached through a mutable place, or an element of a mutable array.</li>
</ul>
An immutable binding holding a <code>*mut T</code> is fine, because <code>p.push(1)</code> passes <code>p</code> itself and does not need <code>p</code> to be mutable.</li>
<li><b>Temporaries.</b> If <code>e</code> is an rvalue and the receiver is <code>self</code>, the compiler materializes a temporary that lives until the end of the enclosing full expression, and passes its address. A temporary cannot be named, so the local escape rule never fires on it. An rvalue with a <code>mut self</code> receiver is an error.</li>
<li><b>Fat pointers.</b> For <code>p: *Concept</code>, <code>p.m()</code> passes <code>p</code> itself and dispatches through the vtable. <code>mut self</code> methods require <code>*mut Concept</code>.</li>
<li><b>Inside a method.</b> <code>self</code> has type <code>*Self</code>, so <code>self.x</code> auto-derefs and <code>*self</code> is the value. In an <code>own self</code> method, <code>self</code> has type <code>Self</code> and is an immutable binding. Write <code>mut it := self;</code> to mutate it.</li>
<li><b>Fields through pointers.</b> Fields reached through <code>*T</code> are read-only, and fields reached through <code>*mut T</code> are writable.</li>
</ol>
