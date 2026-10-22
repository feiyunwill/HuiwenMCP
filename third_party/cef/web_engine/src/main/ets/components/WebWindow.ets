// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import { NativeContext, WindowBound } from '../interface/CommonInterface';
import lazy { DragDropAdapter } from '../adapter/DragDropAdapter';
import LogUtil from '../utils/LogUtil';
import { ConfigData, ContextType, PanAction } from '../common/Constants';
import Inject from '../common/InjectModule';
import JsBindingUtils from '../utils/JsBindingUtils';
import { MultiInputAdapter } from '../adapter/MultiInputAdapter';
import { DragParamManager } from '../common/DragParamManager';
import { window } from '@kit.ArkUI';
import { PromptDialog } from '../components/CustomDialog';
import { NativeThemeAdapter } from '../adapter/NativeThemeAdapter'

type DialogCallbackType = (success: boolean, userInput: string) => void;

const DIALOG_CORNER_RADIUS: number = 10;
const DIALOG_GRID_COUNT: number = 4;
const DIALOG_OFFSET_X: number = 0;
const DIALOG_OFFSET_Y: number = -20;

// 本组件承载的是 CEF 自建窗口（右键菜单“审查元素”打开的 DevTools 窗口）：系统装饰条上的
// 按钮浮在网页内容之上，标题栏本身拖不动。这里把内容区顶部一段作为拖动带，在带内按下并
// 移动超过阈值就调用 Window.startMoving()，把拖动交给系统窗口管理器，效果与 Qt 窗口
// FramelessWidget::startSystemMove 一致。
const DRAG_BAND_VP: number = 32;
const DRAG_THRESHOLD_VP: number = 4;

@Component
export struct WebWindow {
  private static TAG: string = 'WebWindow';
  @LocalStorageLink('xcomponentId') xComponentId: string = '';
  @LocalStorageLink('startUri') startUri: string = '';
  @LocalStorageLink('window') window: window.Window | undefined = undefined;
  @LocalStorageLink('useDarkMode') useDarkMode: boolean = false;
  @LocalStorageLink('statusBarHeight') statusBarHeight: number = 0;
  private LIBRARY_NAME: string = 'adapter';
  private XCOMPONENT_NAME: string = 'xcomponent';
  private MODULE_NAME: string = this.xComponentId + ':' + this.XCOMPONENT_NAME;
  private dialogController: CustomDialogController | undefined = undefined;
  private dragDropAdapter: DragDropAdapter = Inject.get(DragDropAdapter);
  private nativeContext: NativeContext = JsBindingUtils.getNativeContext(ContextType.kMainProcess);
  private multiInputAdapter: MultiInputAdapter = Inject.get(MultiInputAdapter);
  private dragParamManager = Inject.get(DragParamManager);
  private dragInBand: boolean = false;
  private dragging: boolean = false;
  private dragStartX: number = 0;
  private dragStartY: number = 0;
  @State private bgColor: Color | string =
    this.useDarkMode ? NativeThemeAdapter.DARK_BACKGROUND_COLOR : NativeThemeAdapter.LIGHT_BACKGROUND_COLOR;

  aboutToAppear(): void {
    this.dragParamManager.addDragContextData({
      id: this.xComponentId,
      uiContext: this.getUIContext(),
    });
  }

  aboutToDisappear(): void {
    this.dragParamManager.removeDragContextData(this.xComponentId);
  }

  // 是否落在顶部拖动带内（localY 与 DRAG_BAND_VP 同为 vp）
  private isInDragBand(localY: number): boolean {
    return localY >= 0 && localY <= DRAG_BAND_VP;
  }

  // 把拖动交给系统窗口管理器
  private moveByWindowDrag(): void {
    LogUtil.info(WebWindow.TAG, 'moveByWindowDrag xComponentId:' + this.xComponentId);
    try {
      this.window?.startMoving();
    } catch (error) {
      LogUtil.error(WebWindow.TAG, 'startMoving failed: ' + JSON.stringify(error));
    }
  }

  // 鼠标（2in1 主要输入方式）在拖动带内按下并移动超过阈值 → 拖动窗口
  private onWindowMouseEvent(event: MouseEvent): void {
    if (event.action === MouseAction.Press) {
      this.dragging = false;
      this.dragInBand = this.isInDragBand(event.y);
      this.dragStartX = event.x;
      this.dragStartY = event.y;
      return;
    }
    if (event.action === MouseAction.Move) {
      if (!this.dragInBand || this.dragging) {
        return;
      }
      if (Math.abs(event.x - this.dragStartX) > DRAG_THRESHOLD_VP ||
        Math.abs(event.y - this.dragStartY) > DRAG_THRESHOLD_VP) {
        this.dragging = true;
        this.moveByWindowDrag();
      }
      return;
    }
    if (event.action === MouseAction.Release) {
      this.dragInBand = false;
      this.dragging = false;
    }
  }

  // This function is called by DialogAdapter
  CreatePromptDialog() {
    const that = this;
    return (title: string, message: string, promptMessage: string, callback: DialogCallbackType) => {
      that.dialogController = new CustomDialogController({
        builder: PromptDialog({
          title: title,
          subTitle: message,
          textValue: promptMessage,
          callback: callback
        }),
        cancel: () => {},
        autoCancel: false,
        alignment: DialogAlignment.Center,
        offset: { dx: DIALOG_OFFSET_X, dy: DIALOG_OFFSET_Y },
        gridCount: DIALOG_GRID_COUNT,
        customStyle: false,
        backgroundColor: $r('app.color.dialog_background'),
        cornerRadius: DIALOG_CORNER_RADIUS,
      });
      that.dialogController && that.dialogController.open();
    }
  }

