<h3 id="iter"><span class="num">20</span>Iterators and ranges </h3>
<p><code>for</code> loops, ranges and chains like <code>.filter().map().sum()</code> are all built on one concept, <code>Iterator</code>. Nothing here allocates, and nothing is a special case in the compiler. Adapters are ordinary structs, monomorphized like any generic.</p>

<p><b>The Iterator concept</b></p>
<p>An iterator is anything that can produce its next item or say it is finished. "Finished" is <code>null</code>, so the return type is an optional.</p>
<pre><code class="eft">concept Iterator &#123;
    type Item;
    fn next(mut self) -> Self.Item?;
&#125;

struct Counter &#123; n: i32 &#125;
impl Counter : Iterator &#123;
    type Item = i32;
    fn next(mut self) -> i32? &#123;
        self.n += 1;
        if self.n &lt;= 3 &#123; self.n &#125; else &#123; null &#125;
    &#125;
&#125;

mut c := Counter &#123; n: 0 &#125;;
c.next();    // 1
c.next();    // 2
c.next();    // 3
c.next();    // null</code></pre>
<p><code>next</code> takes <code>mut self</code> because the iterator remembers where it is. Optionals are tagged values, so an iterator over <code>i32?</code> yields <code>i32??</code>, and an item that is itself <code>null</code> cannot be confused with the end.</p>

<p><b>How <code>for</code> works</b></p>
<p><code>for</code> calls <code>next()</code> until it returns <code>null</code>.</p>
<pre><code class="eft">for x in it &#123; body &#125;

// desugars to:
mut tmp := it;                   // if the type of it implements Iterator
mut tmp := it.into_iter();       // otherwise, it must implement IntoIterator
while x := tmp.next() &#123; body &#125;</code></pre>
<p>The compiler chooses by the static type of the iterable. If the type implements <code>Iterator</code>, it is used directly and <code>into_iter</code> is not called. Otherwise it must implement <code>IntoIterator</code>. If a type implements both, <code>Iterator</code> wins. There is no blanket impl, so coherence is not affected. <code>while x := …</code> is the optional-binding loop from Control flow, so <code>break</code> and <code>continue</code> behave as usual. <code>into_iter</code> comes from a second small concept:</p>
<pre><code class="eft">concept IntoIterator &#123;
    type Item;
    type Iter: Iterator&lt;Item = Self.Item&gt;;
    fn into_iter(own self) -&gt; Self.Iter;
&#125;</code></pre>
<p>Every <code>Iterator</code> can be used directly in <code>for</code>. Slices and arrays implement <code>IntoIterator</code> and return <code>xs.iter()</code>, so <code>for x in xs</code> and <code>for x in xs.iter()</code> are identical.</p>
<p>One limitation: a generic <code>fn f&lt;T: IntoIterator&gt;(x: T)</code> does not accept a plain iterator, because there is no blanket impl. The workaround is to bound on <code>I: Iterator</code> instead.</p>

<p><b>Ranges</b></p>
<p>A range is a plain struct that implements <code>Iterator</code>.</p>
<pre><code class="eft">for i in 0..n  &#123; &#125;     // Range&lt;usize&gt;:          0, 1, ..., n-1
for i in 0..=n &#123; &#125;     // RangeInclusive&lt;usize&gt;: 0, 1, ..., n

struct Range&lt;T&gt; &#123; cur: T, end: T &#125;
impl&lt;T&gt; Range&lt;T&gt; : Iterator where T: Step &#123;
    type Item = T;
    fn next(mut self) -> T? &#123;
        if self.cur >= self.end &#123; return null; &#125;
        v := self.cur;
        self.cur = v.succ();
        return v;
    &#125;
&#125;

struct RangeInclusive&lt;T&gt; &#123; cur: T, end: T, done: bool &#125;
impl&lt;T&gt; RangeInclusive&lt;T&gt; : Iterator where T: Step &#123;
    type Item = T;
    fn next(mut self) -> T? &#123;
        if self.done || self.cur > self.end &#123; return null; &#125;
        v := self.cur;
        if v == self.end &#123; self.done = true; &#125; else &#123; self.cur = v.succ(); &#125;
        return v;
    &#125;
