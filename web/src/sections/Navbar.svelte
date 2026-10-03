<script>
</script>

<header class="site-header">
  <nav class="dock">
    <a href="#/" class="nav-item logo" aria-label="Home" data-tooltip="Home">
      <span class="tilde">~/</span>eft
    </a>
    <div class="divider"></div>
    <a href="#/roadmap" class="nav-item" aria-label="Roadmap" data-tooltip="Roadmap">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="3 6 9 3 15 6 21 3 21 18 15 21 9 18 3 21"></polygon><line x1="9" y1="3" x2="9" y2="18"></line><line x1="15" y1="6" x2="15" y2="21"></line></svg>
    </a>

    <a href="#/docs" class="nav-item" data-tooltip="Documentation">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"></path><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"></path></svg>
    </a>
    <a href="https://github.com" target="_blank" rel="noreferrer" class="nav-item" data-tooltip="GitHub">
      <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 19c-5 1.5-5-2.5-7-3m14 6v-3.87a3.37 3.37 0 0 0-.94-2.61c3.14-.35 6.44-1.54 6.44-7A5.44 5.44 0 0 0 20 4.77 5.07 5.07 0 0 0 19.91 1S18.73.65 16 2.48a13.38 13.38 0 0 0-7 0C6.27.65 5.09 1 5.09 1A5.07 5.07 0 0 0 5 4.77a5.44 5.44 0 0 0-1.5 3.78c0 5.42 3.3 6.61 6.44 7A3.37 3.37 0 0 0 9 18.13V22"></path></svg>
    </a>
  </nav>
</header>

<style>
  .site-header {
    display: flex;
    justify-content: center;
    width: 100%;
    margin-top: 2rem; /* Matches global container top padding */
    margin-bottom: 4rem;
    position: relative;
    z-index: 100;
  }

  .dock {
    display: flex;
    flex-direction: row;
    align-items: center;
    gap: 1.5rem;
    background: color-mix(in srgb, var(--card) 80%, transparent);
    backdrop-filter: blur(12px);
    -webkit-backdrop-filter: blur(12px);
    border: 1px solid var(--bd);
    border-radius: 12px;
    padding: 0.4rem 0.8rem;
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.05);
  }

  .nav-item {
    position: relative;
    display: flex;
    align-items: center;
    justify-content: center;
    width: 40px;
    height: 40px;
    border: none !important;
    padding: 0 !important;
    border-radius: 8px !important;
    color: var(--mut);
    background: transparent !important;
    text-decoration: none;
    transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
  }

  .nav-item:hover {
    background: var(--bd) !important;
    color: var(--fg);
  }

  .nav-item.logo {
    font-family: ui-monospace, Menlo, Consolas, monospace;
    font-weight: 800;
    font-size: 1.1rem;
    width: auto;
    padding: 0 0.8rem !important;
    color: var(--fg);
    letter-spacing: -0.02em;
  }
  
  .nav-item.logo .tilde {
    color: var(--ac);
    margin-right: 1px;
  }

  .divider {
    width: 1px;
    height: 20px;
    background: var(--bd);
    margin: 0 0.2rem;
  }

  /* Floating Tooltip */
  .nav-item::after {
    content: attr(data-tooltip);
    position: absolute;
    top: calc(100% + 15px);
    left: 50%;
    transform: translateX(-50%) translateY(-5px);
    background: #1a211d;
    color: #e4ece6;
    font-size: 0.75rem;
    font-weight: 600;
    padding: 0.4rem 0.8rem;
    border-radius: 4px;
    border: 1px solid rgba(255, 255, 255, 0.08);
    white-space: nowrap;
    opacity: 0;
    visibility: hidden;
    pointer-events: none;
    transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  }

  .nav-item:hover::after {
    opacity: 1;
    visibility: visible;
    transform: translateX(-50%) translateY(0);
  }
</style>
