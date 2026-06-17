import './ty-shim';
import React from 'react';
import { initPanelEnvironment } from '@ray-js/ray';
import { SdmProvider } from '@ray-js/panel-sdk';
import { devices, dpKit, initOfflinePreview, installOfflineBridge } from '@/devices';
import './app.less';

// Initialize the panel runtime BEFORE the App class is registered.
// `useDefaultOffline: true` is a hint to panel-sdk that we're OK without
// a real cloud device. The real offline bootstrap happens in
// `initOfflinePreview()` below — without it, SdmProvider renders null
// and the page-frame body stays empty.
initPanelEnvironment({ useDefaultOffline: true });

// Pre-populate the SmartDeviceModel with mock state derived from the
// cloud-synced schema so SdmProvider passes its `device.initialized`
// gate and renders children. Auto-no-ops when `getLaunchOptionsSync()`
// reports a `deviceId` or `groupId` — i.e. inside Tuya MiniApp IDE
// against a real simulator device — so the production upload bundle
// runs the original SmartDeviceModel.init() path unchanged.
initOfflinePreview();

// Ray framework hands an instance ref to the App component and expects
// onLaunch / onShow / onHide methods on it (called via createAppInstance).
// Function components cannot accept refs, so this MUST be a class component
// for the framework to register the app instance correctly.
//
// Bridge install timing — the bundle is wrapped in minipack's AMD-style IIFE.
// Mothra's service.html runtime calls the AMD factory with only a few explicit
// args; `window` (a closure parameter at position 6) is left undefined. The
// fix in installOfflineBridge() resolves the real window via `globalThis`,
// which is NOT in the AMD parameter list and therefore not shadowed. Calling
// installOfflineBridge() from componentDidMount remains good practice as a
// second-chance install, but the primary install now succeeds at module-eval
// time via initOfflinePreview() because globalThis is defined in service.html.
export default class App extends React.Component<{ children?: React.ReactNode }> {
  onLaunch() {
    // Bootstrap the SmartDeviceModel for real-device runs. In preview
    // mode this is a no-op because `initialized` is already true.
    if (!(devices.common as any).initialized) {
      devices.common.init();
      devices.common.onInitialized((device) => dpKit.init(device));
    } else {
      dpKit.init(devices.common);
    }
  }
  onShow() { /* called on app foreground */ }
  onHide() { /* called on app background */ }
  componentDidMount() {
    // Runs in the React commit phase — service.html iframe context where
    // window IS defined. installOfflineBridge is idempotent (guarded by
    // __offlinePreviewBridgeInstalled__) so retries from re-mount are safe.
    installOfflineBridge();
  }
  render() {
    return (
      <SdmProvider value={devices.common}>
        {this.props.children}
      </SdmProvider>
    );
  }
}
// trigger watch 1779346778
