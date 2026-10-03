<script>
import { onMount } from 'svelte';
import Sidebar from './sections/Sidebar.svelte';
import Navbar from './sections/Navbar.svelte';
import Hero from './sections/Hero.svelte';
import Section01 from './sections/Section01.svelte';
import Section02 from './sections/Section02.svelte';
import Section03 from './sections/Section03.svelte';
import Section04 from './sections/Section04.svelte';
import Section05 from './sections/Section05.svelte';
import Section06 from './sections/Section06.svelte';
import Section07 from './sections/Section07.svelte';
import Section08 from './sections/Section08.svelte';
import Section09 from './sections/Section09.svelte';
import Section10 from './sections/Section10.svelte';
import Section11 from './sections/Section11.svelte';
import Section12 from './sections/Section12.svelte';
import Section13 from './sections/Section13.svelte';
import Section14 from './sections/Section14.svelte';
import Section15 from './sections/Section15.svelte';
import Section16 from './sections/Section16.svelte';
import Section17 from './sections/Section17.svelte';
import Section18 from './sections/Section18.svelte';
import Section19 from './sections/Section19.svelte';
import Section20 from './sections/Section20.svelte';
import Section21 from './sections/Section21.svelte';
import Section22 from './sections/Section22.svelte';
import Section23 from './sections/Section23.svelte';
import Section24 from './sections/Section24.svelte';
import Section25 from './sections/Section25.svelte';
import Section26 from './sections/Section26.svelte';
import Section27 from './sections/Section27.svelte';
import Section28 from './sections/Section28.svelte';
import Section29 from './sections/Section29.svelte';
import Roadmap from './sections/Roadmap.svelte';

let currentRoute = '/';

function handleHashChange() {
  const oldRoute = currentRoute;
  const hash = window.location.hash.slice(1);

  if (hash === '' || hash === '/') {
    currentRoute = '/';
  } else if (hash.startsWith('/roadmap')) {
    currentRoute = '/roadmap';
  } else if (hash.startsWith('/docs')) {
    currentRoute = '/docs';
  } else if (currentRoute === '/') {
    currentRoute = '/docs';
  }

  // If navigating across major routes, reset scroll to top
  if (oldRoute !== currentRoute && !window.location.hash.includes('#')) {
    window.scrollTo(0, 0);
  }
}

onMount(() => {
  handleHashChange();
  window.addEventListener('hashchange', handleHashChange);
  return () => window.removeEventListener('hashchange', handleHashChange);
});

// Run syntax highlighting and copy button creation when route updates
$: if (currentRoute) {
  setTimeout(() => {
    const esc = (s) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

    document.querySelectorAll("code.eft, code.c, code.cpp, code.rust").forEach(function(el){
      if (el.dataset.highlighted) return;
      el.dataset.highlighted = 'true';
      var s = esc(el.textContent);
      
      const regex = /("[^"\n]*")|(\/\/[^\n]*)|\b(fn|return|if|else|for|while|struct|union|enum|switch|case|default|break|continue|defer|sizeof|alignof|typeof|macro|impl|concept|where|comptime|const|mut|as|in|pub|import)\b|\b(void|bool|i8|i16|i32|i64|u8|u16|u32|u64|f32|f64|isize|usize|noreturn|String|Vec|Option|Result|HashMap)\b|\b(true|false|null)\b|\b([A-Z]\w*)\b|\b([a-z_]\w*)(?=\s*\()/g;
      
      s = s.replace(regex, (match, mStr, mCom, mKw, mTy, mNum, mTyp2, mFunc) => {
        if (mStr) return '<span class="s">' + mStr + '</span>';
        if (mCom) return '<span class="c">' + mCom + '</span>';
        if (mKw) return '<span class="k">' + mKw + '</span>';
        if (mTy) return '<span class="t">' + mTy + '</span>';
        if (mNum) return '<span class="n">' + mNum + '</span>';
        if (mTyp2) return '<span class="t">' + mTyp2 + '</span>';
        if (mFunc) return '<span class="f">' + mFunc + '</span>';
        return match;
      });
      el.innerHTML = s;
    });

    document.querySelectorAll("code.ebnf").forEach(function(el){
      if (el.dataset.highlighted) return;
      el.dataset.highlighted = 'true';
      var s = esc(el.textContent);
      s = s.replace(/&quot;|"[^"\n]*"/g, function(x){ return x; });
      s = s.replace(/("[^"\n]*")/g, '<span class="s">$1</span>');
      s = s.replace(/^([A-Z]\w*)(\s*::=)/gm, '<span class="t">$1</span>$2');
      el.innerHTML = s;
    });

    const copyIcon = `<svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg>`;
    const checkIcon = `<svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#2f7d57" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg>`;

    document.querySelectorAll('pre').forEach(function(el) {
      if (el.dataset.hasCopy) return;
      el.dataset.hasCopy = 'true';
      el.style.position = 'relative';
      
      var btn = document.createElement('button');
      btn.innerHTML = copyIcon;
      btn.title = "Copy to clipboard";
      btn.className = "copy-code-button";
      
      btn.addEventListener('click', function() {
        const codeNode = el.querySelector('code');
        navigator.clipboard.writeText(codeNode ? codeNode.textContent : el.textContent);
        btn.innerHTML = checkIcon;
        btn.classList.add('copied');
        setTimeout(function() { 
          btn.innerHTML = copyIcon;
          btn.classList.remove('copied');
        }, 2000);
      });
      el.appendChild(btn);
    });
  }, 50);
}
</script>

<Navbar />

{#if currentRoute === '/'}
  <Hero />
{:else}
  <div class="app-layout">
    <Sidebar {currentRoute} />
    <main class="content prose">
      {#if currentRoute === '/roadmap'}
        <Roadmap />
      {:else}
        <div class="docs-header">
          <h1 class="docs-title">The Eft programming language reference.</h1>
        </div>
        <Section01 />
        <Section02 />
        <Section03 />
        <Section04 />
        <Section05 />
        <Section06 />
        <Section07 />
        <Section08 />
        <Section09 />
        <Section10 />
        <Section11 />
        <Section12 />
        <Section13 />
        <Section14 />
        <Section15 />
        <Section16 />
        <Section17 />
        <Section18 />
        <Section19 />
        <Section20 />
        <Section21 />
        <Section22 />
        <Section23 />
        <Section24 />
        <Section25 />
        <Section26 />
        <Section27 />
        <Section28 />
        <Section29 />
      {/if}
    </main>
  </div>
{/if}
