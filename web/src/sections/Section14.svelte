<h3 id="concepts"><span class="num">14</span>Concepts: one abstraction, two dispatch modes </h3>
<p>A <code>concept</code> is a named set of method signatures. A type satisfies it by declaring <code>impl Type : Concept</code>, and the compiler verifies the methods. <code>S: Shape</code> is static (monomorphized). <code>*Shape</code> is an explicit fat pointer (data + vtable) for dynamic dispatch; you make the pointer with <code>&amp;</code>, so nothing allocates behind your back. All bounds live in one <code>where</code> clause (inline <code>T: A + B</code> is sugar for it). Concepts may have associated types and refine others.</p>
<pre><code class="eft">concept Shape &#123; fn area(self) -&gt; f64; &#125;

fn total&lt;S: Shape&gt;(xs: S[..]) -&gt; f64 &#123;
    mut sum := 0.0;
    for s in xs &#123; sum += s.area(); &#125;
    sum
&#125;
fn total_dyn(xs: (*Shape)[..]) -&gt; f64 =&gt; xs.iter().map(|s| s.area()).sum();

concept Add &#123; type Output; fn add(own self, o: Self) -&gt; Self.Output; &#125;   // a + b  ==  a.add(b)

fn sum_all&lt;T&gt;(xs: T[..]) -&gt; T
where T: Add&lt;Output = T&gt; + Default
&#123;
    mut acc := T.default();
    for x in xs &#123; acc = acc + x; &#125;
    acc
&#125;</code></pre>
<p>A bare <code>Shape</code> value is unsized and rejected. A concept can be used behind a pointer (<code>*C</code> or <code>*mut C</code>) if all its associated types are named in the pointer type, as in <code>*mut Iterator&lt;Item = i32&gt;</code>. Methods that cannot go in a vtable are excluded rather than rejected. See Dynamic dispatch and object safety.</p>

