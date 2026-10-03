<h2 class="part" id="part4">Part IV · Higher-order features</h2>
<h3 id="lam"><span class="num">19</span>Lambdas and the Fn concept </h3>

<p>Lambdas (closures) in Eft look like <code>|args| body</code>. They capture variables from the surrounding scope <b>by value</b> and produce zero hidden allocations.</p>

<h4>How capturing works</h4>
<p>When a closure uses a variable from outside itself, the compiler copies that variable into a hidden struct it generates for you. The struct is as large as the sum of the captured values — nothing more. The body of the lambda becomes the body of a <code>call</code> method on that struct.</p>
<pre><code class="eft">base := 2.5;
add  := |x: f64| x + base;
//  compiler generates:
//    struct __add &#123; base: f64 &#125;       ← 8 bytes, lives on the stack
//    impl __add : Fn(f64) -> f64 &#123;
//        fn call(self, x: f64) -> f64 &#123; return x + self.base; &#125;
//    &#125;
//  'add' has type __add, not a pointer. Passing it copies 8 bytes.</code></pre>

<p>If you only need one value from a larger structure, extract it before the closure so the struct stays small:</p>
<pre><code class="eft">nums: i32[4] = [10, 20, 30, 40];

first := nums[0];
f := |x| x + first;           // captures 4 bytes (one i32)

// Need arbitrary index at call time? Capture a slice.
// A slice is pointer + length — 16 bytes regardless of array size.
view := nums[..];
g := |i: usize| view[i];      // captures 16 bytes (ptr + len)

// WARNING: if nums is a local variable and the closure outlives its scope,
// the slice's pointer is dangling. The compiler does not check this for you —
// slices and raw pointers inside captured structs are your responsibility.</code></pre>

<h4>Mutable captures</h4>
<p>When a closure mutates a captured variable, the compiler generates a struct with a <code>call_mut(mut self)</code> method, which implements the <code>FnMut</code> concept. Calling it mutates the closure's own private copy of the variable, maintaining state across calls. The original variable outside the closure does not change.</p>
<pre><code class="eft">mut count := 0;
mut inc := || &#123; count += 1; return count; &#125;;

inc();   // call_mut() mutates the closure's struct → returns 1
inc();   // mutates the struct again → returns 2
// The 'count' outside is still 0. The closure has its own copy.

print("&#123;count&#125;\n");   // prints 0</code></pre>
<p>To accumulate state across calls <i>and</i> observe it outside the closure, capture a raw pointer to a mutable variable, making the shared state explicit:</p>
<pre><code class="eft">mut count := 0;
inc := [p = &mut count] || &#123; *p += 1; return *p; &#125;;

inc();   // *p increments the real count → 1
inc();   // → 2
print("&#123;count&#125;\n");   // prints 2</code></pre>

<h4>Capture lists</h4>
<p>By default every referenced name is captured by value automatically. A <b>capture list</b> in square brackets before the parameters lets you spell it out, or alias a name to a different expression:</p>
<pre><code class="eft">base := 10;

// Implicit: compiler captures whatever the body uses
f := |x: f64| x + base as f64;

// Explicit: same thing, written out (silences large-capture warnings)
g := [base] |x: f64| x + base as f64;

// Aliased: capture the slice 'nums[..]' under the name 'view'
h := [view = nums[..]] |i: usize| view[i];

// The compiler warns (not errors) if an implicit capture exceeds 256 bytes.
// Writing the name in a capture list acknowledges you want the copy.</code></pre>

<h4>The Fn and FnMut concepts</h4>
<p><code>Fn</code> and <code>FnMut</code> are built-in concepts. Every callable implements one or both depending on what it captures and how it uses it. The compiler expands the sugar forms for you.</p>
<pre><code class="eft">// Fn and FnMut are defined in std roughly like this:
concept FnMut&lt;Args&gt; &#123;
    type Output;
    fn call_mut(mut self, args: Args) -&gt; Self.Output;
&#125;

concept Fn&lt;Args&gt;: FnMut&lt;Args&gt; &#123;              // Fn refines FnMut
    fn call(self, args: Args) -&gt; Self.Output;
    fn call_mut(mut self, args: Args) -&gt; Self.Output =&gt; self.call(args);   // default
&#125;

// Sugar (what you write):           Fn(A) -&gt; R
// Raw  (what the compiler sees):    Fn&lt;(A,), Output = R&gt;</code></pre>

