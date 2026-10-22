// CEF 平台适配层按此固定路径启动渲染子进程。
import { WebChildProcess } from 'web_engine/childProcess';

export default class CustomChildProcess extends WebChildProcess {
  onStart(): void {
    super.onStart();
  }

  static keepModule(): string { return 'HuiwenCefChildProcess'; }
}
