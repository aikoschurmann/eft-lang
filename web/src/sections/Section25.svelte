<h3 id="mod"><span class="num">25</span>Modules, visibility, FFI, build </h3>
<pre><code class="eft">import std.io;
import net.http as http;

// Bring specific items into scope
import std.io.&#123;print, File&#125;;
// Rename on the fly
import std.io.&#123;open as open_file&#125;;
// Glob import (bring everything from 'io' into scope)
import std.io.*;

// Re-export a module so consumers of this file can use it
pub import net.http;

pub fn serve() &#123; &#125;              // private by default

extern "C" &#123;
    fn puts(s: *u8) -&gt; i32;
    fn malloc(n: usize) -&gt; *mut u8?;
&#125;

fn demo() &#123;
    unsafe &#123; puts(c"hello"); &#125;     // c"..." is a NUL-terminated *u8
&#125;
</code></pre>
<p>Every call into an <code>extern</code> function needs <code>unsafe</code>. A <code>String</code> is a pointer plus a length and is not NUL-terminated, so it cannot be passed to C directly. Use a <code>c"..."</code> literal, or copy into a buffer with a trailing <code>0</code> byte and pass its pointer.</p>
<p>A package is a directory with <code>eft.toml</code> (name, version, <code>[deps]</code> by path or git). One module per file; <code>eft build</code>, <code>eft run</code>, <code>eft test</code>, <code>eft fmt</code>.</p>