<p><b>Compiler Selection.</b> The compiler automatically chooses which concepts the closure implements. If the body assigns to a captured variable, calls a <code>mut self</code> method on a capture, or takes <code>&mut</code> of one, the closure implements <code>FnMut</code> only. Otherwise it implements <code>Fn</code>, and therefore also <code>FnMut</code>.</p>
<p><b>Bound Choice.</b> Accept <code>FnMut</code> for the most general API, as standard iterator adapters do. Accept <code>Fn</code> only when you need to call through a shared, read-only reference.</p>
<p><b>Calling.</b> For <code>f: F</code> with <code>F: Fn</code>, <code>f(x)</code> means <code>f.call(x)</code>, so <code>f</code> may be an immutable binding. With <code>F: FnMut</code>, <code>f(x)</code> means <code>f.call_mut(x)</code>, so <code>f</code> must be a mutable place under the receiver rules in §13. That is why <code>mut inc := || &#123; count += 1; ... &#125;</code> needs <code>mut</code>. Calling through a pointer follows the same table: <code>p: *Fn(A)</code> calls <code>call</code>, while <code>p: *mut FnMut(A)</code> is required to call <code>call_mut</code>. A read-only <code>*FnMut</code> cannot be called.</p>

<h4>Passing closures around — a progressive example</h4>
<p>Start with the simplest approach. Move to the next one only when you hit the exact problem it solves.</p>

<p><b>Step 1 — Generic parameter. Always try this first.</b></p>
<p>Write a generic function with an <code>Fn</code> bound. The compiler stamps out a concrete version for each closure type. The call is a direct, inlined method — zero overhead.</p>
<pre><code class="eft">fn apply&lt;F: Fn(f64) -> f64&gt;(f: F, x: f64) -> f64 &#123;
    return f(x);
&#125;

apply(|x| x * 2.0, 5.0);   // works perfectly</code></pre>

<p><b>Step 2 — The problem: storing a closure in a struct.</b></p>
<p>Suppose you want a <code>Button</code> that holds its own click handler. You try making it generic. Here is what the compiler <em>actually generates</em> behind the scenes for two buttons, including where each call gets bound:</p>
<pre><code class="eft">// What you write:
struct Button&lt;F: Fn()&gt; &#123; label: String, on_click: F &#125;
ok     := Button &#123; label: "OK",     on_click: || print("ok\n")     &#125;;
cancel := Button &#123; label: "Cancel", on_click: || print("cancel\n") &#125;;

// What the compiler generates:

// 1. Each lambda becomes its own struct (nothing captured, so no fields)...
struct __lambda_ok     &#123; &#125;
struct __lambda_cancel &#123; &#125;

// 2. ...plus an impl of Fn. The lambda BODY becomes the body of call().
//    This is where print is bound: print("ok\n") is resolved by normal name
//    lookup (std.io.print) to a direct call, exactly as if you had written
//    this function by hand.
impl __lambda_ok : Fn() &#123;
    fn call(self) &#123; print("ok\n"); &#125;          // direct call to std.io.print
&#125;
impl __lambda_cancel : Fn() &#123;
    fn call(self) &#123; print("cancel\n"); &#125;      // same print, different string
&#125;

// 3. Button&lt;F&gt; is monomorphized once per F, giving two separate structs:
struct Button__lambda_ok     &#123; label: String, on_click: __lambda_ok     &#125;
struct Button__lambda_cancel &#123; label: String, on_click: __lambda_cancel &#125;

// 4. How b.on_click() is resolved. Inside Button&lt;F&gt; the only known fact is F: Fn().
//      generic form:              b.on_click()  means  F.call(b.on_click)
//      in Button__lambda_ok:      __lambda_ok::call(b.on_click)       direct call, can be inlined
//      in Button__lambda_cancel:  __lambda_cancel::call(b.on_click)   a different direct call
//    Nothing is looked up at runtime: the field's type tells the compiler
//    exactly which call() to run.

// ok     has type  Button__lambda_ok
// cancel has type  Button__lambda_cancel
// These are two unrelated types. Impossible to put in the same array.
buttons: Button[2] = [ok, cancel];   // error: type mismatch</code></pre>

<p><b>Step 3 — Fix: <code>*Fn</code> stores both the data and the code as pointers.</b></p>
<p>Instead of baking the closure type into <code>Button</code>, switch the field to <code>*Fn() -&gt; void</code>. Here is what that actually is in memory — the compiler generates <em>one</em> fixed-size pair of pointers, the same for every closure:</p>
<pre><code class="eft">// *Fn() is always this struct, regardless of which closure
// (simplified — real form preserves argument types):
struct __FnPtr &#123;
    data: *void,           // points to the closure's captured variables
    call: fn(*void),       // points to the right call() function for this closure
&#125;                          // always exactly 16 bytes

// So Button becomes:
struct Button &#123;
    label:    String,
    on_click: __FnPtr,     // 16 bytes, same shape no matter which closure
&#125;

// When you write &ok_fn, the compiler fills __FnPtr like this:
ok_fn     := || print("ok\n");      // __lambda_ok &#123; &#125; lives on the stack at address 0x100
cancel_fn := || print("cancel\n");  // __lambda_cancel &#123; &#125; lives at 0x108

//  &ok_fn     →  __FnPtr &#123; data: 0x100,  call: &__lambda_ok::call     &#125;
//  &cancel_fn →  __FnPtr &#123; data: 0x108,  call: &__lambda_cancel::call &#125;
//  The address of each call() is the one generated in Step 2, stored as a value.

