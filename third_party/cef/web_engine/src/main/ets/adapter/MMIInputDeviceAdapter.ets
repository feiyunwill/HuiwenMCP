// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import { BaseAdapter } from '../common/BaseAdapter';
import { injectable } from 'inversify';
import { inputDevice } from '@kit.InputKit';
import LogMethod from '../common/LogDecorator.ts';
import LogUtil from '../utils/LogUtil';
import { MMIInputDeviceData, DeviceListenerChaned } from '../interface/CommonInterface';

const TAG: string = 'MMIInputDeviceAdapter';

@injectable()
export class MMIInputDeviceAdapter extends BaseAdapter {
  private TransInputDeviceData(deviceData: inputDevice.InputDeviceData): MMIInputDeviceData | null {
    if (!deviceData) {
      LogUtil.error(TAG, 'Device data is null or undefined.');
      return null;
    }

    try {
      return {
        id: deviceData.id ?? -1,
        name: deviceData.name ?? "",
        sources: deviceData.sources ?? [],
        sources_type: (deviceData?.sources ?? []).filter(Boolean).join(","),
        bus: deviceData.bus ?? 0,
        product: deviceData.product ?? 0,
        vendor: deviceData.vendor ?? 0,
        version: deviceData.version ?? 0,
        phys: deviceData.phys ?? "",
        uniq: deviceData.uniq ?? ""
      };
    } catch (error) {
      LogUtil.error(TAG, `Error transforming device data error: ${error}`);
      return null;
    }
  }

  private TransDeviceListenerChaned(data: inputDevice.DeviceListener): DeviceListenerChaned | null {
    if (!data) {
      LogUtil.error(TAG, 'Device changed data is null or undefined.');
      return null;
    }

    try {
      return {
        id: data.deviceId ?? -1,
        changed_type: data.type?.toString() ?? "",
      };
    } catch (error) {
      LogUtil.error(TAG, `Error transforming device changed error: ${error}`);
      return null;
    }
  }

  @LogMethod
  getDeviceList(callback: (deviceList: Array<number>) => void) {
    try {
      inputDevice.getDeviceList().then((listIds) => {
        callback(listIds ?? []);
      });
    } catch (error) {
      LogUtil.error(TAG, `Failed to getDeviceList, error: ${error}`);
      callback([]);
    }
  }

  @LogMethod
  getDeviceInfo(deviceId: number, callback: (inputDeviceData: MMIInputDeviceData) => void) {
    if (!callback) {
      LogUtil.error(TAG, 'getDeviceInfo callback is null or undefined.');
      return;
    }
    const errorData: MMIInputDeviceData = {
      id: -1,
      name: "",
      sources: [],
      sources_type: "",
      bus: 0,
      product: 0,
      vendor: 0,
      version: 0,
      phys: "",
      uniq: ""
    }
    try {
      inputDevice.getDeviceInfo(deviceId).then((deviceData: inputDevice.InputDeviceData) => {
        const transformedData = this.TransInputDeviceData(deviceData);
        if (transformedData) {
          callback(transformedData);
        } else {
          LogUtil.error(TAG, 'Transformed getDeviceInfo data is null.');
          callback(errorData);
        }
      });
    } catch (error) {
      LogUtil.error(TAG, `Failed to getDeviceInfo, error: ${error}`);
      callback(errorData);
    }
  }

  @LogMethod
  onDeviceListenerChanged(callback: (changed: DeviceListenerChaned) => void) {
    const errorData: DeviceListenerChaned = {
      id: -1,
      changed_type: "",
    }
    try {
      inputDevice.on("change", (data: inputDevice.DeviceListener) => {
        const transformedData = this.TransDeviceListenerChaned(data);
        if (transformedData) {
          callback(transformedData);
        } else {
          LogUtil.error(TAG, 'Transformed inputDevice change data is null');
          callback(errorData);
        }
      });
    } catch (error) {
      console.error(`Get inputDevice device change failed, error: ${JSON.stringify(error)}`);
      callback(errorData);
    }
  }

  @LogMethod
  offDeviceListenerChanged() {
    inputDevice.off('change');
  }
}