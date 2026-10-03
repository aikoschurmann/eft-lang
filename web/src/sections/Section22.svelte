<h2 class="part" id="part5">Part V · Memory</h2>
<h3 id="alloc"><span class="num">22</span>Allocators and ownership </h3>
<p>Containers take an allocator explicitly (Zig style). There is no implicit global allocator and no hidden heap.</p>
<pre><code class="eft">concept Allocator &#123;
    // Core low-level methods
    fn alloc(mut self, size: usize, align: usize) -&gt; *mut u8!AllocError;
    fn free(mut self, p: *mut u8, size: usize);
&#125;

// Free functions provided by the stdlib for allocating typed slices:
fn alloc_slice&lt;T&gt;(a: *mut Allocator, count: usize) -&gt; mut T[..]!AllocError &#123; /* ... */ &#125;
fn alloc_zeroed_slice&lt;T&gt;(a: *mut Allocator, count: usize) -&gt; mut T[..]!AllocError &#123; /* ... */ &#125;
fn free_slice&lt;T&gt;(a: *mut Allocator, slice: mut T[..]) &#123; /* ... */ &#125;
</code></pre>
<p><code>alloc_slice</code> returns uninitialized memory, which is the one exception to "always initialized". Write every element before reading it, or use <code>alloc_zeroed_slice</code>.</p>

<h4>Ownership and explicit cleanup</h4>
<p>Memory and resources are not cleaned up automatically. There is no <code>Drop</code> trait and no destructor that runs at the end of a scope. If a type owns resources, it implements the <code>Deinit</code> concept and you must clean it up explicitly, usually with <code>defer</code>.</p>
<pre><code class="eft">concept Deinit &#123;
    fn deinit(mut self);
&#125;

mut list := Vec&lt;i32&gt;.new(alloc);
defer list.deinit();</code></pre>

<p>Copying an owning struct is just a shallow copy of its pointers. If you do this and call <code>deinit</code> on both, you will double free the buffer. The debug allocator will catch this.</p>

<p>Generic containers must always clean up their own allocations, but they clean up their elements conditionally. A <code>Vec&lt;T&gt;</code> implements <code>Deinit</code> unconditionally, and uses a <code>comptime if</code> to check if its elements need cleanup:</p>
<pre><code class="eft">impl&lt;T&gt; Vec&lt;T&gt; : Deinit &#123;
    fn deinit(mut self) &#123;
        comptime if implements&lt;T, Deinit&gt;() &#123;
            // Inside this branch, T: Deinit is a known fact, so this type-checks
            for item in self.iter_mut() &#123; (*item).deinit(); &#125;
        &#125;
        std.mem.free_slice(self.alloc, self.data);
    &#125;
&#125;</code></pre>
<p>When you write generic code that takes ownership of a value and needs to clean it up, you must require the bound <code>T: Deinit</code> explicitly.</p>

<h4>Arenas</h4>
<p>With an arena you can often skip element-wise cleanup. <code>Arena.deinit()</code> frees all the memory at once, so you do not need to call <code>deinit</code> on the individual lists or strings allocated inside it (unless they hold non-memory resources like file handles).</p>
<pre><code class="eft">mut arena := Arena.new(page_allocator);
defer arena.deinit();

mut list := Vec&lt;i32&gt;.new(&amp;mut arena);
list.push(1)?; // inside a function returning !AllocError
// no defer list.deinit() needed, the arena frees the memory</code></pre>
<p><code>std.mem.page_allocator</code> is a constant of type <code>*mut Allocator</code> that points at a static OS-backed allocator. <code>Arena.new(page_allocator)</code> therefore passes it directly, with no <code>&amp;mut</code>.</p>

<h4>A minimal std</h4>
<p>The standard library provides standard collections and utilities. Constructors are uniform: <code>Type.new(allocator)</code>.</p>
<div class="wrap"><table><tbody>
<tr><th>Type</th><th>Purpose</th><th>Cleanup (<code>Deinit</code>)</th></tr>
<tr><td><code>Vec&lt;T&gt;</code></td><td>Growable array</td><td>Frees buffer; deinits elements if <code>T: Deinit</code></td></tr>
<tr><td><code>HashMap&lt;K, V&gt;</code></td><td>Hash map</td><td>Frees buckets; deinits keys/values if applicable</td></tr>
<tr><td><code>StringBuf</code></td><td>Growable string</td><td>Frees buffer (unlike <code>String</code>, which is just a view)</td></tr>
<tr><td><code>Arena</code></td><td>Bump allocator</td><td>Frees all memory at once; does <b>not</b> run element deinits</td></tr>
</tbody></table></div>