ok     := Button &#123; label: "OK",     on_click: &ok_fn     &#125;;
cancel := Button &#123; label: "Cancel", on_click: &cancel_fn &#125;;

// Now ok and cancel are BOTH type Button. Same struct layout. Works.
buttons: Button[2] = [ok, cancel];

// Calling b.on_click() at runtime:
//   1. read b.on_click.data  → address of captured variables
//   2. read b.on_click.call  → address of call() function  ← one indirect jump
//   3. jump to call(), passing data as argument
for b in buttons &#123; b.on_click(); &#125;</code></pre>
<p>Compare this with Step 2. There the call was <b>direct</b>, because the type said which <code>call</code> to run. Here the type no longer says, so the address is stored in the struct and the call is <b>indirect</b>. That is the whole difference between the two steps.</p>
<p>The catch: <code>*Fn</code> is <b>non-owning</b>. The original closure structs (<code>ok_fn</code>, <code>cancel_fn</code>) must stay alive for as long as the buttons exist. This is fine when both live in the same scope.</p>

<p><b>Step 4 — The next problem: closures built dynamically inside a loop.</b></p>
<p>Now imagine you are building handlers from a list of names at runtime. The closures are created inside the loop body, so they die at the end of each iteration. Storing a <code>*Fn</code> to them would be a dangling pointer. The compiler catches this one with the local escape rule:</p>
<pre><code class="eft">names: String[3] = ["alice", "bob", "carol"];
mut handlers := Vec&lt;*Fn(Event)&gt;.new(arena);

for name in names &#123;
    f := |e: Event| print("&#123;name&#125;: &#123;e&#125;\n");
    handlers.push(&f)?;    // error: 'f' does not live long enough
&#125;                        // ← f is destroyed here, pointer would dangle</code></pre>

<p><b>Step 5 — Fix: <code>DynFn</code> owns its closure in allocated memory.</b></p>
<p><code>DynFn.new</code> copies the closure's captured struct byte-for-byte into memory from the allocator you pass in. The closure now outlives the scope it was created in, and the allocation is visible in the source. <code>DynFn.new</code> allocates, so it returns <code>DynFn&lt;...&gt;!AllocError</code>. One important detail: the copy is <b>shallow</b>. A <code>String</code> is a view (pointer + length, see section 3), so a captured <code>String</code> copies only those 16 bytes, not the characters. The characters must outlive the <code>DynFn</code>, and that remains your responsibility.</p>
<pre><code class="eft">mut handlers := Vec&lt;DynFn&lt;(Event,)&gt;&gt;.new(arena);

for name in names &#123;
    // name is a String view. These ones point at string literals in read-only
    // static data, so the characters outlive every handler and this is safe.
    // A String pointing into a buffer that is freed earlier would dangle.
    handlers.push(DynFn.new(arena, |e: Event| print("&#123;name&#125;: &#123;e&#125;\n"))?)?; // inside a function returning !AllocError
&#125;

for h in handlers &#123; h(my_event); &#125;   // safe: closure structs live in the arena

// Cleanup: arena.deinit() frees every DynFn at once.
// With another allocator, call h.deinit() on each one (DynFn : Deinit).</code></pre>

<div class="wrap"><table><tbody>
<tr><th>Situation</th><th>Syntax</th><th>Allocation</th><th>Call overhead</th></tr>
<tr><td>Function taking any closure</td><td><code>F: Fn(A) -&gt; R</code></td><td>none</td><td>none — direct, inlineable</td></tr>
<tr><td>Struct field, closures live in same scope</td><td><code>*Fn(A) -&gt; R</code></td><td>none</td><td>one indirect jump</td></tr>
<tr><td>Dynamic list, closures must own their data</td><td><code>DynFn&lt;(A,) -&gt; R&gt;</code></td><td>explicit (allocator)</td><td>one indirect jump (same as *Fn)</td></tr>
</tbody></table></div>

<ul>
<li>Closures are <b>value types</b>. Passing one copies its captured struct, just like any other struct.</li>
<li><code>Fn::call</code> takes <code>self</code> (a read-only reference), while <code>FnMut::call_mut</code> takes <code>mut self</code> (a mutable reference). Mutations inside a closure only affect the closure's struct, not the variables captured from the outer scope. To share mutable state with the outer scope, capture a raw pointer (<code>*mut T</code>) explicitly.</li>
<li>Slices, <code>String</code> views and raw pointers inside captured structs are <b>not</b> lifetime-checked. If the data they point at dies before the closure, the result is undefined. This mirrors the general rule for pointers in Eft.</li>
<li>The compiler warns — not errors — if an implicit capture exceeds 256 bytes. Writing the name in a capture list silences it.</li>
<li>A <code>DynFn</code> owns its captured struct but not what the captures point to. Free it with its allocator, or <code>deinit</code> it, as described under Memory.</li>
</ul>