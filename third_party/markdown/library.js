// 编辑能力保持独立；只读预览在内存中渲染 MD 原文，不另存每篇文档的 HTML。
import './library.css';
import MarkdownIt from 'markdown-it';

export { EditorState } from '@codemirror/state';
export { EditorView } from '@codemirror/view';
export { markdown, markdownLanguage } from '@codemirror/lang-markdown';
export { parser, GFM } from '@lezer/markdown';

const renderer = new MarkdownIt({ html: false, linkify: false, typographer: false });

export function renderMarkdown(source) {
  return renderer.render(source);
}

// 只识别顶层独立段落中的页码，代码块、列表和普通正文中的同样文字不作锚点。
export function renderPreview(source) {
  const environment = {};
  const tokens = renderer.parse(source, environment);
  const markers = [];
  for (let i = 0; i < tokens.length - 2; i++) {
    if (tokens[i].type !== 'paragraph_open' || tokens[i].level !== 0 ||
        tokens[i + 1].type !== 'inline' || tokens[i + 2].type !== 'paragraph_close') continue;
    const match = /^<!-- PDF page ([1-9]\d{0,5}) -->$/.exec(tokens[i + 1].content.trim());
    if (match && Number(match[1]) <= 100000) markers.push({ index: i, page: Number(match[1]) });
  }
  if (!markers.length || markers.some((marker, i) => i && marker.page <= markers[i - 1].page)) {
    return renderMarkdown(source);
  }
  const render = (start, end) => renderer.renderer.render(tokens.slice(start, end), renderer.options, environment);
  return render(0, markers[0].index) + markers.map((marker, i) =>
    `<section class="pdf-page" data-pdf-page="${marker.page}">` +
    render(marker.index + 3, markers[i + 1]?.index ?? tokens.length) + '</section>'
  ).join('');
}

let pages = [];
let pageIndexes = new Map();
let listening = false;
let framePending = false;
let observer;
let appliedTop = null;
let lastPosition;
let initialReport = false;
const clamp = (value) => Math.max(0, Math.min(1, value));
const scrollRange = () => Math.max(0, document.documentElement.scrollHeight - window.innerHeight);
const pageTop = (page) => page.getBoundingClientRect().top + window.scrollY;

export function scrollPosition() {
  const top = window.scrollY;
  const range = scrollRange();
  let low = 0;
  let high = pages.length;
  while (low < high) {
    const middle = (low + high) >>> 1;
    if (pageTop(pages[middle]) <= top) low = middle + 1;
    else high = middle;
  }
  const index = Math.max(0, low - 1);
  const page = pages[index];
  const start = page ? pageTop(page) : 0;
  const end = pages[index + 1] ? pageTop(pages[index + 1]) :
    start + (page?.getBoundingClientRect().height ?? 1);
  return {
    page: page ? Number(page.dataset.pdfPage) : 0,
    fraction: clamp((top - start) / Math.max(1, end - start)),
    progress: range ? clamp(top / range) : 0
  };
}

export function setScrollPosition(page, fraction, progress) {
  if (![page, fraction, progress].every(Number.isFinite)) return;
  const range = scrollRange();
  const index = pageIndexes.get(page) ?? -1;
  let target = clamp(progress) * range;
  if (index >= 0 && progress > 0 && progress < 1) {
    const start = pageTop(pages[index]);
    const end = pages[index + 1] ? pageTop(pages[index + 1]) :
      start + pages[index].getBoundingClientRect().height;
    target = start + clamp(fraction) * Math.max(1, end - start);
  }
  window.scrollTo({ top: Math.max(0, Math.min(range, target)), behavior: 'instant' });
  // 保留目标值直至实际位置变化，异步 scroll 回调不能再次反向通知 Qt。
  appliedTop = window.scrollY;
  lastPosition = scrollPosition();
}

function publishPosition() {
  framePending = false;
  lastPosition = scrollPosition();
  if (!initialReport && appliedTop !== null && Math.abs(window.scrollY - appliedTop) < 1) return;
  initialReport = false;
  appliedTop = null;
  const { page, fraction, progress } = lastPosition;
  window.huiwenReportScroll?.(page, fraction, progress);
}

function queuePosition() {
  if (framePending) return;
  framePending = true;
  window.requestAnimationFrame(publishPosition);
}

function restorePosition() {
  if (lastPosition) {
    const { page, fraction, progress } = lastPosition;
    setScrollPosition(page, fraction, progress);
  }
}

export function showPreview(source) {
  const body = document.getElementById('markdown-body');
  observer?.disconnect();
  body.innerHTML = renderPreview(source);
  pages = [...body.querySelectorAll('.pdf-page')];
  pageIndexes = new Map(pages.map((page, i) => [Number(page.dataset.pdfPage), i]));
  window.scrollTo(0, 0);
  appliedTop = null;
  initialReport = true;
  lastPosition = scrollPosition();
  if (!listening) {
    window.addEventListener('scroll', queuePosition, { passive: true });
    window.addEventListener('resize', restorePosition);
    listening = true;
  }
  if (typeof ResizeObserver !== 'undefined') {
    observer = new ResizeObserver(restorePosition);
    observer.observe(body);
  }
  // 首次通知也作为正文已渲染的握手，原生端随后才开启同步。
  queuePosition();
}
