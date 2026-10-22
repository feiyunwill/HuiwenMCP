import libAddon from 'libadapter.so';

type DialogCallbackType = (success: boolean, userInput: string) => void;

const TITLE_SIZE: number = 20;
const SUBTITLE_SIZE: number = 18;
const ELE_MARGIN: number = 10;
const TEXT_INPUT_HEIGHT: number = 50;
const TEXT_INPUT_WIDTH_PCT: string = '90%';
const BUTTON_WIDTH_PCT: string = '40%';
const BORDER_RADIUS: number = 10;

@CustomDialog
export struct PromptDialog {
  private title: string = '';
  private subTitle: string = '';
  private textValue: string = '';
  private inputValue: string = '';
  private callback: DialogCallbackType | undefined = undefined;

  controller?: CustomDialogController

  cancel: () => void = () => {
    this.callback && this.callback(false, '');
  }
  confirm: () => void = () => {
    this.callback && this.callback(true, this.inputValue);
  }

  build() {
    Column() {
      Text(this.title).fontSize(TITLE_SIZE).margin({ top: ELE_MARGIN})
      Text(this.subTitle).fontSize(SUBTITLE_SIZE).margin({ top: ELE_MARGIN})
      TextInput({ placeholder: '', text: this.textValue })
        .height(TEXT_INPUT_HEIGHT)
        .width(TEXT_INPUT_WIDTH_PCT)
        .margin({ top: ELE_MARGIN })
        .onChange((value: string) => {
          this.textValue = value
        })
      Flex({ justifyContent: FlexAlign.SpaceAround }) {
        Button($r('app.string.button_cancel'))
          .onClick(() => {
            if (this.controller != undefined) {
              this.controller.close()
              this.cancel()
            }
          })
          .backgroundColor($r('app.color.button_background'))
          .fontColor($r('app.color.button_font'))
          .width(BUTTON_WIDTH_PCT)
        Button($r('app.string.button_ok'))
          .onClick(() => {
            if (this.controller != undefined) {
              this.inputValue = this.textValue
              this.controller.close()
              this.confirm()
            }
          })
          .backgroundColor($r('app.color.button_background'))
          .fontColor($r('app.color.button_font'))
          .width(BUTTON_WIDTH_PCT)
      }.margin({ top: ELE_MARGIN, bottom: ELE_MARGIN })
    }.borderRadius(BORDER_RADIUS)
  }
}