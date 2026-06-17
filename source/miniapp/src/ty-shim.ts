/**
 * ty-shim.ts — TuyaOpen IDE preview shim.
 *
 * When this miniapp template runs inside the TuyaOpen IDE preview iframe,
 * Tuya's `gzlJSBridge` (Electron preload) is absent. This file synthesises a
 * minimal `window.ty` and `window.gzlJSBridge` that route every panel API call
 * to the parent webview via postMessage. The webview forwards it to the
 * extension host, which owns a virtual device and (optionally) a cloud bridge.
 *
 * If a real bridge is already present (running inside Tuya MiniApp IDE),
 * we step aside and let it own the globals.
 */

declare global {
  interface Window {
    ty?: any;
    gzlJSBridge?: any;
    __tuyaopenPreviewShim?: boolean;
  }
}

if (
  typeof window !== 'undefined' &&
  !window.gzlJSBridge &&
  !window.__tuyaopenPreviewShim
) {
  installShim();
}

function installShim() {
  const NS = 'tymp';
  const pendings = new Map<
    string,
    { resolve: (v: any) => void; reject: (e: any) => void }
  >();
  const listeners = {
    dpChange: new Set<(p: any) => void>(),
    mqtt: new Set<(p: any) => void>(),
    deviceInfoUpdate: new Set<(p: any) => void>(),
  };

  let initToken: string | null = null;
  let parentReady = false;
  const queuedPings: Array<() => void> = [];

  function uuid(): string {
    return (
      Date.now().toString(36) +
      '-' +
      Math.random().toString(36).slice(2, 10)
    );
  }

  function postToParent(msg: any) {
    try {
      window.parent.postMessage(msg, '*');
    } catch (err) {
      // parent disappeared — preview is detached, swallow.
    }
  }

  function rpc(api: string, args: any[]): Promise<any> {
    return new Promise((resolve, reject) => {
      const id = uuid();
      pendings.set(id, { resolve, reject });
      const send = () =>
        postToParent({ ns: NS, kind: 'invoke', id, api, args, token: initToken });
      if (parentReady) send();
      else queuedPings.push(send);
      // Soft timeout so panels don't hang forever when host is gone.
      setTimeout(() => {
        if (pendings.has(id)) {
          pendings.delete(id);
          // eslint-disable-next-line no-console
          console.warn('[tuyaopen-shim] bridge not ready — RPC timed out after 5s:', api);
          reject(new Error(`[tuyaopen-shim] timeout calling ${api}`));
        }
      }, 5_000);
    });
  }

  // Wrap Tuya's success/fail callback shape into a promise-returning fn that
  // ALSO calls success/fail/complete if provided.
  function callbackBridge(api: string) {
    return (params: any = {}) => {
      const { success, fail, complete, ...rest } = params || {};
      return rpc(api, [rest])
        .then((data: any) => {
          if (typeof success === 'function') success(data);
          if (typeof complete === 'function') complete(data);
          return data;
        })
        .catch((err: any) => {
          const errBody = {
            errorCode: err?.code ?? 'SHIM_ERR',
            errorMsg: err?.message ?? String(err),
            innerError: err,
          };
          if (typeof fail === 'function') fail(errBody);
          if (typeof complete === 'function') complete(errBody);
          throw err;
        });
    };
  }

  function listenerBridge<T>(set: Set<(p: T) => void>) {
    const on = (cb: (p: T) => void) => {
      set.add(cb);
    };
    const off = (cb: (p: T) => void) => {
      set.delete(cb);
    };
    return { on, off };
  }

  const dpListener = listenerBridge(listeners.dpChange);
  const mqttListener = listenerBridge(listeners.mqtt);
  const deviceListener = listenerBridge(listeners.deviceInfoUpdate);

  // Build minimal device namespace. Unknown methods auto-RPC.
  const deviceTargets: Record<string, any> = {
    getDeviceInfo: callbackBridge('device.getDeviceInfo'),
    publishDps: callbackBridge('device.publishDps'),
    publishDpsWithPipeType: callbackBridge('device.publishDps'),
    queryDps: callbackBridge('device.queryDps'),
    getProductInfo: callbackBridge('device.getProductInfo'),
    registerMQTTDeviceListener: callbackBridge('device.registerMQTTDeviceListener'),
    unregisterMQTTDeviceListener: callbackBridge('device.unregisterMQTTDeviceListener'),
    onDpDataChange: dpListener.on,
    offDpDataChange: dpListener.off,
    onMqttMessageReceived: mqttListener.on,
    offMqttMessageReceived: mqttListener.off,
    onDeviceInfoUpdated: deviceListener.on,
    offDeviceInfoUpdated: deviceListener.off,
  };

  const deviceProxy = new Proxy(deviceTargets, {
    get(target, prop: string) {
      if (prop in target) return target[prop];
      // Unknown device API → fall back to RPC, but warn once.
      // Returning undefined would crash strict callers.
      if (typeof prop === 'string' && /^[a-zA-Z]/.test(prop)) {
        const fn = callbackBridge(`device.${prop}`);
        target[prop] = fn;
        return fn;
      }
      return undefined;
    },
  });

  // Top-level ty.* helpers (toast/loading/storage/navigate). All forward
  // to RPC; storage falls back to localStorage so the simplest paths work
  // even before the host responds.
  function ramStorage(key: string) {
    try {
      return JSON.parse(window.localStorage.getItem(`tymp:${key}`) || 'null');
    } catch {
      return null;
    }
  }
  function ramSetStorage(key: string, value: any) {
    try {
      window.localStorage.setItem(`tymp:${key}`, JSON.stringify(value));
    } catch {
      /* ignore */
    }
  }

  const tyTargets: Record<string, any> = {
    device: deviceProxy,
    showToast: callbackBridge('ui.showToast'),
    hideToast: callbackBridge('ui.hideToast'),
    showLoading: callbackBridge('ui.showLoading'),
    hideLoading: callbackBridge('ui.hideLoading'),
    showModal: callbackBridge('ui.showModal'),
    showActionSheet: callbackBridge('ui.showActionSheet'),
    requestCloud: callbackBridge('cloud.request'),
    getStorage: (params: any = {}) => {
      const v = ramStorage(params.key);
      if (v != null && typeof params.success === 'function') params.success({ data: v });
      return Promise.resolve({ data: v });
    },
    setStorage: (params: any = {}) => {
      ramSetStorage(params.key, params.data);
      if (typeof params.success === 'function') params.success({});
      return Promise.resolve({});
    },
    getStorageSync: (key: string) => ramStorage(key),
    setStorageSync: (key: string, value: any) => ramSetStorage(key, value),
    getSystemInfoSync: () => ({
      brand: 'devtools',
      platform: 'devtools',
      system: 'TuyaOpenIDE',
      language: navigator.language,
      pixelRatio: window.devicePixelRatio || 2,
      windowWidth: window.innerWidth,
      windowHeight: window.innerHeight,
      statusBarHeight: 20,
      version: '1.0.0',
    }),
    getSystemInfo: callbackBridge('system.getSystemInfo'),
    navigateBack: callbackBridge('router.navigateBack'),
    // navigateTo / redirectTo / switchTab / reLaunch left to Ray router
  };

  const tyProxy = new Proxy(tyTargets, {
    get(target, prop: string) {
      if (prop in target) return target[prop];
      if (typeof prop === 'string' && /^[a-zA-Z]/.test(prop)) {
        const fn = callbackBridge(prop);
        target[prop] = fn;
        return fn;
      }
      return undefined;
    },
  });

  window.ty = tyProxy;

  // Minimal mothra-shaped gzlJSBridge for code paths that bypass `ty` and
  // poke the bridge directly.
  window.gzlJSBridge = {
    invoke(message: any, callback?: (...a: any[]) => void) {
      const api = message?.api || message?.name || message?.method || 'unknown';
      const data = message?.data ?? message?.params ?? {};
      rpc(`bridge.${api}`, [data]).then(
        (res: any) => callback?.(res),
        (err: any) => callback?.({ errorMsg: err?.message ?? 'shim error' })
      );
    },
    setHandler() {
      /* no-op for now */
    },
  };

  // Receive replies + events + init handshake from parent webview.
  window.addEventListener('message', (ev: MessageEvent) => {
    const m = ev?.data;
    if (!m || m.ns !== NS) return;
    if (m.kind === 'init') {
      initToken = m.token;
      parentReady = true;
      while (queuedPings.length) {
        const fn = queuedPings.shift();
        if (fn) fn();
      }
      return;
    }
    if (m.kind === 'reply') {
      const p = pendings.get(m.id);
      if (!p) return;
      pendings.delete(m.id);
      if (m.ok) p.resolve(m.data);
      else p.reject(m.error || new Error('rpc failed'));
      return;
    }
    if (m.kind === 'event') {
      const topic: string = m.topic;
      const payload = m.payload;
      if (topic === 'device.dpChange') {
        for (const cb of listeners.dpChange) {
          try { cb(payload); } catch (e) { console.error('[tymp] listener error', e); }
        }
      } else if (topic === 'device.mqtt') {
        for (const cb of listeners.mqtt) {
          try { cb(payload); } catch (e) { console.error('[tymp] listener error', e); }
        }
      } else if (topic === 'device.infoUpdate') {
        for (const cb of listeners.deviceInfoUpdate) {
          try { cb(payload); } catch (e) { console.error('[tymp] listener error', e); }
        }
      }
    }
  });

  // Tell parent we're alive.
  postToParent({ ns: NS, kind: 'hello' });

  window.__tuyaopenPreviewShim = true;
  // eslint-disable-next-line no-console
  console.info('[tuyaopen-preview] ty shim installed');
}

export {};
