
declare namespace ty {
  
  export function getAIAssistantHistory(params: {
    
    size: number
    
    primaryId: number
    
    requestId: string
    
    type: string
    
    channel: string
    complete?: () => void
    success?: (params: {
      
      data: string
      
      channel: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function deleteAIAssistant(params: {
    
    primaryId: number
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function sendAIAssistant(params: {
    
    block: string
    
    options: string
    
    type: string
    
    requestId?: string
    complete?: () => void
    success?: (params: {
      
      requestId: string
      
      primaryId: number
      
      code: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function createAIAssistant(params?: {
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getAIAssistantRequestId(params?: {
    complete?: () => void
    success?: (params: {
      
      requestId: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function insertAIAssistantInfo(params: {
    
    requestId: string
    
    type: string
    
    data: string
    
    code: string
    
    message: string
    
    source: string
    
    channel: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function updateAIAssistantInfo(params: {
    
    primaryId: number
    
    data: string
    
    code: string
    
    message: string
    
    source: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getSpeechDisplayType(params?: {
    complete?: () => void
    success?: (params: {
      
      type: number
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function disableAIAssistant(params?: {
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function enableAIAssistant(params?: {
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function deleteAIAssistantDbSource(params: {
    
    homeId: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getSingleAIAssistant(params: {
    
    primaryId: number
    complete?: () => void
    success?: (params: {
      
      primaryId: number
      
      requestId: string
      
      source: string
      
      data: string
      
      code: number
      
      type: string
      
      createTime: number
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function isSupportOldSpeech(params?: {
    complete?: () => void
    success?: (params: boolean) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function apiRequestByAtop(params: {
    
    api: string
    
    version?: string
    
    postData: Record<string, any>
    
    extData?: Record<string, any>
    complete?: () => void
    success?: (params: {
      
      thing_json_?: {}
      
      data: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function apiRequestByHighwayRestful(params: {
    
    host?: string
    
    api: string
    
    header?: Record<string, any>
    
    query?: Record<string, any>
    
    body?: Record<string, any>
    
    method?: HighwayMethod
    complete?: () => void
    success?: (params: {
      
      result: {}
      
      api: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function event(params: {
    
    eventId: string
    
    event: Record<string, {}>
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function beginEvent(params: {
    
    eventName: string
    
    identifier: string
    
    attributes: Record<string, {}>
    
    infos: Record<string, {}>
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function trackEvent(params: {
    
    eventName: string
    
    identifier: string
    
    attributes: Record<string, {}>
    
    infos: Record<string, {}>
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function endEvent(params: {
    
    eventName: string
    
    identifier: string
    
    attributes: Record<string, {}>
    
    infos: Record<string, {}>
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function eventLink(params: {
    
    linkIndex: number
    
    linkId: string
    
    params: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getAppInfo(params?: {
    complete?: () => void
    success?: (params: {
      
      serverTimestamp: number
      appVersion: string
      language: string
      countryCode: string
      regionCode: string
      
      appName: string
      
      appIcon: string
      
      appEnv?: number
      
      appBundleId: string
      
      appScheme: string
      
      appId: string
      
      clientId?: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getCurrentWifiSSID(params?: {
    complete?: () => void
    success?: (params: {
      
      ssId: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getNGConfigByKeys(params: {
    
    keys: string[]
    complete?: () => void
    success?: (params: {
      
      config: Record<string, {}>
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getConfigByKeys(params: {
    
    keys: string[]
    complete?: () => void
    success?: (params: {
      
      config: Record<string, {}>
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getThirdPartyServiceInfo(params: {
    
    types: number[]
    complete?: () => void
    success?: (params: ThirdPartyServiceInfo[]) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function openThirdPartyMiniProgram(params: {
    
    type: number
    
    params: Record<string, {}>
    complete?: () => void
    success?: (params: {
      
      data: Record<string, {}>
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function openCountrySelectPage(params?: {
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getIconfontInfo(params?: {
    complete?: () => void
    success?: (params: {
      
      nameMap: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function iapPay(params: {
    
    orderID: string
    
    productID: string
    
    preFlowCode: string
    complete?: () => void
    success?: (params: {
      
      orderID: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function iapType(params?: {
    complete?: () => void
    success?: (params: {
      
      data: number
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function uploadImage(params: {
    
    filePath: string
    
    bizType: string
    
    contentType?: string
    
    delayTime?: number
    
    pollMaxCount?: number
    complete?: () => void
    success?: (params: {
      
      result: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function uploadVideo(params: {
    
    filePath: string
    
    bizType: string
    
    contentType?: string
    complete?: () => void
    success?: (params: {
      
      result: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getLangKey(params?: {
    complete?: () => void
    success?: (params: {
      
      langKey: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getLangContent(params?: {
    complete?: () => void
    success?: (params: {
      
      langContent: {}
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function registerPageRefreshListener(params?: {
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getNgRawData(params: {
    
    rawKey: string
    complete?: () => void
    success?: (params: {
      
      rawData: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getOnlineCustomerService(params?: {
    complete?: () => void
    success?: (params: { url: string }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function openRNPanel(params: {
    
    deviceId: string
    
    uiId: string
    
    panelUiInfoBean?: PanelUiInfoBean
    
    initialProps?: Record<string, {}>
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function openPanel(params: {
    
    deviceId: string
    
    extraInfo?: PanelExtraParams
    
    initialProps?: Record<string, {}>
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function backToHomeAndOpenPanel(params: {
    
    deviceId: string
    
    extraInfo?: PanelExtraParams
    
    initialProps?: Record<string, {}>
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function preloadPanel(params: {
    
    deviceId: string
    
    extraInfo?: PanelExtraParams
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function openInnerH5(params: {
    
    url: string
    
    title?: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function openAppSystemSettingPage(params: {
    
    scope: string
    
    requestCode?: number
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function openSystemSettingPage(params: {
    
    scope: string
    
    requestCode?: number
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function emitChannel(params: {
    
    eventName: string
    
    event?: {}
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function setActivityResult(params: {
    
    resultCode: number
    
    data?: Record<string, {}>
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function openThirdApp(params: {
    
    uriString: string
    
    packageName: string
    complete?: () => void
    success?: (params: { isCanOpen?: boolean }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function openThirdAppSync(params?: ThirdAppBean): {
    isCanOpen?: boolean
  }

  
  export function openUrlForceDefaultBrowser(params: {
    
    url: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getIapInfo(params?: {
    complete?: () => void
    success?: (params: {
      
      iapType?: number
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function iapPayReady(params: {
    
    subscription: number
    complete?: () => void
    success?: (params: {
      
      result: boolean
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function pay(params: {
    
    order_id: string
    
    product_id: string
    
    subscription?: number
    
    billing_mode?: number
    
    previous_sku?: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function addOrderStatusListener(params: {
    
    order_id: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function removeOrderStatusListener(params: {
    
    order_id: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function router(params: {
    
    url: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function canIUseRouter(params: {
    
    url: string
    complete?: () => void
    success?: (params: {
      
      result: boolean
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function goDeviceDetail(params: {
    
    deviceId: string
    
    groupId?: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function goDeviceAlarm(params: {
    
    deviceId: string
    
    groupId?: string
    
    category?: string
    
    repeat?: number
    
    timerConfig?: TimeConfig
    
    data: {}[]
    
    enableFilter?: boolean
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function share(params: {
    
    type: string
    
    title: string
    
    message: string
    
    contentType: string
    
    recipients?: string[]
    
    imagePath?: string
    
    filePath?: string
    
    webPageUrl?: string
    
    miniProgramInfo?: MiniProgramInfo
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getShareChannelList(params?: {
    complete?: () => void
    success?: (params: {
      
      shareChannelList: string[]
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function isSupportedShortcut(params?: {
    complete?: () => void
    success?: (params: {
      
      isSupported: boolean
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function isAssociatedShortcut(params: {
    
    sceneId: string
    
    name?: string
    complete?: () => void
    success?: (params: {
      
      isAssociated: boolean
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function handleShortcut(params: {
    
    type: number
    
    sceneId: string
    
    name: string
    
    iconUrl?: string
    complete?: () => void
    success?: (params: {
      
      operationStep: number
      
      operationStatus: boolean
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getUserInfo(params?: {
    complete?: () => void
    success?: (params: {
      
      nickName: string
      
      avatarUrl: string
      
      phoneCode: string
      
      isTemporaryUser: boolean
    }) => void
    failure?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function resizeImage(params: {
    
    aspectFitWidth: number
    
    aspectFitHeight: number
    
    maxFileSize?: number
    
    path: string
    complete?: () => void
    success?: (params: {
      
      path: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function rotateImage(params: {
    
    path: string
    
    orientation: number
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function saveToAlbum(params: {
    
    url: string
    
    encryptKey: string
    
    orientation: number
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getWebSocketStatus(params?: {
    complete?: () => void
    success?: (params: {
      
      status: number
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function getWebSocketStatusSync(): {
    
    status: number
  }

  
  export function bindWechat(params?: {
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function isSupportWechat(params?: {
    complete?: () => void
    success?: (params: {
      
      isSupport: boolean
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function gotoWechatMiniApp(params: {
    
    miniAppId: string
    
    path: string
    
    miniProgramType: number
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function isCalling(params?: {
    complete?: () => void
    success?: (params: {
      
      result: boolean
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function canLaunchCall(params?: {
    complete?: () => void
    success?: (params: {
      
      result: boolean
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function launchCall(params: {
    
    targetId: string
    
    timeout: number
    
    extra: Record<string, any>
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function onAIAssistantChange(
    listener: (params: ReceiveBean) => void
  ): void

  
  export function offAIAssistantChange(
    listener: (params: ReceiveBean) => void
  ): void

  
  export function onCountrySelectResult(
    listener: (params: CountrySelectResultResponse) => void
  ): void

  
  export function offCountrySelectResult(
    listener: (params: CountrySelectResultResponse) => void
  ): void

  
  export function onUploadProgressUpdate(
    listener: (params: ProgressEvent) => void
  ): void

  
  export function offUploadProgressUpdate(
    listener: (params: ProgressEvent) => void
  ): void

  
  export function onPageRefresh(listener: (params: RefreshParams) => void): void

  
  export function offPageRefresh(
    listener: (params: RefreshParams) => void
  ): void

  
  export function onReceiveMessage(
    listener: (params: EventChannelMessageParams) => void
  ): void

  
  export function offReceiveMessage(
    listener: (params: EventChannelMessageParams) => void
  ): void

  
  export function onOrderStatusListener(
    listener: (params: OrderStatusEvent) => void
  ): void

  
  export function offOrderStatusListener(
    listener: (params: OrderStatusEvent) => void
  ): void

  
  export function onRouterEvent(listener: (params: RouterEvent) => void): void

  
  export function offRouterEvent(listener: (params: RouterEvent) => void): void

  
  export function onRouterResult(
    listener: (params: RouterResultResponse) => void
  ): void

  
  export function offRouterResult(
    listener: (params: RouterResultResponse) => void
  ): void

  
  export function onFrontPageClose(
    listener: (params: PageCloseResponse) => void
  ): void

  
  export function offFrontPageClose(
    listener: (params: PageCloseResponse) => void
  ): void

  
  export function onWebSocketStatusChange(
    listener: (params: StatusBean) => void
  ): void

  
  export function offWebSocketStatusChange(
    listener: (params: StatusBean) => void
  ): void

  export enum HighwayMethod {
    
    GET = "GET",

    
    POST = "POST",

    
    PUT = "PUT",

    
    DELETE = "DELETE",
  }

  export type ThirdPartyServiceInfo = {
    
    available: boolean
    
    isAppInstalled: boolean
    
    appInstallUrl: string
    
    type: number
  }

  export type PanelUiInfoBean = {
    
    phase?: string
    
    type?: string
    
    ui?: string
    
    version?: string
    
    appRnVersion?: string
    
    name?: string
    
    uiConfig?: Record<string, {}>
    
    rnFind?: boolean
    
    pid?: string
    
    i18nTime?: number
  }

  export type PanelExtraParams = {
    
    productId: string
    
    productVersion: string
    
    i18nTime: string
    
    bizClientId: string
    
    uiType?: string
    
    uiPhase?: string
  }

  export type ThirdAppBean = {
    
    uriString: string
    
    packageName: string
  }

  export type TimeConfig = {
    
    background: string
  }

  export type MiniProgramInfo = {
    
    userName: string
    
    path: string
    
    hdImagePath: string
    
    withShareTicket: boolean
    
    miniProgramType: number
    
    webPageUrl: string
  }

  export type ReceiveBean = {
    
    data: string
  }

  export type CountrySelectResultResponse = {
    
    countryCode?: string
    
    countryAbb?: string
    
    countryName?: string
  }

  export type ProgressEvent = {
    
    filePath: string
    
    progress: number
  }

  export type RefreshParams = {
    
    key: string
    
    data?: Record<string, {}>
  }

  export type EventChannelMessageParams = {
    
    eventId: string
    
    event?: {}
  }

  export type OrderStatusEvent = {
    
    order_id: string
    
    resultCode: number
    
    errorCode?: number
    
    errorMsg?: string
  }

  export type RouterEvent = {
    
    bizEventName: string
    
    bizEventData: Object
  }

  export type RouterResultResponse = {
    
    url: string
    
    data?: string
  }

  export type PageCloseResponse = {
    
    from?: string
  }

  export type StatusBean = {
    
    status: number
  }

  export type ApiRequestByAtopParams = {
    
    api: string
    
    version?: string
    
    postData: Record<string, any>
    
    extData?: Record<string, any>
  }

  export type ApiRequestByAtopResponse = {
    
    thing_json_?: {}
    
    data: string
  }

  export type HighwayReq = {
    
    host?: string
    
    api: string
    
    header?: Record<string, any>
    
    query?: Record<string, any>
    
    body?: Record<string, any>
    
    method?: HighwayMethod
  }

  export type HighwayResp = {
    
    result: {}
    
    api: string
  }

  export type EventBean = {
    
    eventId: string
    
    event: Record<string, {}>
  }

  export type TrackEventBean = {
    
    eventName: string
    
    identifier: string
    
    attributes: Record<string, {}>
    
    infos: Record<string, {}>
  }

  export type EventLinkBean = {
    
    linkIndex: number
    
    linkId: string
    
    params: string
  }

  export type ManagerContext = {
    
    managerId: number
    
    homeId: string
    
    sampleRate: number
    
    channels: number
    
    codec: string
    
    options: string
  }

  export type AsrManagerContext = {
    
    managerId: number
  }

  export type Active = {
    
    isActive: boolean
  }

  export type AppInfoBean = {
    
    serverTimestamp: number
    appVersion: string
    language: string
    countryCode: string
    regionCode: string
    
    appName: string
    
    appIcon: string
    
    appEnv?: number
    
    appBundleId: string
    
    appScheme: string
    
    appId: string
    
    clientId?: string
  }

  export type SystemWirelessInfoBean = {
    
    ssId: string
  }

  export type NGConfigParams = {
    
    keys: string[]
  }

  export type ConfigResponse = {
    
    config: Record<string, {}>
  }

  export type ConfigParams = {
    
    keys: string[]
  }

  export type ThirdPartyServiceParams = {
    
    types: number[]
  }

  export type ThirdPartyMiniProgramParams = {
    
    type: number
    
    params: Record<string, {}>
  }

  export type ThirdPartyMiniProgramResult = {
    
    data: Record<string, {}>
  }

  export type IconfontInfoBean = {
    
    nameMap: string
  }

  export type PaymentParam = {
    
    orderID: string
    
    productID: string
    
    preFlowCode: string
  }

  export type PaymentResponse = {
    
    orderID: string
  }

  export type TypeResponse = {
    
    data: number
  }

  export type UploadParams = {
    
    filePath: string
    
    bizType: string
    
    contentType?: string
    
    delayTime?: number
    
    pollMaxCount?: number
  }

  export type UploadResponse = {
    
    result: string
  }

  export type VideoUploadParams = {
    
    filePath: string
    
    bizType: string
    
    contentType?: string
  }

  export type LocalConstants = {
    
    langKey: string
    
    langContent: {}
  }

  export type LangKeyResult = {
    
    langKey: string
  }

  export type LangContentResult = {
    
    langContent: {}
  }

  export type ManagerContext_YsyvT3 = {
    
    managerId: number
    
    tag: string
  }

  export type ManagerReqContext = {
    
    managerId: number
    
    message: string
  }

  export type NGRequestBean = {
    
    rawKey: string
  }

  export type NGResponseBean = {
    
    rawData: string
  }

  export type CustomerServiceRes = {
    url: string
  }

  export type PanelBean = {
    
    deviceId: string
    
    uiId: string
    
    panelUiInfoBean?: PanelUiInfoBean
    
    initialProps?: Record<string, {}>
  }

  export type PanelParams = {
    
    deviceId: string
    
    extraInfo?: PanelExtraParams
    
    initialProps?: Record<string, {}>
  }

  export type PreloadPanelParams = {
    
    deviceId: string
    
    extraInfo?: PanelExtraParams
  }

  export type WebViewBean = {
    
    url: string
    
    title?: string
  }

  export type SettingPageBean = {
    
    scope: string
    
    requestCode?: number
  }

  export type EventEmitChannelParams = {
    
    eventName: string
    
    event?: {}
  }

  export type EventChannelParams = {
    
    eventId: string
    
    eventName: string
  }

  export type EventOffChannelParams = {
    
    eventId: string
  }

  export type ActivityResultBean = {
    
    resultCode: number
    
    data?: Record<string, {}>
  }

  export type CanOpenThirdAppBean = {
    isCanOpen?: boolean
  }

  export type OpenDefaultBrowserUrlBean = {
    
    url: string
  }

  export type PayInfoBean = {
    
    iapType?: number
  }

  export type iapPayReadyReq = {
    
    subscription: number
  }

  export type iapPayReadyResp = {
    
    result: boolean
  }

  export type payReq = {
    
    order_id: string
    
    product_id: string
    
    subscription?: number
    
    billing_mode?: number
    
    previous_sku?: string
  }

  export type OrderReq = {
    
    order_id: string
  }

  export type Object = {}

  export type RouterBean = {
    
    url: string
  }

  export type RouteUsageResult = {
    
    result: boolean
  }

  export type DeviceDetailBean = {
    
    deviceId: string
    
    groupId?: string
  }

  export type AlarmBean = {
    
    deviceId: string
    
    groupId?: string
    
    category?: string
    
    repeat?: number
    
    timerConfig?: TimeConfig
    
    data: {}[]
    
    enableFilter?: boolean
  }

  export type ShareInformationBean = {
    
    type: string
    
    title: string
    
    message: string
    
    contentType: string
    
    recipients?: string[]
    
    imagePath?: string
    
    filePath?: string
    
    webPageUrl?: string
    
    miniProgramInfo?: MiniProgramInfo
  }

  export type ShareChannelResponse = {
    
    shareChannelList: string[]
  }

  export type SiriEnabledResponse = {
    
    isSupported: boolean
  }

  export type ShortcutAssociatedParams = {
    
    sceneId: string
    
    name?: string
  }

  export type ShortcutAssociatedResponse = {
    
    isAssociated: boolean
  }

  export type ShortcutParams = {
    
    type: number
    
    sceneId: string
    
    name: string
    
    iconUrl?: string
  }

  export type ShortcutOperationResponse = {
    
    operationStep: number
    
    operationStatus: boolean
  }

  export type UserInfoResult = {
    
    nickName: string
    
    avatarUrl: string
    
    phoneCode: string
    
    isTemporaryUser: boolean
  }

  export type ImageResizeBean = {
    
    aspectFitWidth: number
    
    aspectFitHeight: number
    
    maxFileSize?: number
    
    path: string
  }

  export type ImageResizeResultBean = {
    
    path: string
  }

  export type ImageRotateBean = {
    
    path: string
    
    orientation: number
  }

  export type ImageEncryptBean = {
    
    url: string
    
    encryptKey: string
    
    orientation: number
  }

  export type WechatSupport = {
    
    isSupport: boolean
  }

  export type MiniApp = {
    
    miniAppId: string
    
    path: string
    
    miniProgramType: number
  }

  export type Result = {
    
    result: boolean
  }

  export type Call = {
    
    targetId: string
    
    timeout: number
    
    extra: Record<string, any>
  }

  
  interface GetAsrListenerManagerTask {
    
    getAsrActive(params: {
      complete?: () => void
      success?: (params: {
        
        isActive: boolean
      }) => void
      failure?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
    }): void

    
    stopDetect(params: {
      complete?: () => void
      success?: (params: null) => void
      fail?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
    }): void

    
    startDetect(params: {
      complete?: () => void
      success?: (params: null) => void
      fail?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
    }): void

    
    onDetect(
      listener: (params: {
        
        managerId: number
        
        state: number
        
        text: string
        
        errorCode: number
      }) => void
    ): void

    
    offDetect(
      listener: (params: {
        
        managerId: number
        
        state: number
        
        text: string
        
        errorCode: number
      }) => void
    ): void
  }
  export function getAsrListenerManager(params: {
    
    homeId: string
    
    sampleRate: number
    
    channels: number
    
    codec: string
    
    options: string
    complete?: () => void
    success?: (params: null) => void
    failure?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): GetAsrListenerManagerTask

  
  interface GetLogManagerTask {
    
    log(params: {
      
      message: string
      complete?: () => void
      success?: (params: null) => void
      failure?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
    }): void

    
    error(params: {
      
      message: string
      complete?: () => void
      success?: (params: null) => void
      failure?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
    }): void

    
    feedback(params: {
      
      message: string
      complete?: () => void
      success?: (params: null) => void
      failure?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
    }): void

    
    debug(params: {
      
      message: string
      complete?: () => void
      success?: (params: null) => void
      failure?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
    }): void
  }
  export function getLogManager(params: {
    
    tag: string
    complete?: () => void
    success?: (params: null) => void
    failure?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): GetLogManagerTask

  
  interface RegisterChannelTask {
    
    unRegisterChannel(params: {
      complete?: () => void
      success?: (params: null) => void
      fail?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
    }): void
  }
  export function registerChannel(params: {
    
    eventName: string
    complete?: () => void
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): RegisterChannelTask
}
