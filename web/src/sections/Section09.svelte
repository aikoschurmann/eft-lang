<h3 id="ptr"><span class="num">9</span>Pointers </h3>
<p>A pointer is an explicit address. <code>*T</code> reads, <code>*mut T</code> reads and writes. Taking an address is always visible: <code>&amp;</code> or <code>&amp;mut</code>.</p>
<pre><code class="eft">x := 10;
p := &amp;x;               // p: *i32, can only read
mut y := 20;
q := &amp;mut y;           // q: *mut i32; &amp;mut needs y to be declared mut
*q = 21;               // write through the pointer
v := *p;               // read through it

fn zero(p: *mut i32) &#123; *p = 0; &#125;
mut n := 5;
zero(&amp;mut n);          // n is now 0

struct Node &#123; v: i32, next: *Node? &#125;
n2 := Node &#123; v: 1, next: null &#125;;
print("&#123;n2.v&#125;\n");     // n2 is a plain value; through p := &amp;n2, p.v works too, without writing *</code></pre>
<ul>
<li><b>A <code>*T</code> is never null.</b> Absence is the separate type <code>*T?</code> (next section).</li>
<li><b>No pointer arithmetic.</b> To walk memory, use a slice <code>T[..]</code>, which carries its length. Raw arithmetic is only available inside <code>unsafe</code>.</li>
<li><b>No aliasing rules.</b> Several <code>*mut</code> pointers may refer to the same value. There is no borrow checker.</li>
<li><b>A pointer must not outlive its data.</b> The debug allocator and the local escape rule (see Memory safety) catch the common mistakes.</li>
</ul>

