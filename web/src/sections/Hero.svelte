<script>
  let activeIndex = 0;

  const examples = [
    {
      name: "demo.eft",
      code: `// Constrained generics and early return (?)
fn try_fold<T, U, F>(xs: T[..], init: U, f: F) -> U? 
where F: Fn(U, T) -> U? {
    mut acc := init;
    for x in xs {
        acc = f(acc, x)?; // Propagates null!
    }
    return acc;
}

fn main() -> i32 {
    nums: i32[3] = [100, 10, 0];
    
    // Inference and fallible lambdas
    div := |a, b| if b == 0 { null } else { a / b };
    
    if let some(res) = try_fold(nums[..], 1000, div) {
        print("Result: {res}\\n");
    }
    return 0;
}`
    },
    {
      name: "memory.eft",
      code: `import std.mem.{Arena, page_allocator};

// Explicit memory management
fn build_list(a: *mut Allocator) -> *mut Node!Error {
    mut arena := Arena.new(a);
    defer arena.deinit(); // Frees everything at once

    // Struct literals and mutability
    mut head := arena.create(Node { val: 1, next: null })?;
    head.next = arena.create(Node { val: 2, next: null })?;

    return head;
}

struct Node {
    val: i32,
    next: *mut Node?,
}`
    },
    {
      name: "comptime.eft",
      code: `// Compile-time execution and reflection
fn serialize<T>(w: *mut Writer, val: T) -> !fmt.Error {
    w.write(b"{")?;
    
    // Unrolled at compile time
    comptime for f, i in fields_of(T) {
        if i > 0 { w.write(b", ")?; }
        w.print("\\\"{f.name}\\\": {val.[f.name]}")?;
    }
    
    w.write(b"}")?;
    return ok();
}

struct User { id: u64, active: bool }

fn main() {
    u := User { id: 42, active: true };
    serialize(&mut stdout, u)!;
}`
    }
  ];

  const esc = (s) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  const regex = /("[^"\n]*")|(\/\/[^\n]*)|\b(fn|return|if|else|for|while|struct|union|enum|switch|case|default|break|continue|defer|sizeof|alignof|typeof|macro|impl|concept|where|comptime|const|mut|as|in|pub|import)\b|\b(void|bool|i8|i16|i32|i64|u8|u16|u32|u64|f32|f64|isize|usize|noreturn|String|Vec|Option|Result|HashMap)\b|\b(true|false|null)\b|\b([A-Z]\w*)\b|\b([a-z_]\w*)(?=\s*\()/g;

  function highlight(s) {
    s = esc(s);
    return s.replace(regex, (match, mStr, mCom, mKw, mTy, mNum, mTyp2, mFunc) => {
        if (mStr) return '<span class="s">' + mStr + '</span>';
        if (mCom) return '<span class="c">' + mCom + '</span>';
        if (mKw) return '<span class="k">' + mKw + '</span>';
        if (mTy) return '<span class="t">' + mTy + '</span>';
        if (mNum) return '<span class="n">' + mNum + '</span>';
        if (mTyp2) return '<span class="t">' + mTyp2 + '</span>';
        if (mFunc) return '<span class="f">' + mFunc + '</span>';
        return match;
    });
  }
  let copied = false;
  async function copyInstall() {
    try {
      await navigator.clipboard.writeText("curl -sSf https://eft.lang/install.sh | sh");
    } catch (err) {
      const el = document.createElement('textarea');
      el.value = "curl -sSf https://eft.lang/install.sh | sh";
      document.body.appendChild(el);
      el.select();
      document.execCommand('copy');
      document.body.removeChild(el);
    }
    copied = true;
    setTimeout(() => copied = false, 2000);
  }
</script>

