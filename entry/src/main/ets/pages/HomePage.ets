// ArkTS 首页独立于 QPA 固定加载的 Index 承载页，负责功能入口和窗口交接。
import { common } from '@kit.AbilityKit';
import { FunctionSidebar } from '../components/FunctionSidebar';
import { HomeFeatureCard } from '../components/HomeFeatureCard';
import { HOME_FORMAT_DIALOG_WIDTH, HomeFeature, HomeFormatDialog } from '../components/HomeFormatDialog';
import { getNativeStatus } from '../services/ConversionService';

@Entry
@Component
struct HomePage {
  @State pingResult: string = '待检测';
  @State activeFeature: HomeFeature = HomeFeature.None;
  @State workspaceError: string = '';
  @State workspaceLaunching: boolean = false;
  private formatDialogId: number = -1;
  private formatDialogOpening: boolean = false;
  private homeVisible: boolean = false;
  private launchTimeout: number = -1;
  private workspaceReady = (): void => {
    // 只有发起跳转的首页消费一次就绪通知，避免其他页面结束自己的 Ability。
    if (!this.workspaceLaunching) {
      return;
    }
    this.workspaceLaunching = false;
    clearTimeout(this.launchTimeout);
    const context = this.getUIContext().getHostContext() as common.UIAbilityContext;
    // Qt 窗口确认显示后，只结束原首页 Ability，不结束应用进程。
    context.terminateSelf().catch((error: Error) => {
      this.workspaceError = `关闭首页失败：${error.message}`;
      this.workspaceLaunching = false;
    });
  };
  private workspaceFailed = (message: string): void => {
    if (!this.workspaceLaunching) {
      return;
    }
    clearTimeout(this.launchTimeout);
    this.workspaceLaunching = false;
    this.workspaceError = `Qt 工作区启动失败：${message}`;
  };

  aboutToAppear(): void {
    this.homeVisible = true;
    // 首页打开时检查 ArkTS 是否能调用 C++ NAPI 模块。
    this.pingResult = getNativeStatus();
    const context = this.getUIContext().getHostContext() as common.UIAbilityContext;
    context.getApplicationContext().eventHub.on('huiwen.workspace.ready', this.workspaceReady);
    context.getApplicationContext().eventHub.on('huiwen.workspace.failed', this.workspaceFailed);
  }

  aboutToDisappear(): void {
    this.homeVisible = false;
    this.closeFormatDialog();
    clearTimeout(this.launchTimeout);
    const context = this.getUIContext().getHostContext() as common.UIAbilityContext;
    context.getApplicationContext().eventHub.off('huiwen.workspace.ready', this.workspaceReady);
    context.getApplicationContext().eventHub.off('huiwen.workspace.failed', this.workspaceFailed);
  }

  private async openWorkspace(): Promise<void> {
    if (this.workspaceLaunching) {
      return;
    }
    // 先收起格式子窗口，避免它残留在即将显示的 Qt 工作区上方。
    if (!this.closeFormatDialog()) {
      return;
    }
    this.workspaceLaunching = true;
    this.workspaceError = '';
    this.launchTimeout = setTimeout(() => {
      this.workspaceFailed('未收到窗口显示通知，请检查 HUIWEN_QT 日志');
    }, 15000);
    try {
      const context = this.getUIContext().getHostContext() as common.UIAbilityContext;
      await context.startAbility({ bundleName: 'cn.com.HuiWenMCP', abilityName: 'QtWorkspaceAbility' });
    } catch (error) {
      this.workspaceFailed((error as Error).message);
    }
  }

  private closeFormatDialog(): boolean {
    if (this.formatDialogId < 0) {
      return true;
    }
    try {
      this.getUIContext().getPromptAction().closeCustomDialog(this.formatDialogId);
      this.formatDialogId = -1;
      return true;
    } catch (error) {
      this.workspaceError = `关闭格式选择窗口失败：${(error as Error).message}`;
      return false;
    }
  }

  @Builder
  formatDialogContent() {
    HomeFormatDialog({
      feature: this.activeFeature,
      title: this.activeFeature === HomeFeature.Preview ? '请选择要预览的格式' :
        this.activeFeature === HomeFeature.Edit ? '请选择要编辑的格式' : '请选择要转化的格式',
      onCloseRequested: () => {
        this.closeFormatDialog();
      },
      onConvertRequested: () => {
        this.openWorkspace();
      }
    })
  }