&#125;</code></pre>
<p><code>Step</code> is a std concept for types that have a next value (<code>succ</code>) and are ordered; all integers and <code>Rune</code> implement it. The <code>done</code> flag in <code>RangeInclusive</code> is why <code>0..=255</code> on a <code>u8</code> terminates correctly: <code>succ()</code> of 255 is never called.</p>

<p><b>Adapters are lazy structs</b></p>
<p>An adapter wraps another iterator and changes what comes out. Each one is a struct holding the iterator it wraps, plus whatever closure it needs. Building one does no work.</p>
<pre><code class="eft">struct Filter&lt;I, P&gt; &#123; inner: I, pred: P &#125;
impl&lt;I, P&gt; Filter&lt;I, P&gt; : Iterator where I: Iterator, P: FnMut&lt;(I.Item,), Output = bool&gt; &#123;
    type Item = I.Item;
    fn next(mut self) -&gt; I.Item? &#123;
        while x := self.inner.next() &#123;
            if self.pred(x) &#123; return x; &#125;     // keep it
        &#125;                                     // otherwise pull the next one
        return null;
    &#125;
&#125;

struct Map&lt;I, F&gt; &#123; inner: I, f: F &#125;
impl&lt;I, F&gt; Map&lt;I, F&gt; : Iterator where I: Iterator, F: FnMut&lt;(I.Item,)&gt; &#123;
    type Item = F.Output;
    fn next(mut self) -&gt; F.Output? &#123;
        if x := self.inner.next() &#123; return self.f(x); &#125;
        return null;
    &#125;
&#125;

struct Enumerate&lt;I&gt; &#123; inner: I, i: usize &#125;
impl&lt;I&gt; Enumerate&lt;I&gt; : Iterator where I: Iterator &#123;
    type Item = (usize, I.Item);
    fn next(mut self) -> (usize, I.Item)? &#123;
        if x := self.inner.next() &#123;
            idx := self.i;
            self.i += 1;
            return (idx, x);
        &#125;
        return null;
    &#125;
&#125;</code></pre>
<p>The adapter methods are default methods on <code>Iterator</code>, so every iterator gets them. They take <code>own self</code>, so the iterator is moved by copy into the adapter.</p>
<pre><code class="eft">concept Iterator &#123;
    type Item;
    fn next(mut self) -> Self.Item?;

    fn filter&lt;P&gt;(own self, p: P) -&gt; Filter&lt;Self, P&gt;
    where P: FnMut&lt;(Self.Item,), Output = bool&gt;
        =&gt; Filter &#123; inner: self, pred: p &#125;;

    fn map&lt;F&gt;(own self, f: F) -&gt; Map&lt;Self, F&gt;
    where F: FnMut&lt;(Self.Item,)&gt;
        =&gt; Map &#123; inner: self, f: f &#125;;

    fn enumerate(own self) -> Enumerate&lt;Self&gt;
        => Enumerate &#123; inner: self, i: 0 &#125;;
&#125;</code></pre>