  build() {
    XComponent({
      id: this.MODULE_NAME,
      type: XComponentType.SURFACE,
      libraryname: this.LIBRARY_NAME
    })
      .focusable(true)
      .defaultFocus(true)
      .backgroundColor(this.bgColor)
      .onMouse((event: MouseEvent) => {
        this.onWindowMouseEvent(event);
      })
      .gesture(
        PanGesture({ direction: PanDirection.Vertical | PanDirection.Horizontal, distance: 1 })
          .onActionStart((event: GestureEvent) => {
            // 拖动带内起手的滑动属于窗口拖动，不再转发给页面
            let localY = event.fingerList && event.fingerList.length > 0 ? event.fingerList[0].localY : -1;
            if (this.isInDragBand(localY)) {
              this.dragging = true;
              this.moveByWindowDrag();
              return;
            }
            this.multiInputAdapter.OnPanEvent(PanAction.kStart, this.xComponentId, event);
          })
          .onActionUpdate((event: GestureEvent) => {
            if (this.dragging) {
              return;
            }
            this.multiInputAdapter.OnPanEvent(PanAction.kUpdate, this.xComponentId, event);
          })
          .onActionEnd((event: GestureEvent) => {
            if (this.dragging) {
              this.dragging = false;
              return;
            }
            this.multiInputAdapter.OnPanEvent(PanAction.kEnd, this.xComponentId, event);
          })
          .onActionCancel(() => {
            this.dragging = false;
            this.multiInputAdapter.OnPanEvent(PanAction.kCancel, this.xComponentId, undefined);
          })
      )
      .gesture(
        PinchGesture()
          .onActionStart((event: GestureEvent) => {
            this.multiInputAdapter.OnPinchEvent("start", this.xComponentId, event);
          })
          .onActionUpdate((event: GestureEvent) => {
            this.multiInputAdapter.OnPinchEvent("update", this.xComponentId, event);
          })
          .onActionEnd((event: GestureEvent) => {
            this.multiInputAdapter.OnPinchEvent("end", this.xComponentId, event);
          })
      )
      .gesture(
        GestureGroup(GestureMode.Exclusive,
          TapGesture({ count: 2})
            .onAction((event: GestureEvent) => {
              if (event) {
                let offsetX = vp2px(event.fingerList[0].globalX);
                let offsetY = vp2px(event.fingerList[0].globalY);
                this.nativeContext.OnDoubleTapCB(this.xComponentId, offsetX, offsetY);
              }
            })
        )
      )
      .onDragEnter((event?: DragEvent) => {
        LogUtil.info(WebWindow.TAG, 'OhosDrag WebWindow onDragEnter xComponentId:' + this.xComponentId);
        this.dragDropAdapter.dragEnterData(this.xComponentId, (event as DragEvent).getSummary());
      })
      .onDragMove((event?: DragEvent) => {
        let windowX = event?.getWindowX();
        let windowY = event?.getWindowY();
        windowX = vp2px(windowX);
        windowY = vp2px(windowY);
        this.dragDropAdapter.dragMove(this.xComponentId,
                                      windowX,
                                      windowY - this.statusBarHeight);
      })
      .onDragLeave(() => {
        LogUtil.info(WebWindow.TAG, 'OhosDrag WebWindow onDragLeave xComponentId:' + this.xComponentId);
        this.dragDropAdapter.dragLeave(this.xComponentId);
      })
      .onDrop((event?: DragEvent) => {
        if (!event) {
          LogUtil.error(WebWindow.TAG, 'OhosDrag WebWindow DragDrop onDrop event is null');
          return;
        }
        LogUtil.info(WebWindow.TAG, 'OhosDrag WebWindow onDrop xComponentId:' + this.xComponentId);
        let dragData: UnifiedData = (event as DragEvent).getData() as UnifiedData;
        this.dragDropAdapter.dropData(this.xComponentId, dragData);
      })
      .onLoad(() => {
        LogUtil.info(WebWindow.TAG, 'onLoad xComponentId:' + this.xComponentId);
        // this.setDefaultBounds();
        // let vec_args = this.buildArgs();
        // this.nativeContext.runBrowser(vec_args);

        setTimeout(() => {
          this.bgColor = Color.White;
        }, 5000);
      })
      .id(this.XCOMPONENT_NAME)
      .renderFit(RenderFit.TOP_LEFT)
  }

  private setDefaultBounds() {
    if (this.xComponentId !== ConfigData.DEFAULT_WINDOW_ID) {
      return;
    }

    let prop = this.window?.getWindowProperties();
    if (prop) {
      let windowRect: WindowBound = {
        left: prop.windowRect.left,
        top: prop.windowRect.top,
        width: prop.windowRect.width,
        height: prop.windowRect.height
      }

      let drawableRect: WindowBound = {
        left: prop.drawableRect.left,
        top: prop.drawableRect.top + this.statusBarHeight,
        width: prop.drawableRect.width,
        height: prop.drawableRect.height - this.statusBarHeight
      }

      this.nativeContext.OnWindowInitSize(windowRect, drawableRect);
    }

    let state = this.window?.getWindowStatus();
    if (state) {
      this.nativeContext.OnWindowInitState(state);
    }
  }

  private buildArgs(): string[] {
    let default_uri = '--url=https://ams.haitaibrowser.com:8000/ohcef/ohindex.html';
    if (this.startUri.length !== 0) {
      default_uri = this.startUri;
    }
    let resDir = '--bundle-installation-dir=' + getContext().resourceDir;
    let vec_args = [resDir, default_uri];
    return vec_args;
  }
}
