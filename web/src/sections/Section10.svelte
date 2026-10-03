<h3 id="opt"><span class="num">10</span>Optionals and non-null pointers </h3>
<p><code>*T</code> is never null; <code>*T?</code> may be. <code>T?</code> is a plain tagged value (niche-packed for pointers).</p>
<pre><code class="eft">fn find(xs: i32[..], target: i32) -&gt; usize? &#123; /* ... */ &#125;

if i := find(xs, 7) &#123; print("at &#123;i&#125;\n"); &#125;
n := find(xs, 7) ?? 0;
mut p: *Node? = null;
while node := p &#123; visit(node); p = node.next; &#125;
v := find(xs, 7)!;        // assert present (panics if null)
x := find(xs, 7)?;        // unwrap or return null to caller</code></pre>
<h4>Intrinsic methods</h4>
<p>The <code>T?</code> type provides intrinsic methods: <code>opt.is_present()</code>, <code>opt.is_null()</code>, and <code>opt.map(|x| ...)</code>.</p>

