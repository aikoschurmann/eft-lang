<div class="roadmap-page">
  <div class="docs-header">
    <h1 class="docs-title">Eft Roadmap</h1>
    <p class="sub">The path to 1.0 and beyond.</p>
  </div>

  <h2 id="phase-1">Phase 1: Compiler Frontend</h2>
  <p class="phase-meta">Laying the groundwork for the compiler architecture, syntax parsing, and type resolution.</p>
  <div class="task-list">
    <div class="task">
      <div class="status-col"><span class="status done">Done</span></div>
      <div class="task-content">
        <strong>Language Design & EBNF <span class="task-date">(1 Oct 2026)</span></strong>
        <p>Finalizing the core grammar, syntax rules, and type system semantics.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status done">Done</span></div>
      <div class="task-content">
        <strong>Lexer & Tokenizer <span class="task-date">(2 Oct 2026)</span></strong>
        <p>Source text tokenization, string interning, and source location tracking for errors.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status wip">In Progress</span></div>
      <div class="task-content">
        <strong>Parser & AST</strong>
        <p>Pratt parsing for expressions, AST construction, and syntax error recovery.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Type Checker & Resolution</strong>
        <p>Concept bounds checking, trait solving, and return type inference for <code>=&gt;</code> bodies.</p>
      </div>
    </div>
  </div>

  <h2 id="phase-2">Phase 2: Backend & Bootstrapping</h2>
  <p class="phase-meta">Generating executable code and establishing the foundation for memory safety enforcement.</p>
  <div class="task-list">
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Code Generation</strong>
        <p>Initial C-backend translation to get the compiler bootstrapping quickly.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Safety Analyzer</strong>
        <p>Enforcing the local pointer escape rule and checking for uninitialized memory.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Debug Allocator</strong>
        <p>Runtime leak, use-after-free, and double-free detection built into the default memory allocator.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>FFI Basics</strong>
        <p>Seamless C interoperability so Eft can call <code>malloc</code>, <code>write</code>, and standard libc functions.</p>
      </div>
    </div>
  </div>

  <h2 id="phase-3">Phase 3: Core Standard Library</h2>
  <p class="phase-meta">Building native language constructs and core standard library utilities in Eft itself.</p>
  <div class="task-list">
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Memory Management</strong>
        <p>Implementing <code>Arena</code>, <code>Allocator</code>, and the <code>Deinit</code> concept.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Collections</strong>
        <p>Building <code>String</code>, <code>Vec&lt;T&gt;</code>, and <code>HashMap&lt;K, V&gt;</code> using the new compiler.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Systems Extras</strong>
        <p><code>packed struct</code>, memory alignment constraints (<code>align(16)</code>), and bitfields.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Concurrency Primitives</strong>
        <p><code>spawn</code>, channels (<code>Chan&lt;T&gt;</code>), and the <code>Send</code> concept for safe thread-boundary crossing.</p>
      </div>
    </div>
  </div>

  <h2 id="phase-4">Phase 4: Ecosystem, Tooling & 1.0</h2>
  <p class="phase-meta">Polish, stabilization, developer experience tooling, and self-hosting.</p>
  <div class="task-list">
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Package Manager</strong>
        <p>A unified build system, dependency manager, and module registry.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Language Server (LSP)</strong>
        <p>Robust IDE support, autocomplete, jump-to-definition, and inline diagnostics.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Self-hosted Compiler</strong>
        <p>Bootstrapping by rewriting the Eft compiler completely in Eft.</p>
      </div>
    </div>
    <div class="task">
      <div class="status-col"><span class="status todo">Planned</span></div>
      <div class="task-content">
        <strong>Hardware Specifics</strong>
        <p>Inline <code>asm</code> and SIMD primitives (<code>simd&lt;f32, 4&gt;</code>).</p>
      </div>
    </div>
  </div>
</div>

<style>
  h2 {
    /* Inherits sizing from app.css; just adds the underline specifically for roadmap headers */
    padding-bottom: 0.5rem;
    border-bottom: 1px solid var(--bd);
  }

  /* Phase Metadata */
  .phase-meta {
    font-size: 0.95em;
    color: var(--mut);
    margin-top: -1.2rem;
    margin-bottom: 2rem;
    line-height: 1.5;
  }

  /* Task List Styles */
  .task-list {
    display: flex;
    flex-direction: column;
    gap: 1.5rem;
    margin-bottom: 3rem;
  }

  .task {
    display: flex;
    align-items: flex-start;
    gap: 1.25rem;
  }

  .status-col {
    flex: 0 0 110px;
    padding-top: 0.1rem;
  }

  .status {
    display: inline-block;
    font-size: 0.65em;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    padding: 0.25rem 0.5rem;
    border-radius: 6px;
    border: 1px solid currentColor;
    text-align: center;
    width: 100%;
    white-space: nowrap;
  }

  .status.done {
    color: #10b981;
    background: rgba(16, 185, 129, 0.1);
  }

  .status.wip {
    color: #f59e0b;
    background: rgba(245, 158, 11, 0.1);
  }

  .status.todo {
    color: var(--mut);
    background: rgba(150, 150, 150, 0.05);
  }

  .task-content {
    flex-grow: 1;
  }

  .task-content strong {
    display: block;
    color: var(--fg);
    font-size: 1em;
    font-weight: 700;
    margin-bottom: 0.3rem;
    line-height: 1.3;
  }

  .task-date {
    font-size: 0.85em;
    color: var(--mut);
    font-weight: 400;
    margin-left: 0.5rem;
  }

  .task-content p {
    margin: 0;
    color: var(--mut);
    font-size: 0.9em;
    line-height: 1.5;
  }
</style>