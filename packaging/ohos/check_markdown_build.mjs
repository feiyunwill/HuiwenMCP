// 在临时目录检查 MD 构建链路，不调用原生编译、HAP 打包或设备运行。
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import {
  appendFileSync, cpSync, existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync
} from 'node:fs';
import { createRequire } from 'node:module';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import vm from 'node:vm';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const sandbox = mkdtempSync(path.join(os.tmpdir(), 'huiwen-md-'));
const relativeOutput = 'entry/src/main/resources/rawfile/markdown';
const sha256 = (data) => createHash('sha256').update(data).digest('hex');
const readOutput = (name) => readFileSync(path.join(sandbox, relativeOutput, name));
function run() {
  return spawnSync(process.execPath, [path.join(sandbox, 'packaging/ohos/build_markdown.mjs')], {
    cwd: sandbox, encoding: 'utf8', timeout: 180000
  });
}

try {
  const source = path.join(sandbox, 'third_party/markdown');
  mkdirSync(source, { recursive: true });
  mkdirSync(path.join(sandbox, 'packaging/ohos'), { recursive: true });
  for (const name of ['package.json', 'package-lock.json', 'library.js', 'library.css', 'preview.html', 'licenses']) {
    cpSync(path.join(root, 'third_party/markdown', name), path.join(source, name), { recursive: true });
  }
  cpSync(path.join(root, 'packaging/ohos/build_markdown.mjs'), path.join(sandbox, 'packaging/ohos/build_markdown.mjs'));
  assert.equal(existsSync(path.join(source, 'node_modules')), false);
  const first = run();
  assert.equal(first.status, 0, first.stdout + first.stderr);
  assert.match(first.stdout, /DEPENDENCIES install/);
  const api = vm.runInNewContext(`${readOutput('huiwen-markdown.js')}\nHuiwenMarkdown;`, {
    console, setTimeout, clearTimeout
  });
  assert.equal(api.EditorState.create({ doc: '# 标题' }).doc.toString(), '# 标题');
  assert.equal(typeof api.EditorView, 'function');
  assert.equal(typeof api.markdown, 'function');
  assert.ok(api.parser.parse('# 标题').toString().includes('ATXHeading1'));
  assert.ok(api.parser.configure(api.GFM).parse('| a |\n| --- |\n| b |').toString().includes('Table'));
  const html = api.renderMarkdown('# 标题\n\n**粗体**\n\n| a |\n| --- |\n| b |\n\n<script>alert(1)</script>');
  assert.match(html, /<h1>标题<\/h1>/);
  assert.match(html, /<strong>粗体<\/strong>/);
  assert.match(html, /<table>/);
  assert.ok(!html.includes('<script>'));
  assert.ok(!api.renderMarkdown('[危险链接](javascript:alert(1))').includes('href="javascript:'));
  const page = readOutput('preview.html').toString();
  assert.match(page, /Content-Security-Policy/);
  assert.ok(!page.includes('HUIWEN_PREVIEW_SCRIPT'));
  assert.match(page, /id="markdown-body"/);
  assert.match(readOutput('huiwen-markdown.css').toString(), /\.cm-editor/);
  const metadata = JSON.parse(readOutput('build.json'));
  for (const [name, expected] of Object.entries(metadata.files)) {
    const contents = readOutput(name);
    assert.equal(contents.length, expected.bytes);
    assert.equal(sha256(contents), expected.sha256);
  }
  assert.deepEqual(readOutput('THIRD_PARTY_LICENSES.txt'), readFileSync(path.join(source, 'licenses/THIRD_PARTY_LICENSES.txt')));

  rmSync(path.join(source, 'node_modules', `@esbuild/${process.platform}-${process.arch}`), { recursive: true });
  const repaired = run();
  assert.equal(repaired.status, 0, repaired.stdout + repaired.stderr);
  assert.match(repaired.stdout, /DEPENDENCIES install/);

  const initialHash = sha256(readOutput('huiwen-markdown.js'));
  appendFileSync(path.join(source, 'library.js'), '\nexport const buildProbe = "HUIWEN_MD_UPDATE_CHECK";\n');
  appendFileSync(path.join(source, 'library.css'), '\n.huiwen-md-update-check { color: red; }\n');
  const second = run();
  assert.equal(second.status, 0, second.stdout + second.stderr);
  assert.match(second.stdout, /DEPENDENCIES cached/);
  assert.notEqual(sha256(readOutput('huiwen-markdown.js')), initialHash);
  assert.match(readOutput('huiwen-markdown.js').toString(), /HUIWEN_MD_UPDATE_CHECK/);
  assert.match(readOutput('huiwen-markdown.css').toString(), /huiwen-md-update-check/);
  const validHash = sha256(readOutput('huiwen-markdown.js'));
  appendFileSync(path.join(source, 'library.js'), '\nexport const invalid = ;\n');
  const failed = run();
  assert.equal(failed.status, 1);
  assert.match(failed.stderr, /HUIWEN_MD.*FAILED/);
  assert.equal(sha256(readOutput('huiwen-markdown.js')), validHash);

  // 验证真实 hvigorfile 的任务注册、先后依赖以及未吞掉构建异常。
  const require = createRequire(path.join(source, 'package.json'));
  const transformed = require('esbuild').transformSync(readFileSync(path.join(root, 'entry/hvigorfile.ts'), 'utf8'), {
    loader: 'ts', format: 'cjs'
  });
  const tasks = [];
  const calls = [];
  const injected = new Error('build failed');
  const module = { exports: {} };
  vm.runInNewContext(transformed.code, {
    module, exports: module.exports, process,
    require: (name) => {
      if (name === '@ohos/hvigor-ohos-plugin') return { hapTasks: {} };
      if (name === 'child_process') return { execFileSync: (...args) => { calls.push(args); throw injected; } };
      return require(name);
    }
  });
  for (const plugin of module.exports.default.plugins) plugin.apply({ registerTask: (task) => tasks.push(task) });
  const task = tasks.find((item) => item.name === 'PrepareMarkdownResources');
  assert.ok(task);
  assert.ok(task.dependencies.includes('default@PreBuild'));
  assert.ok(task.dependencies.includes('PrepareNativeDependencies'));
  for (const name of ['default@ConfigureCmake', 'default@CompileResource', 'default@PackageHap']) {
    assert.ok(task.postDependencies.includes(name));
  }
  assert.throws(() => task.run({ modulePath: path.join(sandbox, 'entry') }), (error) => error === injected);
  assert.equal(calls[0][0], process.execPath);
  assert.equal(calls[0][1][0], path.join(sandbox, 'packaging/ohos/build_markdown.mjs'));
  console.info('MARKDOWN_BUILD_OK: 首次还原、缺包修复、离线库接口、源码更新、失败阻断、任务注册检查通过');
} finally {
  rmSync(sandbox, { recursive: true, force: true });
}
