// 最近工作区只负责展示；图标、格式、时间固定宽度，文件名单独占满剩余空间。
import { RecentWorkspace } from '../services/RecentWorkspaceService';

@Component
export struct RecentWorkspaceList {
  @Prop workspaces: RecentWorkspace[] = [];
  @Prop loading: boolean = false;
  @Prop available: boolean = true;
  @Prop error: string = '';
  @Prop maxListHeight: number = 240;
  @State hoveredTask: string = '';
  private readonly rowHeight: number = 48;
  private readonly rowSpacing: number = 8;
  onOpen: (taskPath: string) => void = () => {};

  private modeIcon(mode: string): Resource {
    if (mode === 'preview') {
      return $r('app.media.home_preview');
    }
    return mode === 'edit' ? $r('app.media.home_edit') : $r('app.media.home_convert');
  }

  @Builder
  workspaceRow(workspace: RecentWorkspace) {
    Button({ type: ButtonType.Normal }) {
      Row({ space: 12 }) {
        Stack() {
          Image(this.modeIcon(workspace.mode))
            .width(18)
            .height(18)
            .objectFit(ImageFit.Contain)
            .draggable(false)
        }
        .width(28)
        .height(28)
        .flexShrink(0)
        .backgroundColor($r('app.color.home_card'))
        .borderRadius(8)

        Text(workspace.name)
          .layoutWeight(1)
          .constraintSize({ minWidth: 0 })
          .fontSize(14)
          .fontWeight(FontWeight.Normal)
          .fontColor($r('app.color.home_text'))
          .textAlign(TextAlign.Start)
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
          .ellipsisMode(EllipsisMode.CENTER)
          .onHover((hovered: boolean) => {
            this.hoveredTask = hovered ? workspace.taskPath : '';
          })
          .bindPopup(this.hoveredTask === workspace.taskPath && this.available, {
            message: workspace.name,
            placement: Placement.Top,
            focusable: false
          })

        Row({ space: 8 }) {
          Text(workspace.formatLabel)
            .width(104)
            .flexShrink(0)
            .fontSize(12)
            .fontWeight(FontWeight.Normal)
            .fontColor($r('app.color.home_secondary'))
            .textAlign(TextAlign.Start)
            .maxLines(1)
          Text(workspace.timeLabel)
            .width(152)
            .flexShrink(0)
            .fontSize(11)
            .fontWeight(FontWeight.Normal)
            .fontColor($r('app.color.home_muted'))
            .textAlign(TextAlign.End)
            .maxLines(1)
        }
        .flexShrink(0)
      }
      .width('100%')
      .alignItems(VerticalAlign.Center)
    }
    .width('100%')
    .height(this.rowHeight)
    .padding(0)
    .backgroundColor(Color.Transparent)
    .borderRadius(8)
    .hoverEffect(HoverEffect.Auto)
    .enabled(this.available && workspace.canOpen)
    .accessibilityText(`${workspace.name}，${workspace.formatLabel}，${workspace.timeLabel}`)
    .onClick(() => {
      this.hoveredTask = '';
      this.onOpen(workspace.taskPath);
    })
  }

  build() {
    Column({ space: 8 }) {
      Text('最近工作区')
        .fontSize(16)
        .fontWeight(FontWeight.Medium)
        .fontColor($r('app.color.home_text'))
        .margin({ bottom: 8 })
      if (this.loading) {
        Text('正在读取最近工作区…')
          .fontSize(12)
          .fontColor($r('app.color.home_muted'))
      } else if (this.error !== '') {
        Text(this.error)
          .fontSize(12)
          .fontColor($r('app.color.home_warning'))
      } else if (this.workspaces.length === 0) {
        Text('暂无最近工作区')
          .fontSize(12)
          .fontColor($r('app.color.home_muted'))
      } else {
        // 标题留在外层，只让记录滚动；少量记录按实际高度收拢，单条也能贴底。
        Scroll() {
          Column({ space: this.rowSpacing }) {
            ForEach(this.workspaces, (workspace: RecentWorkspace) => {
              this.workspaceRow(workspace)
            }, (workspace: RecentWorkspace) => workspace.taskPath)
          }
          .width('100%')
        }
        .width('100%')
        .height(Math.min(this.workspaces.length * this.rowHeight +
          (this.workspaces.length - 1) * this.rowSpacing, this.maxListHeight))
        .align(Alignment.TopStart)
        .scrollBar(BarState.Auto)
      }
    }
    .width('100%')
    .alignItems(HorizontalAlign.Start)
  }
}
