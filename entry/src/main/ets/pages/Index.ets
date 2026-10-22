// QPA 固定加载 pages/Index；这里只承载 Qt 节点，不放首页和窗口关闭监听。
import CustomChildProcess from '../process/CustomChildProcess';

// 保留 CEF 动态启动的子进程模块，与适配层固定路径一致。
CustomChildProcess.keepModule();
const storage = LocalStorage.getShared();

@Entry(storage)
@Component
struct Index {
  // QPA 在 QtWorkspaceAbility.newLocalStorage() 返回的存储中写入窗口节点 ID。
  @LocalStorageProp('idName') windowId: string = '';

  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      XComponent({
        id: this.windowId,
        type: XComponentType.NODE,
        libraryname: 'plugins_platforms_qopenharmony'
      })
        .width('100%')
        .height('100%')
    }
    .width('100%')
    .height('100%')
  }
}
