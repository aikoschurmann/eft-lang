<h3 id="pat"><span class="num">8</span>Pattern matching </h3>
<p>Destructuring, guards, bindings, or-patterns, exhaustiveness checked. Enum variants can be matched unqualified (without the <code>Kind.</code> prefix) because the type being matched is known.</p>
<pre><code class="eft">enum Kind &#123; Circle &#123; r: f64 &#125;, Rect &#123; w: f64, h: f64 &#125;, Point &#125;

match kind &#123;
    Circle &#123; r &#125; if r &gt; 10.0 =&gt; "big circle",
    Circle &#123; r: _ &#125;          =&gt; "circle",
    Rect &#123; w, h &#125; if w == h  =&gt; "square",
    Rect &#123; .. &#125; | Point      =&gt; "other",
&#125;
(a, b) := pair;
if Circle &#123; r &#125; := kind &#123; print("&#123;r&#125;\n"); &#125;</code></pre>

