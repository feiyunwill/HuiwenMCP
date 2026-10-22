// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import { injectable } from 'inversify';
import { ConfigData } from './Constants';
import { WebProxy } from '../interface/WebProxy';
import { common } from '@kit.AbilityKit';
import window from '@ohos.window';
import adapter_c from 'libadapter_c.so';

AppStorage.setOrCreate('abilityProxyList', []);

@injectable()
export class AbilityManager {
  private appStorage: WebProxy[];

  constructor() {
    let appStorageLink: SubscribedAbstractProperty<WebProxy[]>
      = AppStorage.link('abilityProxyList');
    this.appStorage = appStorageLink.get();
  }

  private paramNormalization(proxyId: string | number): string {
    if (typeof proxyId === 'number') {
      return ConfigData.WINDOW_PREFIX + proxyId;
    }
    return proxyId;
  }

  public addProxy(proxy: WebProxy) {
    for (let i = 0; i < this.appStorage.length; i++) {
      if (proxy.getWindowId() === this.appStorage[i].getWindowId()) {
        return;
      }
    }
    this.appStorage.push(proxy);
  }

  public removeProxy(proxyId: string | number) {
    let id = this.paramNormalization(proxyId);
    for (let i = 0; i < this.appStorage.length; i++) {
      if (id === this.appStorage[i].getWindowId()) {
        this.appStorage.splice(i, 1);
        break;
      }
    }
  }

  public getProxy(proxyId: string | number): WebProxy | undefined {
    let id = this.paramNormalization(proxyId);
    for (let i = 0; i < this.appStorage.length; i++) {
      if (id === this.appStorage[i].getWindowId()) {
        return this.appStorage[i];
      }
    }
    return undefined;
  }

  public isEmpty(): boolean {
    return this.appStorage.length === 0;
  }

  public getProxyCount(): number {
    return this.appStorage.length;
  }

  public getProxyList(): WebProxy[] {
    return this.appStorage;
  }
  
  public getWindowIdByOriginWindow(originWindowId: number): string {
    for (let i = 0; i < this.appStorage.length; i++) {
      if (originWindowId === this.appStorage[i].getOriginWindowId()) {
        return this.appStorage[i].getWindowId();
      }
    }
    return "";
  }

  public getOriginWindowId(windowId: string): number {
    for (let i = 0; i < this.appStorage.length; i++) {
      if (windowId === this.appStorage[i].getWindowId()) {
        return this.appStorage[i].getOriginWindowId();
      }
    }
    return -1;
  }

  public getWindow(id: string | number): window.Window | undefined {
    const xcomponentId: string = this.paramNormalization(id);
    const windowName: string = adapter_c.getWindowNameByXComponentId(xcomponentId);
    if (windowName === '') {
      return undefined;
    }

    const currentWindow = window.findWindow(windowName);
    if (!currentWindow) {
      return undefined;
    }
    return currentWindow;
  }

  public getWindowStage(id: string | number): window.WindowStage | undefined {
    const currentWindow = this.getWindow(id);
    const currentUiContext = currentWindow?.getUIContext()?.getHostContext();
    if (!currentUiContext) {
      return undefined;
    }

    const context = currentUiContext as common.UIAbilityContext;
    if (!context?.windowStage) {
      return undefined;
    }
    return context.windowStage;
  }

};
