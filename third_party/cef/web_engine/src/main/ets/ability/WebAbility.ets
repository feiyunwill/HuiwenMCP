// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import LogUtil from '../utils/LogUtil';
import window from '@ohos.window';
import Want from '@ohos.app.ability.Want';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import { NativeThemeAdapter } from '../adapter/NativeThemeAdapter';
import { AbilityManager } from '../common/AbilityManager';
import { Configuration } from '@ohos.app.ability.Configuration';
import Inject from '../common/InjectModule';
import { BrowserCloseResponse, WindowBound, WindowLimits } from '../interface/CommonInterface';
import CheckEmptyUtils from '../utils/CheckEmptyUtils';
import { DisplayAdapter } from  '../adapter/DisplayAdapter';
import { WebBaseAbility } from './WebBaseAbility';
import { CommandType, ConfigData } from '../common/Constants';
import { PermissionManagerAdapter } from '../adapter/PermissionManagerAdapter';
import { deviceInfo, settings } from '@kit.BasicServicesKit';
import { display } from '@kit.ArkUI';
import { ConfigurationConstant } from '@kit.AbilityKit';

const TAG: string = 'WebAbility';

// 2in1 设备上窗口装饰条隐藏后，系统仍在窗口右上角悬浮绘制最小化/最大化/关闭按钮
//（盖在应用内容之上，无法用 ArkUI 图层遮挡）。实测该按钮条约占窗口顶部 40vp
//（本机密度 1.9 时约 76 物理像素，按钮条底边距窗口顶 40vp）。
// 这里把该高度以物理像素通过 LocalStorage 的 statusBarHeight 传给页面：
// IndexPopup 会让内容区整体下移（等价于 Windows 的标题栏占位），
// CEF 页面自身的按钮（如 DevTools 工具栏的齿轮）就永远不会与系统按钮重叠。
const DECOR_STRIP_HEIGHT_VP: number = 40;

export class WebAbility extends WebBaseAbility {
  private static WINDOW_CLOSE: number = 1000;
  private static SLEEP_INTERVAL_MS = 10;
  private static MAXIMUM_NUM_OF_SLEEP_CYCLES = 250;
  private abilityManager: AbilityManager | undefined = undefined;
  private displayAdapter : DisplayAdapter | undefined = undefined;  
  private nativeThemeAdapter: NativeThemeAdapter | undefined = undefined;