<header class="hero">
  <div class="hero-left">
    <h1 class="hero-title">Every byte,<br>accounted for.</h1>
    <p class="hero-subtitle">Eft is a small systems language with allocators, optionals, iterators and comptime. Nothing allocates unless you can see it in the source, and there is no garbage collector.</p>
    <div class="cta-group">
      <div class="hero-actions">
        <a href="#/docs" class="btn btn-primary">Read the docs</a>
        <a href="https://github.com" target="_blank" rel="noreferrer" class="btn btn-secondary">View on GitHub</a>
      </div>
      <div class="install-box">
        <div class="install-cmd">
          <span class="prompt">$</span>
          <code>curl -sSf https://eft.lang/install.sh | sh</code>
        </div>
        <button class="copy-btn" on:click={copyInstall} title="Copy to clipboard" aria-label="Copy install command">
          {#if copied}
            <svg class="copy-icon copied" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg>
          {:else}
            <svg class="copy-icon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg>
          {/if}
        </button>
      </div>
    </div>
  </div>

  <div class="hero-right">
    <div class="demo-wrapper">
      <div class="demo-tabs">
        {#each examples as ex, i}
          <button 
            class="demo-tab {i === activeIndex ? 'active' : ''}" 
            on:click={() => activeIndex = i}
          >
            {ex.name}
          </button>
        {/each}
      </div>

      <div class="demo-stack">
        {#each examples as ex, i}
          {#if i === activeIndex}
            <section class="demo">
              <div class="bar">
                <div class="dots">
                  <span class="dot red"></span>
                  <span class="dot yellow"></span>
                  <span class="dot green"></span>
                </div>
                <span class="filename">{ex.name}</span>
              </div>
              <pre class="demo-pre"><code class="eft">{@html highlight(ex.code)}</code></pre>
            </section>
          {/if}
        {/each}
      </div>
    </div>
  </div>
</header>

<style>
  .hero {
    max-width: 1120px;
    margin: 0 auto;
    padding: 1rem 1.5rem;
    display: grid;
    grid-template-columns: minmax(0, 0.85fr) minmax(0, 1.15fr);
    gap: 1.5rem;
    align-items: start;
  }

  .hero-left {
    display: flex;
    flex-direction: column;
    margin-top: 1.5rem;
  }

  .hero-title {
    font-size: clamp(32px, 4.5vw, 64px);
    line-height: 0.98;
    letter-spacing: -0.025em;
    font-weight: 800;
    margin: 0 0 0.8rem;
    color: var(--fg);
  }

  .hero-subtitle {
    color: var(--mut);
    margin: 0 0 2.5rem;
    font-size: 1.1rem;
    line-height: 1.55;
  }

  .cta-group {
    display: flex;
    flex-direction: column;
    gap: 1rem;
    width: 100%;
  }
  .hero-actions {
    display: grid;
    grid-template-columns: minmax(0, 0.9fr) minmax(0, 1.1fr);
    gap: 0.8rem;
    width: 100%;
  }
  .btn {
    display: inline-block;
    padding: 0.8rem 1.4rem;
    border-radius: 8px;
    font-weight: 800;
    text-decoration: none;
    border: 2px solid var(--fg);
    transition: all 0.2s ease;
    font-family: system-ui, sans-serif;
    text-align: center;
    box-sizing: border-box;
    width: 100%;
  }

  .btn-primary {
    background: var(--fg);
    color: var(--bg);
  }
  .btn-primary:hover {
    background: transparent;
    color: var(--fg);
  }

  .btn-secondary {
    background: transparent;
    color: var(--fg);
  }
  .btn-secondary:hover {
    background: var(--card);
  }

  .demo-wrapper {
    display: flex;
    flex-direction: column;
    gap: 1rem;
  }

  .demo-tabs {
    display: flex;
    gap: 0.5rem;
  }

  .demo-tab {
    background: transparent;
    border: 1px solid var(--bd);
    color: var(--mut);
    padding: 0.4rem 0.8rem;
    border-radius: 6px;
    font-family: ui-monospace, Menlo, Consolas, monospace;
    font-size: 0.8rem;
    cursor: pointer;
    transition: all 0.2s ease;
  }

  .demo-tab:hover {
    background: var(--card);
    color: var(--fg);
  }

  .demo-tab.active {
    background: var(--fg);
    border-color: var(--fg);
    color: var(--bg);
  }

  .demo-stack {
    position: relative;
    user-select: none;
  }

  .demo {
    background: var(--card);
    border: 1px solid var(--bd);
    border-radius: 12px;
    overflow: hidden;
    display: flex;
    flex-direction: column;
  }

  .bar {
    display: flex;
    align-items: center;
    padding: 0.8rem 1rem;
    border-bottom: 1px solid var(--bd);
    background: color-mix(in srgb, var(--card) 95%, var(--fg) 5%);
    font-family: ui-monospace, Menlo, Consolas, monospace;
    font-size: 0.8rem;
    font-weight: 500;
    color: var(--mut);
    flex-shrink: 0;
    position: relative;
  }

  .dots {
    display: flex;
    gap: 6px;
  }
  .dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
  }
  .dot.red { background: #ff5f56; }
  .dot.yellow { background: #ffbd2e; }
  .dot.green { background: #27c93f; }

  .filename {
    position: absolute;
    left: 50%;
    transform: translateX(-50%);
  }

  .demo-pre {
    margin: 0;
    padding: 1rem;
    background: var(--bg) !important;
    border: none;
    overflow-y: hidden;
    flex: 1;
  }

  .demo-pre code {
    font-family: ui-monospace, Menlo, Consolas, monospace;
    font-size: 0.9rem;
    line-height: 1.6;
    color: var(--fg);
  }

  @media (max-width: 960px) {
    .hero {
      grid-template-columns: 1fr;
      gap: 2.5rem;
      padding-top: 1rem;
    }
  }


  .install-box {
    margin-top: 0;
    width: 100%;
    box-sizing: border-box;
    display: flex;
    justify-content: space-between;
    align-items: center;
    background: #1a211d;
    border: 1px solid #2c372f;
    padding: 0.4rem 0.4rem 0.4rem 1rem;
    border-radius: 8px;
    color: #e4ece6;
    box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);
  }
  .install-cmd {
    display: flex;
    gap: 0.8rem;
    align-items: center;
    font-family: ui-monospace, Menlo, Consolas, monospace;
    font-size: 0.85rem;
  }
  .install-cmd .prompt {
    color: var(--ac);
    user-select: none;
    font-weight: bold;
  }
  .copy-btn {
    background: transparent;
    border: none;
    color: #9aaa9f;
    padding: 0.5rem;
    border-radius: 6px;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.2s ease;
  }
  .copy-btn:hover {
    background: rgba(255, 255, 255, 0.1);
    color: #fff;
  }
  .copy-btn:active {
    transform: scale(0.95);
  }
  .copy-btn .copy-icon {
    transition: all 0.2s ease;
  }
  .copy-btn .copy-icon.copied {
    color: var(--ac);
  }
</style>
