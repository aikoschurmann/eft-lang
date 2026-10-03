<h2 class="part" id="part2">Part II · Data</h2>
<h3 id="structs"><span class="num">7</span>Structs and enums </h3>
<p>A <code>struct</code> groups named fields. An <code>enum</code> is a value that is exactly one of several variants, each with its own data.</p>
<pre><code class="eft">struct Point &#123; x: i32, y: i32 &#125;
p := Point &#123; x: 1, y: 2 &#125;;
mut q := p;                  // assignment copies the bytes; p is unchanged
q.x = 10;

struct Window &#123; w: i32 = 640, h: i32 = 480, title: String &#125;
win := Window &#123; title: "eft" &#125;;      // omitted fields take their defaults

enum Direction &#123; North, South, East, West &#125;

enum Token &#123; Num(f64), Plus, Ident(String) &#125;          // tuple-style data
enum Kind  &#123; Circle &#123; r: f64 &#125;, Rect &#123; w: f64, h: f64 &#125;, Point &#125;   // named fields

k := Kind.Rect &#123; w: 2.0, h: 3.0 &#125;;
t := Token.Num(4.5);</code></pre>
<ul>
<li><b>Value semantics.</b> Assigning or passing a struct copies it. Sharing is always an explicit pointer.</li>
<li>Fields without a default must be given. There are no partially built values.</li>
<li>Methods are not declared inside structs or enums. They go in an <code>impl</code> (see Struct, impl and concept).</li>
</ul>

<h4>Layout and ABI</h4>
<p>Struct fields are laid out in memory exactly in the order they are declared, with padding added to satisfy alignment requirements. This makes C compatibility simple. To remove all padding (which requires unaligned access), use <code>packed struct</code>.</p>
<p>An enum is represented as a tag (an integer) followed by the data of the largest variant. However, Eft performs <b>niche packing</b>: if a variant contains a type that has an invalid bit pattern (a "niche"), the tag can sometimes be hidden inside that data. For example, <code>*T?</code> (an optional pointer) takes up the same space as <code>*T</code>, because the <code>null</code> variant is represented by the all-zero bit pattern, which is a niche in the pointer type. This specific pointer optimization is guaranteed by the ABI and is safe to use in C FFI (as a nullable C pointer).</p>
