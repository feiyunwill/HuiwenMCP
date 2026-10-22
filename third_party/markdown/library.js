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

export function showPreview(source) {
  const body = document.getElementById('markdown-body');
  body.innerHTML = renderMarkdown(source);
  window.scrollTo(0, 0);
}
