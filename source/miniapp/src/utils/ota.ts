// OTA upgrade helper. In IDE dev-preview mode, ty.checkOtaUpgrade is not
// stubbed, so all calls are caught and return null (card stays hidden).

declare const ty: any;

export interface OtaInfo {
  targetVersion: string;
  firmwareSize: number;
  upgrade(onProgress: (pct: number) => void): Promise<void>;
}

export async function checkOta(): Promise<OtaInfo | null> {
  const info = await (typeof ty !== 'undefined' && ty.checkOtaUpgrade
    ? ty.checkOtaUpgrade()
    : Promise.resolve(null)
  ).catch(() => null);

  if (!info?.upgradeAvailable) return null;
  return {
    targetVersion: info.targetVersion,
    firmwareSize: info.firmwareSize,
    upgrade: async (onProgress: (pct: number) => void) => {
      await ty.downloadOtaUpgrade({
        onProgress: ({ progress }: { progress: number }) => onProgress(progress),
      });
      await ty.startOtaUpgrade();
    },
  };
}