<p><b>Consumers do the work</b></p>
<p>A consumer calls <code>next()</code> in a loop and produces a final value. Until one runs, nothing happens:</p>
<pre><code class="eft">xs: i32[3] = [1, 2, 3];
it := xs.iter().map(|x| &#123; print("visit &#123;x&#125;\n"); x * 2 &#125;);
// nothing printed yet: 'it' is just a struct

total := it.sum();      // prints visit 1, visit 2, visit 3</code></pre>
<p>Consumers are default methods too. Some only exist when the item type allows them, using the same conditional-method rule as <code>Vec.sum</code>:</p>
<pre><code class="eft">fn sum(own self) -&gt; Self.Item
where Self.Item: Add&lt;Output = Self.Item&gt; + Default
&#123;
    mut it := self;
    mut total := Self.Item.default();
    while x := it.next() &#123; total = total + x; &#125;
    return total;
&#125;

fn count(own self) -&gt; usize &#123; /* ... */ &#125;
fn fold&lt;A, F: FnMut&lt;(A, Self.Item), Output = A&gt;&gt;(own self, init: A, f: F) -&gt; A &#123; /* ... */ &#125;

// Building a collection needs memory, so it takes an allocator:
fn collect(own self, a: *mut Allocator) -&gt; Vec&lt;Self.Item&gt;!AllocError &#123;
    mut it := self;
    mut v := Vec&lt;Self.Item&gt;.new(a);
    while x := it.next() &#123; v.push(x)?; &#125;
    return ok(v);
&#125;</code></pre>
<p><code>collect</code> is where the "no hidden allocations" rule shows up. A chain never allocates until you give it an allocator.</p>

<p><b>What the compiler does with a chain</b></p>
<pre><code class="eft">total := xs.iter().filter(|x| x % 2 == 0).map(|x| x * x).sum();</code></pre>
<p><b>1. The types nest, one struct per call.</b></p>
<pre><code class="eft">xs.iter()                    // SliceIter&lt;i32&gt;
  .filter(|x| x % 2 == 0)    // Filter&lt;SliceIter&lt;i32&gt;, __lambda_1&gt;
  .map(|x| x * x)            // Map&lt;Filter&lt;SliceIter&lt;i32&gt;, __lambda_1&gt;, __lambda_2&gt;
  .sum()</code></pre>
<p><b>2. The value is one struct on the stack.</b> <code>SliceIter</code> is a slice plus a position (24 bytes). Neither closure captures anything, so they add 0 bytes, and the whole chain is 24 bytes:</p>
<pre><code class="eft">Map &#123; inner: Filter &#123; inner: SliceIter &#123; data: xs[..], pos: 0 &#125;, pred: &#123;&#125; &#125;, f: &#123;&#125; &#125;</code></pre>
<p><b>3. <code>sum</code> drives it.</b> Each <code>Map.next()</code> calls <code>Filter.next()</code>, which calls <code>SliceIter.next()</code>.</p>
<p><b>4. After monomorphization every call is direct and can be inlined</b>, so the optimizer flattens the chain into the loop you would have written by hand:</p>
<pre><code class="eft">mut total := 0;
mut pos := 0;
while pos &lt; xs.len() &#123;
    x := xs[pos];
    pos += 1;
    if x % 2 == 0 &#123; total += x * x; &#125;
&#125;</code></pre>
<p>There are no function pointers, no allocation and no intermediate arrays.</p>

<p><b>Rules</b></p>
<ul>
<li><code>iter()</code> yields elements <b>by value</b>, so <code>|x| x % 2 == 0</code> works directly. For large element types, <code>iter_ptr()</code> yields <code>*T</code> and <code>iter_mut()</code> yields <code>*mut T</code>.</li>
<li>Closures passed to adapters follow the normal capture rules. Captured slices and pointers are not lifetime-checked.</li>
<li>A chain that is stored in a variable and advanced by hand must be declared <code>mut</code>. A chain used directly in <code>for</code> or a consumer is a temporary, and the compiler handles that.</li>
<li>Adapters that type-erase (a list of different iterators, say) are not in this section. Use the same tiers as closures: generic parameter first, then <code>*mut Iterator&lt;Item = T&gt;</code>, then an allocator-backed owner.</li>
</ul>
<div class="wrap"><table><tbody>
<tr><th>What you write</th><th>Type you get</th><th>Allocation</th></tr>
<tr><td><code>0..n</code></td><td><code>Range&lt;T&gt;</code></td><td>none</td></tr>
<tr><td><code>xs.iter().filter(f)</code></td><td><code>Filter&lt;SliceIter&lt;T&gt;, F&gt;</code></td><td>none</td></tr>
<tr><td><code>.sum()</code>, <code>.count()</code>, <code>.fold(..)</code></td><td>a value</td><td>none</td></tr>
<tr><td><code>.collect(&amp;mut arena)?</code></td><td><code>Vec&lt;T&gt;!AllocError</code></td><td>explicit (allocator)</td></tr>
</tbody></table></div>