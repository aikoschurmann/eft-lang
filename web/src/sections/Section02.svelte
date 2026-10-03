<h3 id="vars"><span class="num">2</span>Variables and constants </h3>
<p>Names are introduced with <code>:=</code>. They are immutable unless marked <code>mut</code>, so a reader can see what may change.</p>
<pre><code class="eft">x := 5;                  // immutable, type inferred (i32)
pi := 3.14159;           // inferred f64
mut count := 0;          // mutable
count += 1;              // assignment uses = (or += -= *= /= %=)

name: String = "eft";    // explicit type: name: Type = value
mut total: f64 = 0.0;

const MAX := 1024;       // compile-time constant, usable at file level
(q, r) := divmod(17, 5); // destructuring</code></pre>
<div class="wrap"><table><tbody>
<tr><th>Form</th><th>Meaning</th></tr>
<tr><td><code>x := v;</code></td><td>declare, immutable, type inferred</td></tr>
<tr><td><code>mut x := v;</code></td><td>declare, mutable</td></tr>
<tr><td><code>x: T = v;</code></td><td>declare with an explicit type</td></tr>
<tr><td><code>x = v;</code></td><td>assign to an existing <code>mut</code> variable</td></tr>
<tr><td><code>const X := v;</code></td><td>value computed at compile time, never changes</td></tr>
</tbody></table></div>
<ul>
<li><b>Always initialized.</b> There is no way to declare a variable without a value.</li>
<li><b>Scope is the enclosing block.</b> An inner block may shadow an outer name. Redeclaring a name in the <i>same</i> block is an error. Loop variables (<code>for x in xs</code>) and conditional bindings (<code>if x := x</code>) declare new names in the inner block, making it idiomatic to unwrap a value into a variable of the same name.</li>
<li><b>Immutable means the whole value.</b> If <code>p := Point &#123; x: 1, y: 2 &#125;</code> then <code>p.x = 5</code> is rejected.</li>
<li><b>Integer literals default to <code>i32</code>, float literals to <code>f64</code></b>, unless the context says otherwise.</li>
</ul>