  onConfigurationUpdate(config: Configuration) {
    this.nativeThemeAdapter?.setSystemNativeTheme(config.colorMode);
    this.displayAdapter?.onSystemFontSizeChange(config.fontSizeScale);
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    super.onCreate(want, launchParam);

    this.abilityManager = Inject.get(AbilityManager);
    this.nativeThemeAdapter = Inject.get(NativeThemeAdapter);
    this.displayAdapter = Inject.get(DisplayAdapter);
    Inject.get(PermissionManagerAdapter).initPermissions();
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    super.onWindowStageCreate(windowStage);

    if (this.checkSingleInstance()) {
      LogUtil.warn(TAG, 'repeat launch from desktop, need terminate');
      return;
    }

    this.abilityManager?.addProxy(this);

    if (deviceInfo.sdkApiVersion >= 14) {
      windowStage.setWindowRectAutoSave(true);
    }
    windowStage.getMainWindow((err, data) => {
      if (err.code) {
        LogUtil.error(TAG, 'Failed to obtain the main window: ' + JSON.stringify(err));
        return;
      }

      try {
        data.on('windowStatusChange', (status) => {
          this.nativeContext.OnWindowStatusChange(this.xcomponentId, status);
        })
      } catch (exception) {
        LogUtil.error(TAG,'Failed to listener for window status changes: ' + JSON.stringify(exception));
      }

      try {
        data.on('windowSizeChange', (size) => {
          const prop = data.getWindowProperties();
          let windowBound: WindowBound = {
            left: prop.windowRect.left + prop.drawableRect.left,
            top: this.hideTitleBar ? prop.windowRect.top + prop.drawableRect.top : prop.windowRect.top,
            width: size.width - prop.drawableRect.left - prop.drawableRect.left,
            height: this.hideTitleBar ? size.height - prop.drawableRect.left - prop.drawableRect.top : size.height - prop.drawableRect.top,
          };
          this.nativeContext.OnWindowSizeChange(this.xcomponentId, windowBound);
        });
      } catch (exception) {
        LogUtil.error(TAG,'Failed to listener for window size changes: ' + JSON.stringify(exception));
      }

      try {
        data.on("windowRectChange", (option) => {
          const prop = data.getWindowProperties();
          const rect = option.rect;
          let windowBound: window.Rect = {
            left: rect.left + prop.drawableRect.left,
            top: rect.top + prop.drawableRect.top,
            width: rect.width - prop.drawableRect.left - prop.drawableRect.left,
            height: rect.height - prop.drawableRect.left - prop.drawableRect.top,
          };
          this.nativeContext.OnWindowRectChange(this.xcomponentId,
            windowBound,
            option.reason.valueOf());
        });
      } catch (exception) {
        LogUtil.error(TAG,'Failed to listener for window rect changes: ' + JSON.stringify(exception));
      }

      try {
        data.on('windowEvent', (type) => {
          this.nativeContext.OnWindowEvent(this.xcomponentId, type.valueOf());
        });
      } catch (exception) {
        LogUtil.error(TAG, 'Failed to register callback: ' + JSON.stringify(exception));
      }

      try {
        data.on("windowVisibilityChange", (visible) => {
          this.nativeContext.OnWindowVisibilityChange(this.xcomponentId, visible);
        });
      } catch (exception) {
        LogUtil.error(TAG, 'Failed to register callback: ' + JSON.stringify(exception));
      }

      try {
        data.on('keyboardHeightChange', (height) => {
          this.nativeContext.OnKeyboardHeightChange(this.xcomponentId, height);
        });
      } catch (exception) {
        LogUtil.error(TAG, 'Failed to listener for keyboard height changes: ' + JSON.stringify(exception));
      }
    });

    const para: Record<string, Object> = {
      'xcomponentId': this.xcomponentId,
      'startUri': this.startUri,
      'window': windowStage.getMainWindowSync()
    };
    // 2in1：预留系统悬浮窗口控制按钮条高度（物理像素），页面顶部留出等高空白，
    // 内容不再延伸到按钮条下方。IndexPopup 按 px2vp 换算成 padding 顶部留白，
    // WebWindow 的拖放坐标换算（windowY - statusBarHeight）同样依赖该值。
    if (deviceInfo.deviceType === '2in1') {
      try {
        const density: number = display.getDefaultDisplaySync().densityPixels;
        para['statusBarHeight'] = Math.round(DECOR_STRIP_HEIGHT_VP * density);
      } catch (err) {
        LogUtil.error(TAG, 'get display density failed: ' + JSON.stringify(err));
      }
    }
    let storage: LocalStorage = new LocalStorage(para);
    windowStage.loadContent(this.getContentPath(), storage, (err, data) => {
      if (err.code) {
        LogUtil.error(TAG, 'Failed to load the content: ' + JSON.stringify(err));
        return;
      }

      let window: window.Window = windowStage.getMainWindowSync();
      // hide title bar
      window.setWindowDecorVisible(!this.hideTitleBar);

      if (deviceInfo.sdkApiVersion >= 14) {
        if (!this.controlButtonsVisible) {
          window.setWindowTitleButtonVisible(false, false, false);
        } else if (this.useDarkMode && deviceInfo.deviceType === '2in1') {
          let buttonStyle: window.DecorButtonStyle = {
            colorMode: ConfigurationConstant.ColorMode.COLOR_MODE_DARK
          };
          window.setDecorButtonStyle(buttonStyle);
         }
      }
      if (deviceInfo.sdkApiVersion >= 15) {
        this.nativeContext.RegisterWindowEventFilter(window.getWindowProperties().id);
      }

      if(deviceInfo.deviceType == 'tablet') {
        const context: Context = getContext(this);
        let res: boolean = settings.registerKeyObserver(context, 
          'window_pcmode_switch_status', settings.domainName.USER_PROPERTY, () => {
          this.updateWindowPcModeSwitchStatus(context);
        });
        LogUtil.info(TAG, 'register window_pcmode_switch_status,res:' + res + ',xcomponentId:' + this.xcomponentId);
        if (res) {
          this.updateWindowPcModeSwitchStatus(context);
        }
      }

      try {
        if (this.xcomponentId === ConfigData.DEFAULT_WINDOW_ID) {
          let windowLimits = this.getWindow()?.getWindowLimits();
          let uiContext = window.getUIContext();
          let limits: WindowLimits = {
            maxHeight: Math.ceil(uiContext.px2vp(windowLimits?.maxHeight ?? 0)),
            maxWidth: Math.ceil(uiContext.px2vp(windowLimits?.maxWidth ?? 0)),
            minHeight: Math.ceil(uiContext.px2vp(windowLimits?.minHeight ?? 0)),
            minWidth: Math.ceil(uiContext.px2vp(windowLimits?.minWidth ?? 0))
          }
          this.nativeContext.SetSystemWindowLimits(limits);
        }
      } catch (exception) {
        LogUtil.error(TAG, 'Failed to get window limits or get ui context:' + JSON.stringify(exception));
      }
    });
  }

