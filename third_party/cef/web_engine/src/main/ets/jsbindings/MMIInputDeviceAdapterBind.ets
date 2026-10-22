// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import Inject from '../common/InjectModule';
import JsBindingUtils from '../utils/JsBindingUtils';
import type { MMIInputDeviceData, DeviceListenerChaned } from '../interface/CommonInterface';
import lazy { MMIInputDeviceAdapter } from '../adapter/MMIInputDeviceAdapter';

function implMMIInputDeviceAdapter(): MMIInputDeviceAdapter {
  return Inject.getOrCreate(MMIInputDeviceAdapter);
}

function getDeviceList(callback: (deviceList: Array<number>) => void) {
  implMMIInputDeviceAdapter().getDeviceList(callback);
}

function getDeviceInfo(deviceId: number, callback: (inputDeviceData: MMIInputDeviceData) => void) {
  implMMIInputDeviceAdapter().getDeviceInfo(deviceId, callback);
}

function onDeviceListenerChanged(callback: (changed: DeviceListenerChaned) => void) {
  implMMIInputDeviceAdapter().onDeviceListenerChanged(callback);
}

function offDeviceListenerChanged() {
  implMMIInputDeviceAdapter().offDeviceListenerChanged();
}

export class MMIInputDeviceAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction('MMIInputDeviceAdapter.GetDeviceList', getDeviceList);
    JsBindingUtils.bindFunction('MMIInputDeviceAdapter.GetDeviceInfo', getDeviceInfo);
    JsBindingUtils.bindFunction('MMIInputDeviceAdapter.OnDeviceListenerChanged', onDeviceListenerChanged);
    JsBindingUtils.bindFunction('MMIInputDeviceAdapter.OffDeviceListenerChanged', offDeviceListenerChanged);
  }
}