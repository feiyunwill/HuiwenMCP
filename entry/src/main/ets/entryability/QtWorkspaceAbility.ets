// Qt 独立窗口入口；保留自己的 Context 处理文件选择，首页结束后仍可使用。
import { UIAbility, Want, AbilityConstant, OpenLinkOptions } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { window } from '@kit.ArkUI';
import { fileUri } from '@kit.CoreFileKit';
import hilog from '@ohos.hilog';
import qpa from 'libplugins_platforms_qopenharmony.so';
import nativeBridge from 'libhuiwen_native.so';
import { DocumentKind, pickDocumentToSandbox } from '../utils/DocumentStore';

interface WorkspaceEvent {
  action: string;
  path: string;
}

interface WorkspaceReply {
  action: string;
  name: string;
  inputPath: string;
  outputPath: string;
  error: string;
}

export default class QtWorkspaceAbility extends UIAbility {
  // QPA 读取这两个字段，加载应用共享库并调用其中的 main()。
  private launchApplication: string = 'libhuiwen_qt.so';
  private launchParams: string = '';
  private pickerBusy: boolean = false;
  private workspaceReady: boolean = false;

  // QPA 调用本方法后写入 idName，并把存储传给固定的 pages/Index 承载页。
  newLocalStorage(): LocalStorage {
    return new LocalStorage();
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    try {
      nativeBridge.registerWorkspace((json: string) => {
        this.handleWorkspaceEvent(json).catch((error: Error) => {
          hilog.error(0xFF00, 'QtWorkspace', 'HUIWEN_QT_EVENT_FAILED: %{public}s', error.message);
        });
      });
      qpa.startQtApplication(this);
      hilog.info(0xFF00, 'QtWorkspace', 'HUIWEN_QT_START');
    } catch (error) {
      const message = (error as Error).message;
      this.context.getApplicationContext().eventHub.emit('huiwen.workspace.failed', message);
      this.context.terminateSelf().catch((failure: Error) => {
        hilog.error(0xFF00, 'QtWorkspace', '关闭失败的 Qt 窗口：%{public}s', failure.message);
      });
    }
  }

  private async handleWorkspaceEvent(json: string): Promise<void> {
    const event = JSON.parse(json) as WorkspaceEvent;
    if (event.action === 'ready') {
      this.workspaceReady = true;
      hilog.info(0xFF00, 'QtWorkspace', 'HUIWEN_QT_READY');
      this.context.getApplicationContext().eventHub.emit('huiwen.workspace.ready');
      return;
    }
    if (event.action === 'close') {
      await this.context.terminateSelf();
      return;
    }
    const reply: WorkspaceReply = { action: event.action, name: '', inputPath: '', outputPath: '', error: '' };
    if (event.action === 'pick' && this.pickerBusy) {
      return;
    }
    try {
      if (event.action === 'pick') {
        this.pickerBusy = true;
        const document = await pickDocumentToSandbox(this.context, DocumentKind.PDF, true);
        if (document !== null) {
          reply.name = document.name;
          reply.inputPath = document.inputPath;
          reply.outputPath = document.outputPath;
          hilog.info(0xFF00, 'QtWorkspace', 'HUIWEN_QT_PICK_OK');
        }
      } else if (event.action === 'open') {
        await this.openOutputLocation(event.path);
      }
    } catch (error) {
      reply.error = (error as Error).message;
      hilog.error(0xFF00, 'QtWorkspace', 'HUIWEN_QT_%{public}s_FAILED: %{public}s', event.action, reply.error);
    } finally {
      if (event.action === 'pick') {
        this.pickerBusy = false;
      }
      nativeBridge.workspaceReply(JSON.stringify(reply));
    }
  }

  private async openOutputLocation(path: string): Promise<void> {
    const outputRoot: string = `${this.context.getApplicationContext().filesDir}/outputs/`;
    if (!path.startsWith(outputRoot) || /[\\\u0000]/.test(path) ||
      /\/(\.|\.\.)\//.test(path) || !path.toLowerCase().endsWith('.md')) {
      throw new Error('输出路径不属于本应用 Markdown 沙箱');
    }
    const outputDir: string = path.slice(0, path.lastIndexOf('/'));
    // 文件管理器按文件 URI 定位所在目录，不再用保存选择器导出副本。
    const uri = fileUri.getUriFromPath(path);
    const options: OpenLinkOptions = {
      parameters: { fileUri: uri }
    };
    hilog.info(0xFF00, 'QtWorkspace', 'HUIWEN_QT_OPEN_REQUEST: directory=%{public}s, uri=%{public}s',
      outputDir, uri);
    try {
      await this.context.openLink('filemanager://openDirectory', options);
      // Promise 完成只表示拉起请求成功，目录定位效果仍需真机确认。
      hilog.info(0xFF00, 'QtWorkspace', 'HUIWEN_QT_OPEN_REQUEST_ACCEPTED');
    } catch (error) {
      const failure = error as BusinessError;
      hilog.error(0xFF00, 'QtWorkspace', 'HUIWEN_QT_OPEN_FAILED: code=%{public}d, message=%{public}s',
        failure.code, failure.message);
      throw new Error(`无法打开 Markdown 所在目录（${failure.code}）：${failure.message}`);
    }
  }

  onDestroy(): void {
    nativeBridge.clearWorkspace();
  }

  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    // 复用已有工作区时，继续由 QPA 管理窗口，并重发就绪通知结束新首页。
    qpa.handleOnNewWant(want, launchParam);
    if (this.workspaceReady) {
      this.context.getApplicationContext().eventHub.emit('huiwen.workspace.ready');
    }
  }
}
