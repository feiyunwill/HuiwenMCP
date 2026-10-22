// 仅导出编辑和解析能力，页面由后续 Qt/CEF 接入阶段创建。
import './library.css';

export { EditorState } from '@codemirror/state';
export { EditorView } from '@codemirror/view';
export { markdown, markdownLanguage } from '@codemirror/lang-markdown';
export { parser, GFM } from '@lezer/markdown';
