// 系统选择器负责选取授权，沙箱导入由 C++ NAPI 完成。
import { common } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { fileIo, fileUri, picker } from '@kit.CoreFileKit';
import hilog from '@ohos.hilog';
import nativeBridge, { HarmonySandboxDocument } from 'libhuiwen_native.so';

function importError(stage: string, error: BusinessError): Error {
  const code = String(error.code ?? 'unknown');
  hilog.error(0xFF00, 'DocumentStore', '%{public}s failed: code=%{public}s', stage, code);
  return new Error(`${stage}失败：${error.message} (code=${code})`);
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
  let source: fileIo.File;
  try {
    source = await fileIo.open(uri, fileIo.OpenMode.READ_ONLY);
  } catch (error) {
    throw importError('打开所选文件', error as BusinessError);
  }
  try {
    const document: HarmonySandboxDocument = await nativeBridge.harmonyImportDocument(
      context.getApplicationContext().filesDir, name, source.fd, kind, forConversion);
    const result: PickedDocument = {
      name,
      uri,
      inputPath: document.inputPath,
      outputPath: document.outputPath
    };
    return result;
  } finally {
    await fileIo.close(source);
  }
}
