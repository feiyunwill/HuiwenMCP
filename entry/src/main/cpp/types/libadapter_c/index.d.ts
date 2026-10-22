// 迁入的预编译 CEF 节点适配库；声明汇文及 HAR 实际使用的现有接口。
interface CefNodeAdapter {
  initFunc(scaledDensity: number): void;
  getWindowNameByXComponentId(id: string): string;
}
declare const adapter: CefNodeAdapter;
export default adapter;
