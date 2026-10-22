// 只初始化 CEF 平台服务，不使用来源工程的无边框 Ability 和权限申请。
// 必须先初始化 Reflect 元数据，再加载包含 @injectable() 的 CEF 模块。
import 'reflect-metadata';
import { common } from '@kit.AbilityKit';
import { window, display } from '@kit.ArkUI';
import adapter from 'libadapter_c.so';
import JsBindingUtils from 'web_engine/src/main/ets/utils/JsBindingUtils';
import { ConfigData, ContextType } from 'web_engine/src/main/ets/common/Constants';
import { CommonDependencyProvider } from 'web_engine/src/main/ets/common/CommonDependencyProvider';
import { GlobalThisHelper } from 'web_engine/src/main/ets/utils/GlobalThisHelper';
import { JsBindingMethod } from 'web_engine/src/main/ets/jsbindings/JsBindingMethod';
import Inject from 'web_engine/src/main/ets/common/InjectModule';
import { AbilityManager } from 'web_engine/src/main/ets/common/AbilityManager';
import { WebProxy } from 'web_engine/src/main/ets/interface/WebProxy';
import { WindowBound } from 'web_engine/src/main/ets/interface/CommonInterface';

export function initializeCefPlatform(context: common.Context): void {
  let density: number = 1;
  try {
    density = display.getDefaultDisplaySync().scaledDensity;
  } catch (error) {
    console.warn('CEF display density unavailable; using 1');
  }
  adapter.initFunc(density);
  JsBindingUtils.initNativeContext(ContextType.kMainProcess);
  if (!GlobalThisHelper.isLaunched()) {
    GlobalThisHelper.appInit(new CommonDependencyProvider(context));
  }
  JsBindingMethod.bind();
}

export class CefWorkspaceProxy implements WebProxy {
  private context: common.UIAbilityContext;
  private stage: window.WindowStage;
  private id: string;

  constructor(context: common.UIAbilityContext, stage: window.WindowStage) {
    this.context = context;
    this.stage = stage;
    // 单例工作区使用适配层约定的 Qt 宿主 ID，供平台服务反查窗口。
    this.id = ConfigData.DEFAULT_QT_WINDOW;
    Inject.get(AbilityManager).addProxy(this);
    const properties = stage.getMainWindowSync().getWindowProperties();
    const windowRect: WindowBound = {
      left: properties.windowRect.left, top: properties.windowRect.top,
      width: properties.windowRect.width, height: properties.windowRect.height
    };
    const drawableRect: WindowBound = {
      left: properties.drawableRect.left, top: properties.drawableRect.top,
      width: properties.drawableRect.width, height: properties.drawableRect.height
    };
    JsBindingUtils.getNativeContext(ContextType.kMainProcess)
      .OnWindowInitSize(windowRect, drawableRect);
  }

  dispose(): void {
    Inject.get(AbilityManager).removeProxy(this.id);
  }

  getWindowStage(): window.WindowStage { return this.stage; }
  getWindow(): window.Window { return this.stage.getMainWindowSync(); }
  getWindowContext(): common.Context { return this.context; }
  getWindowId(): string { return this.id; }
  getOriginWindowId(): number { return this.getWindow().getWindowProperties().id; }
  getIsQtChild(): boolean { return true; }
  isUIAbility(): boolean { return true; }
  updateAvoidAreaStatus(isFullScreen: boolean): void {}

  closeWindow(): void {
    this.context.terminateSelf().catch((error: Error) => console.error(error.message));
  }

  setWindowTitle(title: string): void {
    this.context.setMissionLabel(title).catch((error: Error) => console.error(error.message));
  }

  createSubWindow(name: string): Promise<window.Window> {
    return this.stage.createSubWindow(name);
  }
}
