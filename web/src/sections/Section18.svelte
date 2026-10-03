<h3 id="checked"><span class="num">18</span>How constraints are checked </h3>
<p>A bound is a <b>promise the caller makes and the function body relies on</b>. The compiler checks each side once, in this order. The running example is:</p>
<pre><code class="eft">fn sum_all&lt;T: Add&lt;Output = T&gt; + Default&gt;(xs: T[..]) -&gt; T &#123;
    mut acc := T.default();        // ok: Default is a bound
    for x in xs &#123; acc = acc + x; &#125; // ok: Add is a bound, and Output = T makes acc + x a T
    acc
&#125;</code></pre>
<h4>1. Collect the facts</h4>
<p>The bounds in the generic list and the <code>where</code> clause become a set of facts about <code>T</code>. Here: <code>T: Add</code>, <code>T.Output == T</code>, <code>T: Default</code>.</p>
<h4>2. Check the body once, using only those facts</h4>
<p>Inside the function <code>T</code> is opaque. An operation is legal only if some fact provides it. The body is checked <b>once</b>, not once per instantiation as C++ templates are, so errors point at the generic code itself.</p>
<pre><code class="eft">fn bad1&lt;T: Add&gt;(a: T, b: T) -&gt; T =&gt; a + b;
// error: a + b has type T.Output, but T was expected
// help: add `Output = T` to the Add bound

