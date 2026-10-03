<h2 class="part" id="part6">Part VI · Compile time and projects</h2>
<h3 id="ct"><span class="num">24</span>comptime </h3>
<p>Eft replaces macros, const generics, and <code>static_assert</code> with one mechanism: running ordinary Eft code at compile time. By prefixing a block, function, or loop with <code>comptime</code>, you guarantee it executes during compilation.</p>

<h4>What is allowed at compile time</h4>
<p>You can use normal control flow, loops, and math. However, the compile-time execution environment is restricted:</p>
<ul>
<li><b>Allowed:</b> Math, struct creation, array manipulation, and calling other <code>comptime</code> functions.</li>
<li><b>Not allowed:</b> Calling non-comptime functions, I/O (no reading files or network), system calls, and global state mutation.</li>
<li><b>Memory:</b> You cannot use explicit allocators (like <code>Arena</code>) at compile time. All compile-time state must live in variables or arrays of known size.</li>
</ul>

<h4>Const parameters and producing types</h4>
<p>Generics can take values instead of types. A parameter marked <code>const Ident: Type</code> expects a compile-time known value. You can use these values to construct types or size arrays:</p>
<pre><code class="eft">struct Array&lt;T, const N: usize&gt; &#123;
    data: T[N],
&#125;

// Using the const parameter:
mut buf := Array&lt;u8, 1024&gt; &#123; /* ... */ &#125;;</code></pre>

<h4>Reflection and unrolling</h4>
<p>Instead of relying on macro syntax, Eft provides built-in <code>comptime</code> reflection functions. A <code>comptime for</code> loop unrolls its body once for each iteration, allowing you to generate code dynamically based on type information.</p>
<ul>
<li><code>size_of(T)</code> returns the byte size of a type.</li>
<li><code>fields_of(T)</code> returns a compile-time array of field metadata (name, type, offset).</li>
<li><code>variants_of(T)</code> returns a compile-time array of enum variant metadata (name, payload fields).</li>
<li><code>implements&lt;T, C&gt;()</code> returns true if type <code>T</code> satisfies concept <code>C</code>.</li>
<li><code>x.[name]</code> lets you access a field of a value if <code>name</code> is a compile-time string.</li>
</ul>

<pre><code class="eft">fn dump&lt;T&gt;(x: T) &#123;
    // The compiler unrolls this loop, generating one print statement per field.
    // If T has fields 'age' and 'name', it generates:
    // print("age = &#123;x.age&#125;\n");
    // print("name = &#123;x.name&#125;\n");
    comptime for f in fields_of(T) &#123;
        print("&#123;f.name&#125; = &#123;x.[f.name]&#125;\n");
    &#125;
&#125;</code></pre>
<p>Note that <code>comptime for</code> bodies over <code>fields_of(T)</code>, and any concept default methods that use them, are an explicit exception to the generic checking rules: they are checked <b>per instantiation</b>. This is why <code>x.[f.name]</code> works inside <code>dump</code> even though <code>T</code> is unbounded.</p>

<h4>Static assertions</h4>
<p><code>comptime</code> blocks can be used to assert properties about types before the program ever runs. The <code>compile_error("...")</code> built-in immediately stops compilation with your custom message.</p>
<pre><code class="eft">comptime &#123;
    if size_of(Header) != 16 &#123;
        compile_error("header layout must be exactly 16 bytes");
    &#125;
&#125;</code></pre>