  onWindowStageDestroy() {
    if (CheckEmptyUtils.isEmpty(this.xcomponentId)) {
      LogUtil.warn(TAG, 'onWindowStageDestroy component is not instantiated');
      return;
    }
    if (deviceInfo.sdkApiVersion >= 15) {
      let originWindowId = this.abilityManager?.getOriginWindowId(this.xcomponentId);
      this.nativeContext.ClearWindowEventFilter(originWindowId);
    }
    // Main window is destroyed, release UI related resources
    this.abilityManager?.removeProxy(this.xcomponentId);
  }

  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    let uriNotNull = want["uri"] !== undefined && want["uri"] !== '';
    if(want!== undefined && want !== null) {
      LogUtil.info(TAG, 'onNewWant begin, want: ' +
        'bundleName: ' + JSON.stringify(want.bundleName) +
        ', abilityName: ' + JSON.stringify(want.abilityName) +
        ', deviceId: ' + JSON.stringify(want.deviceId) +
        ', uri not null: ' + JSON.stringify(uriNotNull) +
        ', type: ' + JSON.stringify(want.type) +
        ', flags: ' + JSON.stringify(want.flags) +
        ', action: ' + JSON.stringify(want.action) +
        ', parameters: ' + JSON.stringify(want.parameters) +
        ', entities: ' + JSON.stringify(want.entities) +
        ', moduleName: ' + JSON.stringify(want.moduleName) +
        ', LaunchParam: ' + JSON.stringify(launchParam));
    }
    if (uriNotNull) {
      let uri = want['uri'] as string;
      this.openNewWindow(uri.replace("file://docs/", "file:///"));
    } else if (want.parameters && want.parameters['openInNewWindow'] === true) {
      this.openNewWindow(want['uri'] as string);
    }
  }

  onPrepareToTerminate(): boolean {
    this.nativeContext.OnWindowEvent(this.xcomponentId, WebAbility.WINDOW_CLOSE);
    return true;
  }

  async onPrepareToTerminateAsync(): Promise<boolean> {
    let id: number = Number(this.xcomponentId.substring(ConfigData.WINDOW_PREFIX.length));
    let response: BrowserCloseResponse = BrowserCloseResponse.kUndetermined;
    this.nativeContext.OnWindowEvent(this.xcomponentId, WebAbility.WINDOW_CLOSE);
    for (let i = 0; i < WebAbility.MAXIMUM_NUM_OF_SLEEP_CYCLES; i++) {
      await this.sleep(WebAbility.SLEEP_INTERVAL_MS);
      response = this.nativeContext.GetBrowserCloseResponse(id);
      if (response !== BrowserCloseResponse.kUndetermined) {
        break;
      }
    }
    if (response === BrowserCloseResponse.kClosingContinue || response === BrowserCloseResponse.kClosed) {
      return false;
    }
    return true;
  }

  sleep(delay: number) {
    return new Promise<void>((resolve) => setTimeout(resolve, delay));
  }

  async onDestroy(): Promise<void> {
    let id: number = Number(this.xcomponentId.substring(ConfigData.WINDOW_PREFIX.length));
    while (true) {
      await this.sleep(WebAbility.SLEEP_INTERVAL_MS);
      let response = this.nativeContext.GetBrowserCloseResponse(id);
      if (response === BrowserCloseResponse.kClosed || response === BrowserCloseResponse.kClosedAnyway) {
        break;
      }
    }
    if (this.abilityManager?.getProxyCount() === 0) {
      while (true) {
        await this.sleep(WebAbility.SLEEP_INTERVAL_MS);
        let destroyed = this.nativeContext.BrowserDestroyed();
        if (destroyed) {
          break;
        }
      }
    }
    return;
  }

  private checkSingleInstance(): boolean {
    if (!CheckEmptyUtils.isEmpty(this.xcomponentId)) {
      return false;
    }
    this.openNewWindow(this.startUri);
    this.closeWindow();
    return true;
  }

  private openNewWindow(open_url: string) {
    this.nativeContext.ExecuteCommand(CommandType.kNewWindow, { url: open_url });
  }

  private updateWindowPcModeSwitchStatus(context: Context) {
    try {
      let value = settings.getValueSync(context, 'window_pcmode_switch_status', 'false',
        settings.domainName.USER_PROPERTY);
      LogUtil.info(TAG, 'update window_pcmode_switch_status, ' +
        'new status:' + value + ',xcomponentId:' + this.xcomponentId);
      let newStatus: boolean = false;
      if (value === 'true') {
        newStatus = true;
      }
      this.nativeContext.UpdateWindowPcModeSwitchStatusCB(newStatus);
    } catch (err) {
      LogUtil.error(TAG, 'update window_pcmode_switch_status fail, err:' + JSON.stringify(err));
    }
  }
};
