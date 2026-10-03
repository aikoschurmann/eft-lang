<h3 id="flow"><span class="num">5</span>Control flow </h3>
<h4>if is an expression</h4>
<pre><code class="eft">if ready &#123; start(); &#125;                       // statement form: else is optional

sign := if x &lt; 0 &#123; -1 &#125; else if x == 0 &#123; 0 &#125; else &#123; 1 &#125;;   // value form: else is required

if i := find(xs, 7) &#123; print("at &#123;i&#125;\n"); &#125;  // bind an optional when present</code></pre>
<p>No parentheses around the condition, braces are mandatory, and the condition must be <code>bool</code>. In value form every branch must have the same type.</p>
<h4>while, for, break, continue</h4>
<pre><code class="eft">mut i := 0;
while i &lt; 10 &#123; i += 1; &#125;

for k in 0..5 &#123; &#125;                     // 0,1,2,3,4
for k in 0..=5 &#123; &#125;                    // 0..5 inclusive
for x in xs &#123; &#125;                       // any slice or Iterator
for (i, x) in xs.enumerate() &#123; &#125;

for x in xs &#123;
    if x &lt; 0 &#123; continue; &#125;            // next iteration
    if x == 99 &#123; break; &#125;             // leave the innermost loop
&#125;

loop &#123; /* runs until break or return */ &#125;</code></pre>
<p><code>break</code> and <code>continue</code> affect the innermost loop unless you name a label. A label is written <code>@name:</code> before the loop and <code>@name</code> after <code>break</code> or <code>continue</code>.</p>
<pre><code class="eft">@outer: for row in grid &#123;
    for x in row &#123;
        if x &lt; 0 &#123; continue @outer; &#125;     // next row
        if x == target &#123; break @outer; &#125;   // leave both loops
    &#125;
&#125;

first := loop &#123;                            // loop is an expression
    v := next();
    if v &gt; 10 &#123; break v; &#125;                // break carries the value
&#125;;</code></pre>
<p>Only <code>loop</code> can break with a value, because <code>while</code> and <code>for</code> may run zero times. Every <code>break</code> in one <code>loop</code> must give the same type, and a bare <code>break;</code> makes the loop <code>void</code>. <code>while true</code> still works, but <code>loop</code> is the idiom for an infinite loop.</p>
<h4>return and defer</h4>
<pre><code class="eft">fn first_even(xs: i32[..]) -&gt; i32? &#123;
    for x in xs &#123; if x % 2 == 0 &#123; return x; &#125; &#125;   // early return
    null
&#125;

fn work() &#123;
    mut f := open("data.txt")!;
    defer f.deinit();          // runs when the scope exits, in reverse order of declaration
    /* use f; every return path closes it */
&#125;</code></pre>
<p><code>match</code> is the multi-way branch. It is introduced under Pattern matching because it works on shapes of data, not just values.</p>

