<h2 class="part" id="part1">Part I · The basics</h2>
<h3 id="hello"><span class="num">1</span>Hello, Eft </h3>
<p>An Eft program is a set of items (functions, structs, enums, concepts, impls) spread over files. Execution starts at <code>main</code>.</p>
<pre><code class="eft">// a line comment
fn main() -&gt; i32 &#123;
    print("hello, eft\n");
    return 0;
&#125;
</code></pre>
<ul>
<li>Line comments (<code>//</code>) and block comments (<code>/* ... */</code>, which can nest) exist. Statements end with <code>;</code>.</li>
<li>Blocks are expressions. The last expression, written <b>without</b> a semicolon, is the block's value.</li>
<li><code>main</code> returns an <code>i32</code> exit code, or nothing. It can also return a result (<code>!E</code>); an error (which must implement <code>Debug</code>; <code>impl E : Debug &#123;&#125;</code> gives the reflection-based default) prints to stderr and exits with code 1.</li>
<li><code>print</code> takes a format string. <code>&#123;expr&#125;</code> inserts a value (see String formatting).</li>
<li><code>eft run</code> builds and runs the package.</li>
</ul>
<pre><code class="eft">fn square(x: i32) -&gt; i32 &#123;
    x * x              // no semicolon: this is the return value
&#125;</code></pre>

