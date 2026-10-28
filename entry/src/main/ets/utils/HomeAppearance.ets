import { common, ConfigurationConstant } from '@kit.AbilityKit';
import { preferences } from '@kit.ArkData';

export enum HomeAppearance {
  System = 'system',
  Light = 'light',
  Dark = 'dark'
}

function setHomeColorMode(context: common.Context, appearance: HomeAppearance): void {
  const mode = appearance === HomeAppearance.Light ? ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT :
    appearance === HomeAppearance.Dark ? ConfigurationConstant.ColorMode.COLOR_MODE_DARK :
      ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET;
  context.getApplicationContext().setColorMode(mode);
}

export async function applyHomeAppearance(context: common.Context, appearance: HomeAppearance): Promise<void> {
  const settings = preferences.getPreferencesSync(context, { name: 'home_settings' });
  settings.putSync('appearance', appearance);
  await settings.flush();
  setHomeColorMode(context, appearance);
}

export function initializeHomeAppearance(context: common.Context): void {
  // Ability 创建时显式读取磁盘，避免依赖尚未初始化的 UI 持久存储后端。
  const settings = preferences.getPreferencesSync(context, { name: 'home_settings' });
  const saved = settings.getSync('appearance', HomeAppearance.System);
  const appearance = saved === HomeAppearance.Light ? HomeAppearance.Light :
    saved === HomeAppearance.Dark ? HomeAppearance.Dark : HomeAppearance.System;
  AppStorage.setOrCreate('homeAppearance', appearance);
  setHomeColorMode(context, appearance);
}
