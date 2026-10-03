<h2 class="part" id="part7">Part VII · Worked examples</h2>
<h3 id="example"><span class="num">27</span>Full working example </h3>
<p>Here is a complete example implementing a generic <code>HashMap</code>. It perfectly demonstrates how explicit allocators, generics, concept constraints, optionals, and pattern matching come together in Eft.</p>
<pre><code class="eft">import std.io.&#123;print&#125;;
import std.mem.&#123;Allocator, Arena, AllocError, alloc_slice, alloc_zeroed_slice, free_slice, page_allocator&#125;;

// Note: std concepts like Hash, Eq, Debug, From, Deinit, and Iterator
// are included in the Eft prelude and do not need to be imported.

enum MapError &#123;
    OutOfMemory,
    Full,
&#125;

impl MapError : Debug &#123;&#125;      // reflection-based default
impl MapError : From&lt;AllocError&gt; &#123;
    fn from(e: AllocError) -&gt; MapError =&gt; MapError.OutOfMemory;
&#125;

// 1. Generic Struct with constraints
struct HashMap&lt;K: Hash + Eq, V&gt; &#123;
    alloc: *mut Allocator,
    keys: mut K[..],
    vals: mut V[..],
    occupied: mut bool[..],
    capacity: usize,
    count: usize,
&#125;

// 2. Implementation of the generic struct.
// The bounds from the struct must be restated.
impl&lt;K, V&gt; HashMap&lt;K, V&gt; where K: Hash + Eq &#123;
    
    // Note how an explicit allocator is always injected
    pub fn new(alloc: *mut Allocator, initial_cap: usize) -&gt; HashMap&lt;K, V&gt;!MapError &#123;
        return ok(HashMap&lt;K, V&gt;&#123;
            alloc: alloc,
            keys: alloc_slice&lt;K&gt;(alloc, initial_cap)?,
            vals: alloc_slice&lt;V&gt;(alloc, initial_cap)?,
            occupied: alloc_zeroed_slice&lt;bool&gt;(alloc, initial_cap)?, // zero-init prevents reading uninitialized memory
            capacity: initial_cap,
            count: 0,
        &#125;);
    &#125;

    pub fn put(mut self, key: K, val: V) -&gt; !MapError &#123;
        // (In a real map, we would resize when load factor gets high)
        mut idx := (key.hash() as usize) % self.capacity;
        start_idx := idx;
        
        while self.occupied[idx] &#123;
            if self.keys[idx].eq(key) &#123;
                // Key exists, overwrite value
                self.vals[idx] = val;
                return ok();
            &#125;
            idx = (idx + 1) % self.capacity;
            if idx == start_idx &#123; return err(MapError.Full); &#125; // Map is completely full
        &#125;

        self.keys[idx] = key;
        self.vals[idx] = val;
        self.occupied[idx] = true;
        self.count += 1;
        return ok();
    &#125;

    // Returns an optional pointer `*V?` to avoid copying the value
    pub fn get(self, key: K) -&gt; *V? &#123;
        mut idx := (key.hash() as usize) % self.capacity;
        start_idx := idx;

        while self.occupied[idx] &#123;
            if self.keys[idx].eq(key) &#123;
                return &amp;self.vals[idx];
            &#125;
            idx = (idx + 1) % self.capacity;
            if idx == start_idx &#123; break; &#125; // Map is completely full
        &#125;
        return null;
    &#125;

&#125;

impl&lt;K, V&gt; HashMap&lt;K, V&gt; : Deinit where K: Hash + Eq &#123;
    fn deinit(mut self) &#123;
        free_slice(self.alloc, self.keys);
        free_slice(self.alloc, self.vals);
        free_slice(self.alloc, self.occupied);
    &#125;
&#125;

// 3. Using the HashMap
// The main function can return an exit code, or !E to implicitly exit on error.
pub fn main() -&gt; !MapError &#123;
    // Explicit memory management with a defer cleanup
    mut arena := Arena.new(page_allocator);
    defer arena.deinit();

    // The compiler statically enforces that `String` implements `Hash + Eq`
    // &mut arena coerces to *mut Allocator automatically
    mut map := HashMap&lt;String, i32&gt;.new(&amp;mut arena, 16)?;
    
    // Explicit cleanup (though the arena would handle it anyway)
    defer map.deinit();

    map.put("score", 100)?;
    map.put("health", 95)?;

    // Use the `if val := opt` syntax to unwrap an optional
    if val := map.get("score") &#123;
        print("Score is &#123;*val&#125;\n");
    &#125; else &#123;
        print("Score not found\n");
    &#125;

    return ok();
&#125;
</code></pre>