  private async selectFeature(feature: HomeFeature): Promise<void> {
    if (this.workspaceLaunching || this.formatDialogOpening || this.formatDialogId >= 0) {
      return;
    }
    this.activeFeature = feature;
    this.workspaceError = '';
    this.formatDialogOpening = true;
    try {
      const promptAction = this.getUIContext().getPromptAction();
      // 按自定义弹窗模板保存系统返回的 ID，由首页统一管理打开和关闭。
      const dialogId = await promptAction.openCustomDialog({
        builder: () => {
          this.formatDialogContent();
        },
        alignment: DialogAlignment.Center,
        showInSubWindow: true,
        // PromptAction 的独立子窗口不能同时启用模态。
        isModal: false,
        autoCancel: true,
        // 背板与内容共用固定宽度，只由背板绘制背景、圆角及阴影。
        width: HOME_FORMAT_DIALOG_WIDTH,
        backgroundColor: '#F7F8FA',
        backgroundBlurStyle: BlurStyle.NONE,
        cornerRadius: 16,
        shadow: { radius: 16, color: '#22000000', offsetX: 0, offsetY: 6 },
        onWillDismiss: (action: DismissDialogAction) => {
          this.formatDialogId = -1;
          action.dismiss();
        }
      });
      // 打开是异步的；首页若已退出，及时关闭刚创建的子窗口。
      if (!this.homeVisible) {
        promptAction.closeCustomDialog(dialogId);
        return;
      }
      this.formatDialogId = dialogId;
    } catch (error) {
      this.workspaceError = `打开格式选择窗口失败：${(error as Error).message}`;
    } finally {
      this.formatDialogOpening = false;
    }
  }

  build() {
    // 根布局透明，只有左侧窄栏透出窗口后方；主内容自己绘制不透明背景。
    Row() {
      FunctionSidebar()
      this.homeContent()
    }
    .width('100%')
    .height('100%')
    .backgroundColor(Color.Transparent)
  }

  @Builder
  homeContent() {
    Row({ space: 24 }) {
      // 除窄侧栏及间距外，介绍区与主功能区按 3:7 分配剩余宽度。
      Column({ space: 20 }) {
        Text('汇文MCP')
          .fontSize(48)
          .minFontSize(20)
          .maxFontSize(48)
          .maxLines(1)
          .fontWeight(FontWeight.Bold)
          .fontColor('#183153')
        Text('PDF 与 Markdown 的阅读和转换工具')
          .fontSize(20)
          .fontColor('#4B6080')
        Text('汇集千文 · 连通万物')
          .fontSize(16)
          .fontColor('#667B98')
      }
      .layoutWeight(3)
      .height('100%')
      .padding(32)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Start)
      .backgroundColor('#EAF2FF')
      .borderRadius(24)

      Column({ space: 24 }) {
        Text('开始使用')
          .fontSize(28)
          .fontWeight(FontWeight.Bold)
          .fontColor('#183153')
        Text('选择需要的功能')
          .fontSize(16)
          .fontColor('#667B98')

        Row({ space: 12 }) {
          HomeFeatureCard({
            title: '文档预览',
            icon: $r('app.media.home_preview'),
            selected: this.activeFeature === HomeFeature.Preview,
            available: !this.workspaceLaunching,
            onSelect: () => this.selectFeature(HomeFeature.Preview)
          })
          .layoutWeight(1)
          HomeFeatureCard({
            title: '文档编辑',
            icon: $r('app.media.home_edit'),
            selected: this.activeFeature === HomeFeature.Edit,
            available: !this.workspaceLaunching,
            onSelect: () => this.selectFeature(HomeFeature.Edit)
          })
          .layoutWeight(1)
          HomeFeatureCard({
            title: '文档转化',
            icon: $r('app.media.home_convert'),
            selected: this.activeFeature === HomeFeature.Convert,
            available: !this.workspaceLaunching,
            onSelect: () => this.selectFeature(HomeFeature.Convert)
          })
          .layoutWeight(1)
        }
        .width('100%')

        if (this.workspaceLaunching) {
          Text('正在打开转换工作区…')
            .fontSize(14)
            .fontColor('#667B98')
        }
        if (this.workspaceError !== '') {
          Text(this.workspaceError)
            .fontSize(13)
            .fontColor('#B45309')
        }
        if (this.pingResult !== 'NAPI OK') {
          Text('原生模块尚未就绪')
            .fontSize(13)
            .fontColor('#B45309')
        }
      }
      .layoutWeight(7)
      .height('100%')
      .padding(24)
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Start)
      .backgroundColor('#FFFFFF')
      .borderRadius(24)
    }
    .layoutWeight(1)
    .height('100%')
    .padding(24)
    .backgroundColor('#F5F7FB')
  }
}
