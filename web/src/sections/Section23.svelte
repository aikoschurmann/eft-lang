<h3 id="mem"><span class="num">23</span>Memory safety: what Eft checks and what it doesn't </h3>
<p>Eft does not have a borrow checker. To keep the language small and compile times fast, memory safety relies on simple strict rules and runtime debugging rather than complex static analysis.</p>

<div class="wrap"><table><tbody>
<tr><th>Checked</th><th>Not checked</th></tr>
<tr><td>Bounds (debug builds)</td><td>Use after free</td></tr>
<tr><td>Integer overflow (debug builds)</td><td>Dangling pointers into arenas</td></tr>
<tr><td>Optionals and non-null pointers</td><td>Aliasing</td></tr>
<tr><td>Local escape rule: no returning, or storing in outer scope, a pointer or slice to a local</td><td>Pointers and slices inside closure captures</td></tr>
<tr><td>Debug allocator (caught at runtime in debug builds only): use after free, double free, leaks</td><td>Anything inside <code>unsafe</code></td></tr>
</tbody></table></div>

<p>The <b>local escape rule</b> is a simple syntactic check. If a variable is declared in a function, you cannot return a pointer to it, nor can you assign a pointer to it into a variable declared outside its scope. This prevents the most common form of dangling pointers without requiring lifetime annotations.</p>
<pre><code class="eft">fn bad() -&gt; *i32 &#123;
    x := 5;
    return &amp;x; // error: pointer to local variable escapes
&#125;</code></pre>

<p>For everything else, safety comes from the <b>debug allocator</b>. When enabled, it replaces std.mem.page_allocator with one that tracks every allocation, fills freed memory with poison values, and aborts immediately on double frees or leaks. In production builds, this overhead is removed.</p>

<h4>Undefined behavior (UB)</h4>
<p>Eft tries to minimize UB, but does not eliminate it. Integer overflow is explicitly <b>not</b> UB (it wraps in release builds and panics in debug builds). However, the following operations cause undefined behavior:</p>
<ul>
<li>Dereferencing a dangling, uninitialized, or unaligned raw pointer</li>
<li>Using memory after it has been freed</li>
<li>Reading memory returned by <code>alloc_slice</code> before writing it</li>
<li>Data races (concurrent reads and writes without synchronization)</li>
<li>Violating the bounds of a slice (using <code>.unchecked[]</code>, or a normal <code>xs[i]</code> in release builds)</li>
</ul>

<h4>The <code>unsafe</code> block</h4>
<p>Operations that the compiler cannot verify or that easily cause undefined behavior must be wrapped in an <code>unsafe &#123; ... &#125;</code> block. This includes:</p>
<ul>
<li>Calling C functions (FFI)</li>
<li>Dereferencing a <code>raw_ptr</code> (an untyped pointer like <code>void*</code> in C)</li>
<li>Pointer arithmetic</li>
<li>Casting between pointers of different types</li>
<li>Using <code>.unchecked[]</code> to bypass slice bounds checks</li>
</ul>
<pre><code class="eft">unsafe &#123;
    p := raw_ptr(addr); // raw pointer arithmetic and casting
&#125;</code></pre>
