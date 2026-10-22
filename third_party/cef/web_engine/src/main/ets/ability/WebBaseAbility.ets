// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import UIAbility from '@ohos.app.ability.UIAbility';
import LogUtil from '../utils/LogUtil';
import window from '@ohos.window';
import Want from '@ohos.app.ability.Want';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import JsBindingUtils from '../utils/JsBindingUtils';
import { NativeContext } from '../interface/CommonInterface';
import { CommandType, ConfigData, ContextType } from '../common/Constants';
import CheckEmptyUtils from '../utils/CheckEmptyUtils';
import { WebProxy } from '../interface/WebProxy';
import { common } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';

const TAG: string = 'WebBaseAbility';

export class WebBaseAbility extends UIAbility implements WebProxy {
  protected xcomponentId: string = '';
  protected startUri: string = '';
  protected windowStage: window.WindowStage | undefined = undefined;
  protected nativeContext: NativeContext =
    JsBindingUtils.getNativeContext(ContextType.kMainProcess);
  // cef: 隐藏原生标题栏以实现 frameless 窗口
  protected hideTitleBar: boolean = true;
  protected controlButtonsVisible: boolean = false;
  protected originWindowId: number = -1;
  protected useDarkMode: boolean = false;
  protected isStateless: boolean = false;

  private initParameters(want: Want) {
    if (want && want.parameters) {
      if (want.parameters['xcomponentId'] !== undefined) {
        this.xcomponentId = want.parameters['xcomponentId'] as string;
      }
      if (want.parameters["hideTitleBar"] !== undefined) {
        this.hideTitleBar = want.parameters['hideTitleBar'] as boolean;
      }
      if (want.parameters["controlButtonsVisible"] !== undefined) {
        this.controlButtonsVisible = want.parameters['controlButtonsVisible'] as boolean;
      }
      if (want.parameters["useDarkMode"] !== undefined) {
        this.useDarkMode = want.parameters['useDarkMode'] as boolean;
      }
      if (want.parameters["isStateless"] !== undefined) {
        this.isStateless = want.parameters['isStateless'] as boolean;
      }
    }

    if (want["uri"]) {
      let storePath = want['uri'] as string;
      // convert storage path to uri
      this.startUri = storePath.replace("file://docs/", "file:///");
    }
  }

  protected getContentPath(): string {
    return 'pages/IndexPopup';
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    this.initParameters(want);

    if (CheckEmptyUtils.isEmpty(this.xcomponentId)) {
      LogUtil.info(TAG, 'onCreate, Allocate xcomponent id');
      let widgetId = this.nativeContext.ExecuteCommand(CommandType.kGetWidget, {}).widget_Id;
      this.xcomponentId = ConfigData.WINDOW_PREFIX + widgetId;
    }
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    this.windowStage = windowStage;
    this.originWindowId = windowStage?.getMainWindowSync().getWindowProperties().id;
  }

  // implements WebProxy interface
  getWindowStage(): window.WindowStage | undefined {
    return this.windowStage;
  }

  getWindow(): window.Window | undefined {
    return this.windowStage?.getMainWindowSync();
  }

  getWindowContext(): common.Context {
    return this.context;
  }

  getWindowId(): string {
    return this.xcomponentId;
  }

  closeWindow() {
    this.context.terminateSelf()
      .then(() => {
        LogUtil.info(TAG, 'terminate Self id: ' + this.getWindowId());
      })
      .catch((err: BusinessError) => {
        LogUtil.error(TAG, 'Failed to terminate Self: ' + JSON.stringify(err));
      });
  }

  setWindowTitle(title:string) {
    this.context.setMissionLabel(title)
      .catch((err: BusinessError) => {
        LogUtil.error(TAG, 'Failed to set title: ' + JSON.stringify(err));
      });
  }

  createSubWindow(name: string): Promise<window.Window> {
    return new Promise<window.Window>((resolve, reject) => {
      if (!this.windowStage) {
        reject('Failed to get windowStage');
        return;
      }
      this.windowStage.createSubWindow(name)
        .then((data: window.Window) => {
          resolve(data);
        }).catch((error: BusinessError) => {
        reject(error);
      })
    });
  }

  getOriginWindowId(): number {
    return this.originWindowId;
  }

  getIsQtChild(): boolean {
    return false;
  }

  isUIAbility(): boolean {
    return true;
  }

  public updateAvoidAreaStatus(isFullScreen: boolean) {
    let windowClass = this.getWindow();

    // Set window to full screen.
    windowClass?.setWindowLayoutFullScreen(true).then(() => {
      LogUtil.info(TAG, 'Succeeded in setting the window layout to full-screen mode.');
    }).catch((err: BusinessError) => {
      LogUtil.error(TAG, 'Failed to set the window layout to full-screen mode. Cause:' + JSON.stringify(err));
    });

    // Hide the navigation bar here.
    windowClass?.setSpecificSystemBarEnabled('navigationIndicator', false).then(() => {
      LogUtil.info(TAG, 'Succeeded in setting the navigation indicator to be invisible.');
    }).catch((err: BusinessError) => {
      LogUtil.error(TAG, `Failed to set the navigation indicator to be invisible. Code is ${err.code}, message is ${err.message}`);
    });

    // The top status bar is set up to be hidden when in full-screen mode and displayed when maximized.
    windowClass?.setSpecificSystemBarEnabled('status', !isFullScreen).then(() => {
      LogUtil.info(TAG, 'Succeeded in setting the status bar to be invisible.');
    }).catch((err: BusinessError) => {
      LogUtil.error(TAG, `Failed to set the status bar to be invisible. Code is ${err.code}, message is ${err.message}`);
    });
  }
};
