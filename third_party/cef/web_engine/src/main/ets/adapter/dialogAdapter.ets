import libAddon from 'libadapter.so';

type DialogCallbackType = (success: boolean, userInput: string) => void;
type PromptDialogCreateFuncType = (title: string, message: string, promptMessage: string, callback: DialogCallbackType) => void;

enum DialogAdapterType {
  JAVASCRIPT_DIALOG_TYPE_ALERT,
  JAVASCRIPT_DIALOG_TYPE_CONFIRM,
  JAVASCRIPT_DIALOG_TYPE_PROMPT
};

export default class DialogAdapter {
  static CreatePromptDialog: PromptDialogCreateFuncType | undefined = undefined;

  static BindDialogMethods() {
    libAddon.JSBind.bindFunction("DialogAdapter.ShowDialog", DialogAdapter.ShowDialog);
  }

  static SetPromptDialogCreateFunction(func: PromptDialogCreateFuncType) {
    DialogAdapter.CreatePromptDialog = func;
  }

  static ShowDialog(type: DialogAdapterType, title: string, message: string, promptMessage: string, callback: DialogCallbackType) {
    if (type === DialogAdapterType.JAVASCRIPT_DIALOG_TYPE_ALERT) {
      DialogAdapter.ShowAlert(title, message, callback);
    } else if (type === DialogAdapterType.JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
      DialogAdapter.ShowConfirm(title, message, callback);
    } else if (type === DialogAdapterType.JAVASCRIPT_DIALOG_TYPE_PROMPT) {
      DialogAdapter.ShowPrompt(title, message, promptMessage, callback);
    }
  }

  static ShowAlert(title: string, message: string, callback: DialogCallbackType) {
    AlertDialog.show({
      title: title,
      message: message,
      autoCancel: false,
      cancel: () => {},
      primaryButton: {
        value: $r('app.string.button_ok'),
        style: DialogButtonStyle.HIGHLIGHT,
        action: () => {
          callback(false, '');
        },
      }
    });
  }

  static ShowConfirm(title: string, message: string, callback: DialogCallbackType) {
    AlertDialog.show({
      title: title,
      message: message,
      autoCancel: false,
      cancel: () => {},
      primaryButton: {
        value: $r('app.string.button_cancel'),
        style: DialogButtonStyle.HIGHLIGHT,
        action: () => {
          callback(false, '');
        },
      },
      secondaryButton: {
        value: $r('app.string.button_ok'),
        style: DialogButtonStyle.HIGHLIGHT,
        action: () => {
          callback(true, '');
        }
      }
    });
  }

  static ShowPrompt(title: string, message: string, promptMessage: string, callback: DialogCallbackType) {
    DialogAdapter.CreatePromptDialog &&
      DialogAdapter.CreatePromptDialog(title, message, promptMessage, callback);
  }
}