fn bad2&lt;T&gt;(a: T, b: T) -&gt; bool =&gt; a == b;
// error: T has no Eq bound, so == is not available</code></pre>
<h4>3. Check each call</h4>
<p>At <code>sum_all(xs)</code> the compiler infers <code>T</code>, then for every bound looks up the matching impl. A missing impl is reported <b>at the call site</b>. Associated types in the found impl must also match the bound's equality constraints.</p>
<pre><code class="eft">sum_all(points);
// error: Point does not implement Add
// note: required by the bound on `T` in `sum_all`</code></pre>
<h4>4. Monomorphize</h4>
<p>With every bound satisfied, the compiler emits a copy of the function with <code>T</code> replaced, and turns each <code>a + b</code> into a direct call to that type's <code>add</code>. No lookup or dispatch happens at runtime. Copies are deduplicated by their argument list.</p>
<h4>5. Evaluate comptime predicates</h4>
<p>A clause such as <code>where comptime N &gt; 0</code> is evaluated during instantiation, once <code>N</code> is known. If it is false, the instantiation is rejected with the failed expression in the message.</p>
<h4>5b. comptime-if facts</h4>
<p><code>implements&lt;T, C&gt;()</code> is a comptime predicate. In a <code>comptime if P &#123; A &#125; else &#123; B &#125;</code> inside a generic body, the compiler checks branch <code>A</code> once with <code>P</code> assumed true. When <code>P</code> is <code>implements&lt;T, C&gt;()</code>, that means with the fact <code>T: C</code> added. It checks branch <code>B</code> with <code>P</code> assumed false, which adds no facts. At instantiation only the taken branch is emitted, so the untaken branch is never monomorphized.</p>
<h4>Associated types and equality</h4>
<p><code>T.Output</code> is a projection, "the Output of T's Add impl". With <code>T: Add&lt;Output = T&gt;</code> the compiler rewrites <code>T.Output</code> to <code>T</code>. Without it the projection stays opaque and is equal only to itself, which is why <code>bad1</code> fails. A clause <code>where A.Item == B.Item</code> adds a rewrite rule in the same way. A <code>where I.Item: Display</code> clause is simply one more fact, attached to the projection.</p>
<h4>Impl lookup and coherence</h4>
<ul>
<li>There is at most one impl per <code>(Type, Concept&lt;Args&gt;)</code> pair. <code>Convert&lt;f64&gt;</code> and <code>Convert&lt;String&gt;</code> for <code>i32</code> are different pairs, so both may exist.</li>
<li>A conditional impl (<code>impl&lt;T&gt; Vec&lt;T&gt; : Display where T: Display</code>) is a candidate only when its <code>where</code> facts hold for the type being looked up.</li>
<li>Two impls whose headers could match the same type are rejected where they are declared, not where they are used.</li>
<li>If two bounds on one parameter provide a method with the same name, the call is ambiguous. Qualify it: <code>Shape.area(x)</code>.</li>
<li>The orphan rule: you can write <code>impl T : C</code> only if your package defined either <code>T</code> or <code>C</code>. You cannot implement a foreign concept for a foreign type.</li>
</ul>
<h4>Dynamic dispatch and object safety</h4>
<p>A pointer to a concept (<code>*C</code>, <code>*mut C</code>) is a fat pointer: a data pointer plus a vtable pointer. Every associated type must be named: <code>*mut Iterator&lt;Item = i32&gt;</code>.</p>
<p>A method is excluded from the vtable if it:</p>
<ul>
<li>is generic, or</li>
<li>takes <code>own self</code>, or</li>
<li>mentions <code>Self</code> anywhere except as the receiver (an argument or the return type). Note that this means <code>Eq.eq(self, o: Self)</code> is excluded from vtables, rendering <code>*Eq</code> unusable for equality checks.</li>
</ul>
<p>Excluded methods can only be called through static dispatch. Calling one on a fat pointer is an error that names the method and says why it was excluded.</p>
<p>A projection of <code>Self</code> whose associated type is named in the pointer type does not count as mentioning <code>Self</code>. In <code>*mut Iterator&lt;Item = i32&gt;</code>, the signature <code>next(mut self) -&gt; Self.Item?</code> is read as <code>-&gt; i32?</code> and stays in the vtable. Any other use of <code>Self</code> (an argument or return type of <code>Self</code> itself) excludes the method.</p>
<p><code>next(mut self)</code> stays in the vtable, so <code>*mut Iterator&lt;Item = T&gt;</code> can be advanced. The pointer's mutability must match the receiver: <code>mut self</code> methods need <code>*mut C</code>, so a read-only <code>*Iterator&lt;Item = T&gt;</code> cannot be advanced.</p>
<p>To use excluded methods such as <code>.map(...)</code> on a fat pointer, std provides:</p>
<pre><code class="eft">impl&lt;T&gt; *mut Iterator&lt;Item = T&gt; : Iterator &#123;
    type Item = T;
    fn next(mut self) -&gt; T? &#123; /* vtable call */ &#125;
&#125;</code></pre>
<p><code>.map(f)</code> takes <code>own self</code>, so the pointer itself (two words) is copied into the <code>Map</code> adapter. Only <code>next</code> goes through the vtable.</p>

<h4>Where each failure is reported</h4>
<div class="wrap"><table><tbody>
<tr><th>Failure</th><th>Reported at</th><th>Example</th></tr>
<tr><td>Operation not provided by any bound</td><td>the generic body</td><td><code>a == b</code> with no <code>Eq</code></td></tr>
<tr><td>Associated type mismatch inside the body</td><td>the generic body</td><td><code>T.Output</code> vs <code>T</code></td></tr>
<tr><td>Missing impl for a type argument</td><td>the call or use</td><td><code>sum_all</code> on a type with no <code>Add</code></td></tr>
<tr><td>Impl exists but its associated type differs</td><td>the call or use</td><td><code>Output</code> is not <code>T</code></td></tr>
<tr><td>Failed comptime predicate</td><td>the instantiation</td><td><code>Buf&lt;u8, 0&gt;</code></td></tr>
<tr><td>Overlapping impls</td><td>the impl declaration</td><td>two impls that can match one type</td></tr>
<tr><td>Missing method in <code>impl T : C</code></td><td>the impl header</td><td>forgot <code>area</code></td></tr>
</tbody></table></div>

