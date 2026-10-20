// 从系统文档选择器导入文件，并在应用私有沙箱中安排输入、输出路径。
import { common } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { fileIo, fileUri, picker } from '@kit.CoreFileKit';
import hilog from '@ohos.hilog';

interface WorkFilePaths {
  inputDir: string;
  outputDir: string;
  inputPath: string;
  outputPath: string;
}

function dailyDocumentPaths(root: string, name: string, extension: string, forConversion: boolean): WorkFilePaths {
  const date: Date = new Date();
  const day: string = `${date.getFullYear()}-${String(date.getMonth() + 1).padStart(2, '0')}-${String(date.getDate()).padStart(2, '0')}`;
  const inputDir: string = `${root}/inputs/${day}`;
  const outputDir: string = forConversion ? `${root}/outputs/${day}` : '';
  let stem: string = name.slice(0, name.lastIndexOf('.')).replace(/[\\/:*?"<>|\u0000-\u001f]/g, '_').slice(0, 80).trim();
  const last: number = stem.charCodeAt(stem.length - 1);
  if (last >= 0xD800 && last <= 0xDBFF) {
    stem = stem.slice(0, -1);
  }
  if (stem.length === 0 || stem === '.' || stem === '..') {
    stem = 'document';
  }
  for (let index: number = 1; index <= 10000; index++) {
    const suffix: string = index === 1 ? '' : ` (${index})`;
    const inputPath: string = `${inputDir}/${stem}${suffix}.${extension}`;
    const outputPath: string = forConversion ? `${outputDir}/${stem}${suffix}.md` : '';
    if (!sandboxPathExists(inputPath) && (!forConversion || !sandboxPathExists(outputPath))) {
      return { inputDir, outputDir, inputPath, outputPath };
    }
  }
  throw new Error('同名文件过多，请更换文件名');
}

function sandboxPathExists(path: string): boolean {
  try {
    return fileIo.accessSync(path);
  } catch (error) {
    const failure = error as BusinessError;
    if (failure.code === 13900002) {
      return false;
    }
    throw importError('检查沙箱文件', failure);
  }
}

function importError(stage: string, error: BusinessError): Error {
  const code = String(error.code ?? 'unknown');
  hilog.error(0xFF00, 'DocumentStore', '%{public}s failed: code=%{public}s', stage, code);
  return new Error(`${stage}失败：${error.message} (code=${code})`);
}

// 沙箱目录会被多次使用，已存在时直接复用。
function ensureDirectory(path: string, stage: string): void {
  try {
    fileIo.mkdirSync(path, true);
  } catch (error) {
    const failure = error as BusinessError;
    if (failure.code !== 13900015) {
      throw importError(stage, failure);
    }
  }
}

export enum DocumentKind {
  PDF = 'pdf',
  Markdown = 'md'
}

export interface PickedDocument {
  name: string;
  uri: string;
  inputPath: string;
  outputPath: string;
}

export async function pickDocumentToSandbox(context: common.UIAbilityContext, kind: DocumentKind,
  forConversion: boolean): Promise<PickedDocument | null> {
  const options = new picker.DocumentSelectOptions();
  options.maxSelectNumber = 1;
  options.fileSuffixFilters = kind === DocumentKind.PDF ? ['PDF|.pdf'] : ['Markdown|.md,.markdown'];
  options.defaultFilePathUri = 'file://docs/storage/Users/currentUser';

  const uris = await new picker.DocumentViewPicker(context).select(options);
  if (uris.length === 0) {
    return null;
  }

  const uri = uris[0];
  const name = new fileUri.FileUri(uri).name;
  const valid = kind === DocumentKind.PDF ? /\.pdf$/i.test(name) : /\.(md|markdown)$/i.test(name);
  if (!valid) {
    throw new Error('请选择对应格式的文件');
  }

  // 使用应用级 filesDir；设备上的物理目录位于当前用户的包名沙箱下。
  const filesDir = context.getApplicationContext().filesDir;
  const extension = name.toLowerCase().endsWith('.markdown') ? 'markdown' : kind;
  const paths: WorkFilePaths = dailyDocumentPaths(filesDir, name, extension, forConversion);
  const inputPath: string = paths.inputPath;
  const outputPath: string = paths.outputPath;
  ensureDirectory(paths.inputDir, '创建输入目录');
  if (forConversion) {
    ensureDirectory(paths.outputDir, '创建输出目录');
  }

  // Picker 的 URI 只有选取授权；复制后 C++ 和 Python 使用沙箱内的路径。
  let source: fileIo.File;
  try {
    source = await fileIo.open(uri, fileIo.OpenMode.READ_ONLY);
  } catch (error) {
    throw importError('打开所选文件', error as BusinessError);
  }
  try {
    await fileIo.copyFile(source.fd, inputPath);
  } catch (error) {
    try {
      fileIo.unlinkSync(inputPath);
    } catch (_) {
      // 复制失败且目标文件不存在时无需清理。
    }
    throw importError('复制所选文件', error as BusinessError);
  } finally {
    await fileIo.close(source);
  }

  hilog.info(0xFF00, 'DocumentStore', 'HUIWEN_OUTPUT stage=import_success input=%{public}s output=%{public}s',
    inputPath, outputPath);
  return { name, uri, inputPath, outputPath };
}
