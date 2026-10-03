<h3 id="err"><span class="num">11</span>Errors: <code>?</code>, <code>defer</code>, <code>errdefer</code> </h3>
<p><code>T!E</code> is a result type: either a success value of type <code>T</code> or an error of type <code>E</code>. Under the hood, this is just syntactic sugar for a built-in generic enum:</p>
<pre><code class="eft">enum Result&lt;T, E&gt; &#123; ok(T), err(E) &#125;</code></pre>
<p>An error payload is usually just your own custom <code>enum</code>. There is no special "error concept".</p>

<h4>Intrinsic methods</h4>
<p>The <code>!E</code> union provides intrinsic methods for functional error handling: <code>res.is_ok()</code>, <code>res.is_err()</code>, and <code>res.map_err(|e| ...)</code>.</p>

<h4>1. Creating and returning errors</h4>
<p>Construct a result explicitly using <code>ok(value)</code> or <code>err(error_value)</code>. Because errors are just normal enums, you define them exactly like any other data.</p>
<pre><code class="eft">enum ParseError &#123; InvalidChar &#125;

fn parse_digit(c: u8) -&gt; u8!ParseError &#123;
    if c &lt; b'0' || c &gt; b'9' &#123;
        return err(ParseError.InvalidChar);
    &#125;
    return ok(c - b'0');
&#125;</code></pre>

<h4>2. Propagating errors</h4>
<p>The <code>?</code> operator unwraps the success value, or immediately returns the error to the caller. <code>errdefer</code> schedules cleanup that runs <b>only</b> if the function returns an error later on.</p>
<pre><code class="eft">enum IoError &#123; NotFound, PermissionDenied, OutOfMemory &#125;
impl IoError : From&lt;AllocError&gt; &#123;
    fn from(e: AllocError) -&gt; IoError =&gt; IoError.OutOfMemory;
&#125;

fn load(path: String, a: *mut Allocator) -&gt; Config!IoError &#123;
    // ? unwraps the File on success, or returns err(IoError) to the caller
    mut f := open(path)?;
    defer f.deinit();  // always runs when we leave the scope

    buf := std.mem.alloc_slice&lt;u8&gt;(a, f.size()?)?;
    errdefer std.mem.free_slice(a, buf);  // runs ONLY if parse() or a later step fails
    
    f.read_into(buf)?;
    
    // If parse returns Config!IoError, we can just return it directly
    return parse(buf);
&#125;</code></pre>

<h4>3. Handling errors</h4>
<p>To intercept an error instead of propagating it with <code>?</code>, use <code>match</code> on the result.</p>
<pre><code class="eft">fn main() -&gt; i32 &#123;
    match parse_digit(b'X') &#123;
        ok(val) =&gt; &#123;
            print("Parsed: &#123;val&#125;\n");
            return 0;
        &#125;,
        err(e) =&gt; &#123;
            print("Invalid character\n");
            return 1;
        &#125;
    &#125;
&#125;</code></pre>

<h4>Error conversion</h4>
<p>The <code>?</code> operator automatically converts between error types if the target type implements the <code>From</code> concept. For example, if <code>f.size()</code> returns <code>!IoError</code> but <code>load</code> returns <code>!AppError</code>, <code>?</code> will implicitly call <code>From&lt;IoError&gt;.from(e)</code>. The standard library provides a blanket impl of <code>From&lt;T&gt;</code> for <code>T</code> so that same-type errors work automatically.</p>
<pre><code class="eft">concept From&lt;S&gt; &#123;
    fn from(value: S) -&gt; Self;   // static function
&#125;

impl AppError : From&lt;IoError&gt; &#123;
    fn from(e: IoError) -&gt; AppError =&gt; AppError.Io(e);
&#125;</code></pre>

<h4><code>!E</code> as a return type</h4>
<p>If a function can fail but produces no meaningful success value, the return type is <code>!E</code>. This is syntactic sugar for <code>void!E</code>. A successful return uses <code>return ok();</code>.</p>
<pre><code class="eft">fn write_file(path: String, data: u8[..]) -&gt; !IoError &#123;
    mut f := create(path)?;
    defer f.deinit();
    f.write(data)?;
    return ok();
&#125;</code></pre>

<h4>Panics</h4>
<p>A panic is a fatal error that terminates the program (e.g., division by zero, out of bounds array access, or calling <code>assert(false)</code>). It prints a stack trace and aborts immediately. There is no unwinding and you cannot catch or recover from a panic. <b>Because it aborts, <code>defer</code> and <code>errdefer</code> do not run during a panic.</b> Use regular <code>T!E</code> errors for anything you expect to handle.</p>
<p>To explicitly panic on an error instead of handling it, use the <code>!</code> operator (e.g., <code>mut f := open("data.txt")!;</code>). This unwraps the success value or panics if it's an error. The <code>!</code> operator works identically on optionals (unwrapping or panicking on <code>null</code>).</p>
