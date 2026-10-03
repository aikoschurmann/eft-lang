<h3 id="fmt"><span class="num">21</span>String formatting </h3>
<p>String interpolation uses curly braces: <code>"&#123;expr&#125;"</code>. This does not allocate a string; instead, the compiler turns an interpolated literal into a lightweight <code>fmt.Args</code> value that describes the format. This works for any type that implements the <code>Display</code> concept. Adding <code>:?</code> uses the <code>Debug</code> concept instead, which is intended for developer-facing output.</p>
<pre><code class="eft">print("len = &#123;v.len()&#125;\n");
print("raw bytes = &#123;v:?&#125;\n");</code></pre>

<h4>Reader and Writer</h4>
<p>I/O and formatting are built around the <code>Reader</code> and <code>Writer</code> concepts. <code>Reader</code> streams bytes in from a source, and <code>Writer</code> streams bytes out. Because a writer could be a file (failing with an I/O error) or a memory buffer (failing with an allocation error), formatting functions return a generic <code>!fmt.Error</code> to indicate that writing failed.</p>
<pre><code class="eft">concept Reader &#123;
    // Returns the number of bytes read, or 0 on EOF
    fn read(mut self, buf: mut u8[..]) -&gt; usize!fmt.Error;
&#125;

concept Writer &#123;
    fn write(mut self, data: u8[..]) -&gt; !fmt.Error;
    
    // Default method to write an interpolated format block
    fn print(mut self, args: fmt.Args) -&gt; !fmt.Error &#123; /* ... */ &#125;
&#125;

// std.fmt
struct Spec &#123;
    fill: Rune = ' ',
    align: Align = Align.Left,
    width: usize = 0,
    precision: usize? = null,
    kind: Kind = Kind.Default,
&#125;
enum Align &#123; Left, Right, Center &#125;
enum Kind  &#123; Default, Hex, HexUpper, Binary &#125;
enum Error &#123; Io, OutOfMemory &#125;
impl Error : From&lt;AllocError&gt; &#123;
    fn from(e: AllocError) -&gt; Error =&gt; Error.OutOfMemory;
&#125;

// fmt.Args: the compiler-built description of an interpolated literal.
// A list of segments, each either literal text or (pointer to value, formatter, Spec).
// It holds no allocation and does not outlive the statement that built it.

concept Display &#123;
    fn fmt(self, w: *mut Writer, spec: fmt.Spec) -&gt; !fmt.Error;
&#125;
concept Debug &#123;
    // default: prints the type name and its fields or variant, via reflection
    fn debug_fmt(self, w: *mut Writer, spec: fmt.Spec) -&gt; !fmt.Error &#123;
        /* comptime for over fields_of(Self) / variants_of(Self) */
    &#125;
&#125;
// To get the default, write an empty impl: impl MapError : Debug &#123;&#125;. There is no automatic implementation.

struct Point &#123; x: i32, y: i32 &#125;
impl Point : Display &#123;
    fn fmt(self, w: *mut Writer, spec: fmt.Spec) -&gt; !fmt.Error &#123;
        return w.print("(&#123;self.x&#125;, &#123;self.y&#125;)");
    &#125;
&#125;</code></pre>

<p><code>"&#123;x&#125;"</code> calls <code>Display.fmt(x, w, spec)</code>. <code>"&#123;x:?&#125;"</code> calls <code>Debug.debug_fmt(x, w, spec)</code>. Any other specifier after the colon fills in <code>spec</code>. A literal brace is written <code>&#123;&#123;</code> or <code>&#125;&#125;</code>.</p>

<h4>Format specifiers</h4>
<p>You can format numbers by adding a specifier after the colon:</p>
<ul>
<li><code>&#123;pi:.3&#125;</code> — float precision (3 decimal places)</li>
<li><code>&#123;n:04&#125;</code> — zero padding (minimum width 4)</li>
<li><code>&#123;n:x&#125;</code> — hex (lowercase), <code>&#123;n:X&#125;</code> for uppercase</li>
<li><code>&#123;s:&gt;10&#125;</code> — right align, width 10</li>
</ul>

<h4>Growable strings</h4>
<p>Because formatting requires a <code>Writer</code>, building a string in memory means writing to a growable buffer that owns its memory.</p>
<pre><code class="eft">mut buf := StringBuf.new(alloc);
defer buf.deinit();

buf.print("Hello, &#123;name&#125;!")?;
s: String = buf.as_str(); // slice view over the buffer</code></pre>
<p>This distinguishes <code>String</code> (a lightweight, non-owning slice guaranteed to be UTF-8, see Section 3) from <code>StringBuf</code> (a heap-allocated container).</p>

<h4>Parsing</h4>
<p>To convert strings back into values, types implement the <code>Parse</code> concept. This is the exact inverse of <code>Display</code>.</p>
<pre><code class="eft">concept Parse &#123;
    fn parse(s: String) -&gt; Self!fmt.Error;
&#125;

// Usage:
n := i32.parse("123")?;</code></pre>
