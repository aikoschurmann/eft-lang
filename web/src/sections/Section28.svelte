<h3 id="later"><span class="num">28</span>Later, decided early </h3>
<ul>
<li><b>Concurrency:</b> <code>spawn</code>, <code>Chan&lt;T&gt;</code>, atomics, and a marker concept <code>Send</code> for types safe to move across threads. Async deferred.</li>
<li><b>Systems extras:</b> <code>packed struct</code>, <code>align(16)</code>, bitfields, <code>volatile</code>, inline <code>asm</code>, <code>simd&lt;f32, 4&gt;</code>.</li>
<li><b>Slice safety:</b> bounds checks in debug, off in release; <code>xs.unchecked[i]</code> is the explicit escape.</li>
<li><b>Tooling:</b> formatter, LSP, tree-sitter grammar (derived from the EBNF below).</li>
<li><b>Shadowing:</b> allowed across blocks, an error within one block. Parameters live in the function body's scope, so a body declaration may not redeclare a parameter.</li>
</ul>
<pre><code class="eft">packed struct Hdr &#123; tag: u8, len: u32 &#125;
spawn(|| worker(ch.clone()));
fn share&lt;T: Send&gt;(x: T) &#123; &#125;
x := xs.unchecked[i];</code></pre>
