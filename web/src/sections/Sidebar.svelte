<script>
  import { onMount, tick } from 'svelte';

  export let currentRoute = '/docs';

  let headings = [];
  let activeId = "";
  let lockedHeadingId = null;
  let tocLockUntil = 0;
  let tocListEl = null;
  let sidebarEl = null;
  let headingElements = [];

  function navigateToHeading(headingId) {
    const heading = document.getElementById(headingId);
    if (!heading) return;

    const prefers_reduced_motion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    const headingTopInViewport = heading.getBoundingClientRect().top;
    const distance = Math.abs(headingTopInViewport);
    const lockDurationMs = prefers_reduced_motion ? 0 : Math.min(1500, Math.max(450, distance * 0.8));
    const topOffset = 10;
    const targetY = Math.max(0, window.scrollY + headingTopInViewport - topOffset);

    lockedHeadingId = headingId;
    tocLockUntil = performance.now() + lockDurationMs;

    window.scrollTo({
      top: targetY,
      behavior: prefers_reduced_motion ? 'auto' : 'smooth',
    });

    const url = new URL(window.location.href);
    url.hash = headingId;
    history.replaceState(null, '', `${url.pathname}${url.search}${url.hash}`);
    activeId = headingId;
  }

  function handleTocNavigation(event, headingId) {
    event.preventDefault();
    navigateToHeading(headingId);
  }

  function isTocNavigationKey(key) {
    return key === 'ArrowDown' || key === 'ArrowRight' || key === 'ArrowUp' || key === 'ArrowLeft' || key === 'Home' || key === 'End';
  }

  function handleTocKeyNavigation(event, headingId = undefined) {
    if (!isTocNavigationKey(event.key)) return;
    if (!tocListEl) return;

    const links = Array.from(tocListEl.querySelectorAll('.toc-link'));
    if (links.length === 0) return;

    const focusedElement = document.activeElement;
    const focusedLink = focusedElement instanceof HTMLAnchorElement && tocListEl.contains(focusedElement)
      ? focusedElement
      : null;

    const focusedHeadingId = focusedLink?.dataset.headingId;
    const currentHeadingId = headingId || focusedHeadingId || activeId || links[0].dataset.headingId;

    if (!currentHeadingId) return;

    const currentIndex = links.findIndex((link) => link.dataset.headingId === currentHeadingId);
    if (currentIndex === -1) return;

    let targetIndex = currentIndex;
    if (event.key === 'ArrowDown' || event.key === 'ArrowRight') {
      targetIndex = Math.min(links.length - 1, currentIndex + 1);
    } else if (event.key === 'ArrowUp' || event.key === 'ArrowLeft') {
      targetIndex = Math.max(0, currentIndex - 1);
    } else if (event.key === 'Home') {
      targetIndex = 0;
    } else if (event.key === 'End') {
      targetIndex = links.length - 1;
    } else {
      return;
    }

    event.preventDefault();
    const targetLink = links[targetIndex];
    const targetHeadingId = targetLink.dataset.headingId;
    if (!targetHeadingId) return;

    targetLink.focus({ preventScroll: true });
    navigateToHeading(targetHeadingId);
  }

  function isTocLocked() {
    if (!lockedHeadingId) return false;
    if (performance.now() >= tocLockUntil) {
      lockedHeadingId = null;
      return false;
    }
    return true;
  }

  function keepActiveTocLinkInView(behavior = 'auto') {
    if (!activeId || !sidebarEl || !tocListEl) return;

    const safeId = typeof CSS !== 'undefined' && CSS.escape ? CSS.escape(activeId) : activeId;
    const activeLink = tocListEl.querySelector(`.toc-link[data-heading-id="${safeId}"]`);
    if (!activeLink) return;

    const containerRect = sidebarEl.getBoundingClientRect();
    const linkRect = activeLink.getBoundingClientRect();
    const linkCenter = (linkRect.top + linkRect.bottom) / 2;

    const upperBound = containerRect.top + containerRect.height * 0.32;
    const lowerBound = containerRect.top + containerRect.height * 0.68;

    const shouldRecenter = linkCenter < upperBound || linkCenter > lowerBound;
    if (!shouldRecenter) return;

    const prefers_reduced_motion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    const centeredTop = activeLink.offsetTop - (sidebarEl.clientHeight - activeLink.offsetHeight) / 2;
    const clampedTop = Math.max(0, Math.min(centeredTop, sidebarEl.scrollHeight - sidebarEl.clientHeight));

    sidebarEl.scrollTo({
      top: clampedTop,
      behavior: prefers_reduced_motion ? 'auto' : behavior
    });
  }

  $: if (activeId) {
    keepActiveTocLinkInView('smooth');
  }

  function updateActiveHeading() {
    if (headingElements.length === 0) return;
    if (isTocLocked()) {
      activeId = lockedHeadingId || activeId;
      return;
    }

    const isAtBottom = (window.innerHeight + window.scrollY) >= document.documentElement.scrollHeight - 10;
    if (isAtBottom) {
      activeId = headingElements[headingElements.length - 1].id;
      return;
    }

    const activationOffset = 180;
    let currentId = headingElements[0].id;

    for (const el of headingElements) {
      if (el.getBoundingClientRect().top <= activationOffset) {
        currentId = el.id;
      } else {
        break;
      }
    }
    activeId = currentId;
  }

  function scanHeadings() {
    const elements = Array.from(document.querySelectorAll('main.content h2[id], main.content h3[id]'));
    if (elements.length === 0) {
      headings = [];
      headingElements = [];
      activeId = "";
      return;
    }

    headingElements = elements;
    headings = elements.map((el) => {
      const isPart = el.classList.contains('part');
      const isH2 = el.tagName.toLowerCase() === 'h2';
      const depth = (isPart || isH2) ? 1 : 2;

      const clone = el.cloneNode(true);
      const toRemove = clone.querySelectorAll('.num, .top');
      toRemove.forEach(n => n.remove());
      const text = clone.textContent.trim();

      return {
        id: el.id,
        text,
        depth
      };
    });

    updateActiveHeading();
  }

  // Refresh TOC whenever currentRoute changes
  $: if (currentRoute) {
    tick().then(() => {
      setTimeout(scanHeadings, 60);
    });
  }

  onMount(() => {
    let ticking = false;

    const onScrollOrResize = () => {
      if (ticking) return;
      ticking = true;
      requestAnimationFrame(() => {
        ticking = false;
        updateActiveHeading();
      });
    };

    const onGlobalKeydown = (event) => {
      if (!tocListEl || !isTocNavigationKey(event.key)) return;
      const focusedElement = document.activeElement;
      const focusWithinToc = !!focusedElement && tocListEl.contains(focusedElement);
      const tocSidebar = tocListEl.closest('.sidebar');
      const tocHovered = tocListEl.matches(':hover') || (!!tocSidebar && tocSidebar.matches(':hover'));

      if (!focusWithinToc && !tocHovered) return;
      handleTocKeyNavigation(event);
    };

    const onHashChange = () => {
      setTimeout(scanHeadings, 60);
    };

    scanHeadings();

    window.addEventListener('scroll', onScrollOrResize, { passive: true });
    window.addEventListener('resize', onScrollOrResize);
    window.addEventListener('keydown', onGlobalKeydown);
    window.addEventListener('hashchange', onHashChange);

    return () => {
      window.removeEventListener('scroll', onScrollOrResize);
      window.removeEventListener('resize', onScrollOrResize);
      window.removeEventListener('keydown', onGlobalKeydown);
      window.removeEventListener('hashchange', onHashChange);
    };
  });
