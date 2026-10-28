// Qt 独立窗口入口；保留自己的 Context 处理文件选择，首页结束后仍可使用。
import { UIAbility, Want, AbilityConstant, OpenLinkOptions } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { window } from '@kit.ArkUI';
import { fileUri } from '@kit.CoreFileKit';
import hilog from '@ohos.hilog';
import qpa from 'libplugins_platforms_qopenharmony.so';
import nativeBridge, { HarmonySandboxDocument } from 'libhuiwen_native.so';
import { DocumentKind, DocumentMode, pickDocumentToSandbox } from '../utils/DocumentStore';
import { CefWorkspaceProxy } from '../utils/CefPlatform';

interface WorkspaceEvent {
  action: string;
  path: string;
}

interface WorkspaceReply extends HarmonySandboxDocument {
  action: string;
  error: string;
}

interface WorkspaceRestoreResult {
  taskPath: string;
  error: string;
}

export default class QtWorkspaceAbility extends UIAbility {
  // QPA 读取这两个字段，加载应用共享库并调用其中的 main()。
  private launchApplication: string = 'libhuiwen_qt.so';
  private launchParams: string = '';
  private pickerBusy: boolean = false;
  private workspaceReady: boolean = false;
  private pendingTask: string = '';
  private restoring: boolean = false;
  private destroyed: boolean = false;
  private cefProxy: CefWorkspaceProxy | undefined = undefined;

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    super.onCreate(want, launchParam);
    this.pendingTask = this.requestedTask(want);
  }

  private requestedTask(want: Want): string {
    const value = want.parameters?.huiwenTaskPath;
    return typeof value === 'string' ? value : '';
  }

  private async deliverPendingTask(): Promise<void> {
    if (this.restoring || this.destroyed) {
      return;
    }
    const taskPath: string = this.pendingTask;
    if (taskPath === '') {
      this.context.getApplicationContext().eventHub.emit('huiwen.workspace.ready');
      return;
    }
    this.restoring = true;
    try {
      // 只传任务标识，实际文件路径由 C++ 重新校验；读取阶段不更新时间。
      const document: HarmonySandboxDocument = await nativeBridge.harmonyLoadTask(
        this.context.getApplicationContext().filesDir, taskPath);
      if (this.destroyed || this.pendingTask !== taskPath) {
        return;
      }
      const reply: WorkspaceReply = {
        action: 'restore', name: document.name, kind: document.kind, mode: document.mode,
        taskPath: document.taskPath, inputPath: document.inputPath, outputPath: document.outputPath,
        openPath: document.openPath, lastUsedAt: document.lastUsedAt, error: ''
      };
      nativeBridge.workspaceReply(JSON.stringify(reply));
    } catch (error) {
      if (!this.destroyed && this.pendingTask === taskPath) {
        this.pendingTask = '';
        this.restoring = false;
        const message: string = (error as Error).message;
        hilog.error(0xFF00, 'QtWorkspace', 'HUIWEN_QT_RESTORE_FAILED: %{public}s', message);
        this.context.getApplicationContext().eventHub.emit('huiwen.workspace.failed', message);
      }
    }
  }

  // QPA 调用本方法后写入 idName，并把存储传给固定的 pages/Index 承载页。
  newLocalStorage(): LocalStorage {
    return new LocalStorage();
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    try {
      this.cefProxy = new CefWorkspaceProxy(this.context, windowStage);
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
      await this.deliverPendingTask();
      return;
    }
    if (event.action === 'restoreResult') {
      const result = JSON.parse(event.path) as WorkspaceRestoreResult;
      if (!this.restoring || result.taskPath !== this.pendingTask) {
        return;
      }
      this.pendingTask = '';
      this.restoring = false;
      if (result.error !== '') {
        hilog.error(0xFF00, 'QtWorkspace', 'HUIWEN_QT_RESTORE_FAILED: %{public}s', result.error);
        this.context.getApplicationContext().eventHub.emit('huiwen.workspace.failed', result.error);
      } else {
        hilog.info(0xFF00, 'QtWorkspace', 'HUIWEN_QT_RESTORE_OK');
        // Qt 已载入任务并更新时间，首页此时才结束自己的 Ability。
        this.context.getApplicationContext().eventHub.emit('huiwen.workspace.ready');
      }
      return;
    }
    if (event.action === 'close') {
      await this.context.terminateSelf();
      return;
    }
    const reply: WorkspaceReply = {
      action: event.action, name: '', kind: '', mode: '', taskPath: '', inputPath: '',
      outputPath: '', openPath: '', lastUsedAt: 0, error: ''
    };
    if (event.action === 'pick' && this.pickerBusy) {
      return;
    }
    try {
      if (event.action === 'pick') {
        this.pickerBusy = true;
        const document = await pickDocumentToSandbox(this.context, DocumentKind.PDF, DocumentMode.Convert);
        if (document !== null) {
          reply.name = document.name;
          reply.inputPath = document.inputPath;
          reply.outputPath = document.outputPath;
          reply.kind = document.kind;
          reply.mode = document.mode;
          reply.taskPath = document.taskPath;
          reply.openPath = document.openPath;
          reply.lastUsedAt = document.lastUsedAt;
          hilog.info(0xFF00, 'QtWorkspace', 'HUIWEN_QT_PICK_OK');
        }
      } else if (event.action === 'open') {
        await this.openOutputLocation(event.path);
      } else if (event.action === 'openDirectory') {
        // 顶部文件菜单只打开当前任务目录，路径仍由通用 C++ 沙箱规则校验。
        const directory: string = nativeBridge.harmonyValidateTaskDirectory(
          this.context.getApplicationContext().filesDir, event.path);
        await this.openFileManager(directory);
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
    const outputPath: string = nativeBridge.harmonyValidateOutput(
      this.context.getApplicationContext().filesDir, path);
    await this.openFileManager(outputPath);
  }

  // 文件卡片传入已验证文件，目录菜单传入已验证目录，共用系统文件管理器入口。
  private async openFileManager(path: string): Promise<void> {
    const uri: string = fileUri.getUriFromPath(path);
    const options: OpenLinkOptions = {
      parameters: { fileUri: uri }
    };
    hilog.info(0xFF00, 'QtWorkspace', 'HUIWEN_QT_OPEN_REQUEST: path=%{public}s, uri=%{public}s',
      path, uri);
    try {
      await this.context.openLink('filemanager://openDirectory', options);
      // Promise 完成只表示拉起请求成功，目录定位效果仍需真机确认。
      hilog.info(0xFF00, 'QtWorkspace', 'HUIWEN_QT_OPEN_REQUEST_ACCEPTED');
    } catch (error) {
      const failure = error as BusinessError;
      hilog.error(0xFF00, 'QtWorkspace', 'HUIWEN_QT_OPEN_FAILED: code=%{public}d, message=%{public}s',
        failure.code, failure.message);
      throw new Error(`无法打开文件目录（${failure.code}）：${failure.message}`);
    }
  }

  onDestroy(): void {
    this.destroyed = true;
    this.pendingTask = '';
    nativeBridge.clearWorkspace();
    this.cefProxy?.dispose();
    this.cefProxy = undefined;
  }

  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    // 复用已有窗口时也重新传递任务；仅在 Qt 确认恢复后通知首页关闭。
    qpa.handleOnNewWant(want, launchParam);
    if (this.restoring || this.pickerBusy) {
      this.context.getApplicationContext().eventHub.emit('huiwen.workspace.failed', '工作区正在处理文件，请稍后重试');
      return;
    }
    this.pendingTask = this.requestedTask(want);
    if (this.workspaceReady) {
      this.deliverPendingTask();
    }
  }
}
