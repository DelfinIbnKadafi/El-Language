import { createHighlighter } from "https://esm.sh/shiki@1.24.0";

const NAV_CONFIG_PATH = "content.json";
const EL_GRAMMAR_PATH = "assets/el.tmLanguage.json";
const HOME_PATH = "content/getting-started/welcome.md";

const sidebarEl = document.getElementById("sidebar-content");
const contentEl = document.getElementById("content");
const searchInput = document.getElementById("search-input");
const searchResults = document.getElementById("search-results");
const navToggle = document.getElementById("nav-toggle");
const sidebar = document.getElementById("sidebar");
const navScrim = document.getElementById("nav-scrim");

let pages = [];
let groups = [];
let highlighter = null;

function parseNavConfig(data) {
  const parsedGroups = [];

  for (const groupName of Object.keys(data)) {
    const group = { name: groupName, pages: [] };
    const groupPages = data[groupName] || {};

    for (const title of Object.keys(groupPages)) {
      const entry = groupPages[title] || {};
      group.pages.push({
        title,
        path: entry.file_md,
        desc: entry.desc || "",
        group: groupName,
      });
    }

    parsedGroups.push(group);
  }

  return parsedGroups;
}

function renderSidebar(activePath) {
  sidebarEl.innerHTML = "";
  for (const group of groups) {
    const groupEl = document.createElement("div");
    groupEl.className = "nav-group";

    const titleEl = document.createElement("div");
    titleEl.className = "nav-group-title";
    titleEl.textContent = group.name;
    groupEl.appendChild(titleEl);

    for (const page of group.pages) {
      const link = document.createElement("a");
      link.className = "nav-link" + (page.path === activePath ? " active" : "");
      link.textContent = page.title;
      link.href = "#" + page.path;
      groupEl.appendChild(link);
    }
    sidebarEl.appendChild(groupEl);
  }
}

function runSearch(query) {
  const q = query.trim().toLowerCase();
  searchResults.innerHTML = "";

  if (!q) {
    searchResults.classList.add("hidden");
    return;
  }

  const matches = pages.filter(
    (p) => p.title.toLowerCase().includes(q) || (p.desc || "").toLowerCase().includes(q)
  );

  if (matches.length === 0) {
    const empty = document.createElement("div");
    empty.className = "search-empty";
    empty.textContent = "No pages found";
    searchResults.appendChild(empty);
  } else {
    for (const page of matches.slice(0, 8)) {
      const item = document.createElement("div");
      item.className = "search-result-item";
      const descLine = page.desc ? escapeHtml(page.desc) : escapeHtml(page.group);
      item.innerHTML = `${escapeHtml(page.title)}<span class="group-label">${descLine}</span>`;
      item.addEventListener("click", () => {
        window.location.hash = page.path;
        searchInput.value = "";
        searchResults.classList.add("hidden");
      });
      searchResults.appendChild(item);
    }
  }

  searchResults.classList.remove("hidden");
}

searchInput.addEventListener("input", (e) => runSearch(e.target.value));
searchInput.addEventListener("focus", (e) => { if (e.target.value) runSearch(e.target.value); });
document.addEventListener("click", (e) => {
  if (!e.target.closest(".search-wrap")) searchResults.classList.add("hidden");
});

navToggle.addEventListener("click", () => {
  const open = sidebar.classList.toggle("open");
  navScrim.classList.toggle("hidden", !open);
  navToggle.setAttribute("aria-expanded", String(open));
});

navScrim.addEventListener("click", () => {
  sidebar.classList.remove("open");
  navScrim.classList.add("hidden");
  navToggle.setAttribute("aria-expanded", "false");
});

function escapeHtml(str) {
  return str.replace(/[&<>"']/g, (c) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;",
  })[c]);
}

function dirOf(path) {
  const idx = path.lastIndexOf("/");
  return idx === -1 ? "" : path.slice(0, idx + 1);
}

function resolveRelative(base, target) {
  if (/^https?:\/\//.test(target) || target.startsWith("/") || target.startsWith("#")) {
    return target;
  }
  return target;
}

function renderErrorCard(path, reason) {
  const fileName = path.split("/").pop();
  return `
    <div class="notice-card notice-card--error">
      <div class="notice-label">${escapeHtml(fileName)}</div>
      <div class="notice-box">${escapeHtml(reason)}</div>
    </div>
  `;
}

function renderFooterNav(path) {
  const index = pages.findIndex((p) => p.path === path);
  if (index === -1) return "";

  const prev = pages[index - 1];
  const next = pages[index + 1];
  if (!prev && !next) return "";

  const prevHtml = prev
    ? `<a class="footer-nav-link footer-nav-link--prev" href="#${prev.path}">
         <span class="footer-nav-label">← Previous</span>
         <span class="footer-nav-title">${escapeHtml(prev.title)}</span>
       </a>`
    : "";

  const nextHtml = next
    ? `<a class="footer-nav-link footer-nav-link--next" href="#${next.path}">
         <span class="footer-nav-label">Next →</span>
         <span class="footer-nav-title">${escapeHtml(next.title)}</span>
       </a>`
    : "";

  return `<div class="page-footer-nav">${prevHtml}${nextHtml}</div>`;
}