</script>

{#if headings.length > 0}
  <aside class="sidebar" bind:this={sidebarEl}>
    <div class="toc">
      <div class="toc-title">Contents</div>
      <p class="sub">
        {#if currentRoute === '/roadmap'}
          Roadmap phases and key implementation targets.
        {:else}
          Read top to bottom. Each section only uses ideas from earlier ones.
        {/if}
      </p>

      <ul class="toc-list" bind:this={tocListEl}>
        {#each headings as heading}
          <li>
            <a
              href="#{heading.id}"
              data-heading-id={heading.id}
              class="toc-link depth-{heading.depth}"
              class:active={activeId === heading.id}
              on:click={(event) => handleTocNavigation(event, heading.id)}
            >
              {heading.text}
            </a>
          </li>
        {/each}
      </ul>
    </div>
  </aside>
{/if}

<style>
  .toc-title {
    flex: 0 0 auto;
    background: var(--bg);
    padding-bottom: 0.5rem;
    margin-bottom: 0.75rem;
    position: sticky;
    top: 0;
    z-index: 1;
    display: block;
    font-size: 0.75rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.1em;
    color: var(--fg);
  }

  .sub {
    font-size: 0.85rem;
    color: var(--mut);
    margin-bottom: 1rem;
    line-height: 1.4;
  }

  .toc-list {
    overflow-y: auto;
    min-height: 0;
    scrollbar-width: none;
    list-style: none;
    padding: 0;
    margin: 0;
    display: flex;
    flex-direction: column;
    gap: 0.1rem;
    border-left: 1px solid var(--bd);
  }

  .toc-list::-webkit-scrollbar {
    display: none;
  }

  .toc-link {
    display: block;
    font-size: 0.9rem;
    color: var(--mut);
    text-decoration: none;
    line-height: 1.5;
    transition: all 0.2s ease;
    padding: 0.25rem 1rem;
    border-left: 2px solid transparent;
    margin-left: -1px;
    border-radius: 4px;
    outline: none;
  }

  .toc-link.depth-2 {
    padding-left: 2rem;
    font-size: 0.85rem;
    opacity: 0.8;
  }

  .toc-link.depth-1 {
    font-weight: 600;
  }

  .toc-link:hover {
    background: var(--bd);
  }

  .toc-link:focus,
  .toc-link:focus-visible {
    outline: none;
  }

  .toc-link.active {
    color: var(--ac);
    border-left-color: transparent;
    background: color-mix(in srgb, var(--ac) 10%, transparent);
  }
</style>