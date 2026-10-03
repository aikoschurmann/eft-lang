<h2 class="part" id="part3">Part III · Behavior and abstraction</h2>
<h3 id="blocks"><span class="num">12</span>Struct, impl and concept </h3>
<p>Eft separates three jobs that most languages merge into one <code>class</code>. Each keyword answers exactly one question.</p>
<div class="wrap"><table><tbody>
<tr><th></th><th>Question it answers</th><th>Contains</th><th>Creates values?</th></tr>
<tr><td><code>struct</code></td><td>What data does a value hold?</td><td>Fields only</td><td>Yes</td></tr>
<tr><td><code>impl</code></td><td>What can this specific type do, and which concepts does it promise?</td><td>Methods with real bodies, plus an optional <code>: Concept</code> list</td><td>No, it attaches code to a struct or enum</td></tr>
<tr><td><code>concept</code></td><td>What must a type be able to do?</td><td>Method signatures, no bodies (a few default methods allowed)</td><td>No, it is a contract, never a value</td></tr>
</tbody></table></div>
<p>Think of it as: <b>struct</b> is the noun, <b>impl</b> is what that noun does, and <b>concept</b> is a job description that any noun can qualify for.</p>

<h4>Step 1: struct holds data</h4>
<pre><code class="eft">struct Circle &#123; r: f64 &#125;
struct Rect   &#123; w: f64, h: f64 &#125;</code></pre>
<p>A struct has no methods inside its braces. It is only the memory layout.</p>

<h4>Step 2: impl gives one type its behavior</h4>
<pre><code class="eft">impl Circle &#123;
    fn circumference(self) =&gt; 2.0 * 3.14159 * self.r;
&#125;</code></pre>
<p>Each <code>impl</code> belongs to exactly one type. It adds methods and promises nothing else unless you name a concept. Inherent impls must be for a type defined in the current package (the orphan rule).</p>

<h4>Step 3: concept states what code needs</h4>
<pre><code class="eft">concept Shape &#123;
    fn area(self) -&gt; f64;      // signature only, no body
&#125;</code></pre>
<p>A concept is a checklist. It says nothing about fields or storage, and you cannot create a <code>Shape</code> value from it.</p>

<h4>Step 4: connect them in the impl header</h4>
<p>A type joins a concept by naming it after a colon in its <code>impl</code>. The compiler then checks that every required method is present, with the right signature. A concept method with a default may be omitted. This is the only link between a type and a concept, and it lives in one visible place.</p>
<pre><code class="eft">impl Circle : Shape &#123;
    fn area(self) =&gt; 3.14159 * self.r * self.r;
    fn diameter(self) =&gt; 2.0 * self.r;      // extra methods are fine
&#125;
impl Rect : Shape + Display &#123;              // several concepts, one impl
    fn area(self) =&gt; self.w * self.h;
    fn fmt(self, w: *mut Writer, spec: fmt.Spec) -&gt; !fmt.Error &#123; /* ... */ &#125;
&#125;
impl f64 : Shape &#123; /* retroactive, on a type you don't own */ &#125;</code></pre>
<p>If <code>Circle</code> forgets <code>area</code>, the error points at <code>impl Circle : Shape</code> itself. A type that merely has a method named <code>area</code> but never declared <code>: Shape</code> does <b>not</b> satisfy the concept, so accidental matches cannot happen.</p>

<h4>What is not allowed</h4>
<ul>
<li>A <code>concept</code> cannot have fields, and a bare <code>Shape</code> cannot be a variable. Use <code>*Shape</code> for dynamic dispatch, or generic bounds like <code>S: Shape</code> (covered in the next sections).</li>
<li>A <code>struct</code> cannot contain methods. They always go in an <code>impl</code>.</li>
<li><code>impl Shape &#123; ... &#125;</code> is an error. Write <code>impl Circle : Shape</code>, never an impl for the concept itself.</li>
<li>One type cannot have two impl blocks for the same concept. Inherent impls may be split across blocks.</li>
<li>There is no inheritance. Reuse comes from composition (a struct containing another) and concept refinement (<code>concept Ord: Eq</code>).</li>
</ul>
<p class="sub">Compared with other languages: <code>struct</code> + <code>impl</code> together are roughly a class, and <code>concept</code> is roughly an interface or trait. Unlike Rust there is no separate <code>impl Trait for Type</code> block, so there is only one kind of <code>impl</code>.</p>