function expandPageLinkButtons(md, baseDir) {
  const lines = md.split("\n");
  let inFence = false;

  return lines.map((line) => {
    if (/^```/.test(line.trim())) {
      inFence = !inFence;
      return line;
    }
    if (inFence) return line;

    return line.replace(/(?<!!)\[([\w\-./]+\.md)\](?!\()/g, (match, rawPath) => {
      const resolved = resolveRelative(baseDir, rawPath);
      const known = pages.find((p) => p.path === resolved);
      const label = known ? known.title : rawPath.split("/").pop().replace(/\.md$/, "");
      return `<a class="page-link-button" href="#${resolved}">${escapeHtml(label)} →</a>`;
    });
  }).join("\n");
}

function makeRenderer(baseDir) {
  const renderer = new marked.Renderer();

  renderer.image = (href, title, text) => {
    const src = resolveRelative(baseDir, href);
    return `<img src="${src}" alt="${escapeHtml(text || "")}"${title ? ` title="${escapeHtml(title)}"` : ""} />`;
  };

  renderer.link = (href, title, text) => {
    if (/^https?:\/\//.test(href)) {
      return `<a href="${href}" target="_blank" rel="noopener">${text}</a>`;
    }
    if (href.endsWith(".md")) {
      return `<a href="#${resolveRelative(baseDir, href)}">${text}</a>`;
    }
    return `<a href="${resolveRelative(baseDir, href)}">${text}</a>`;
  };

  renderer.code = (code, infostring) => {
    const lang = (infostring || "").trim().toLowerCase();
    if (lang === "mermaid") {
      return `<div class="mermaid">${escapeHtml(code)}</div>`;
    }
    const id = "code-" + Math.random().toString(36).slice(2, 9);
    return `<pre data-lang="${lang}" data-pending="${id}"><code id="${id}">${escapeHtml(code)}</code></pre>`;
  };

  return renderer;
}

async function highlightPendingBlocks(container) {
  const blocks = container.querySelectorAll("pre[data-pending]");

  for (const pre of blocks) {
    const lang = pre.getAttribute("data-lang");
    const raw = pre.querySelector("code").textContent;

    if (lang === "el" && highlighter) {
      try {
        const html = highlighter.codeToHtml(raw, { lang: "ellang", theme: "github-dark-default" });
        const tmp = document.createElement("div");
        tmp.innerHTML = html;
        const highlighted = tmp.querySelector("pre");
        if (highlighted) {
          pre.replaceWith(highlighted);
          continue;
        }
      } catch (err) {
        console.warn("Couldn't highlight this block, leaving it plain:", err);
      }
    }
    pre.removeAttribute("data-pending");
  }
}

async function renderMermaidBlocks(container) {
  if (!window.mermaid) return;
  const blocks = container.querySelectorAll(".mermaid");
  if (!blocks.length) return;
  mermaid.initialize({ startOnLoad: false, theme: "neutral" });
  await mermaid.run({ nodes: blocks });
}

async function loadPage(path) {
  contentEl.innerHTML = '<p class="loading">Loading…</p>';

  try {
    const res = await fetch(path);
    if (!res.ok) throw new Error("not found");
    const md = await res.text();

    const baseDir = dirOf(path);
    const html = marked.parse(expandPageLinkButtons(md, baseDir), { renderer: makeRenderer(baseDir) });
    contentEl.innerHTML = html + renderFooterNav(path);

    await highlightPendingBlocks(contentEl);
    await renderMermaidBlocks(contentEl);

    renderSidebar(path);

    const activePage = pages.find((p) => p.path === path);
    document.title = activePage ? `${activePage.title} — EL Lang Docs` : "EL Lang Docs";
  } catch (err) {
    console.error("Failed to load", path, err);
    contentEl.innerHTML = renderErrorCard(path, "Can't load this page.");
  }

  sidebar.classList.remove("open");
  navScrim.classList.add("hidden");
  window.scrollTo(0, 0);
}

function currentPathFromHash() {
  return window.location.hash.replace(/^#/, "") || HOME_PATH;
}

window.addEventListener("hashchange", () => loadPage(currentPathFromHash()));

document.querySelector("[data-home]").addEventListener("click", (e) => {
  e.preventDefault();
  window.location.hash = HOME_PATH;
});

async function init() {
  const navData = await fetch(NAV_CONFIG_PATH).then((r) => r.json());
  groups = parseNavConfig(navData);
  pages = groups.flatMap((g) => g.pages);
  renderSidebar(null);

  try {
    highlighter = await createHighlighter({ themes: ["github-dark-default"], langs: [] });
    const grammar = await fetch(EL_GRAMMAR_PATH).then((r) => r.json());
    grammar.name = "ellang";
    await highlighter.loadLanguage(grammar);
  } catch (err) {
    console.warn("EL syntax highlighter didn't load, falling back to plain code blocks:", err);
  }

  await loadPage(currentPathFromHash());
}

init();
