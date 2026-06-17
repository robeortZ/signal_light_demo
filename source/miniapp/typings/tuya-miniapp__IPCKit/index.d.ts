
declare namespace ty.ipc {
  
  export function generateSignedUrl(params: {
    
    path: string
    
    expiration: string
    
    region: string
    
    token: string
    
    sk: string
    
    provider: string
    
    endpoint: string
    
    ak: string
    
    bucket: string
    complete?: () => void
    success?: (params: {
      
      signedUrl: string
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

  
  export function enableAudioNS(params: {
    
    deviceId: string
    
    enable: boolean
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

  
  export function enableAudioAGC(params: {
    
    deviceId: string
    
    enable: boolean
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

  
  export function enableAudioAEC(params: {
    
    deviceId: string
    
    enable: boolean
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

  
  export function obtainCameraConfig(params: {
    
    deviceId: string
    complete?: () => void
    success?: (params: {
      
      supportedAudioMode: number
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

  
  export function startIdentifyingBirds(params: {
    
    identifier: string
    
    deviceId: string
    complete?: () => void
    success?: (params: {
      
      result: Object
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

  
  export function cancelIdentifyingBirds(params: {
    
    identifier: string
    
    deviceId: string
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

  
  export function isSupportAICloud(params?: {
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

  
  export function isSupportBirdFeeder(params?: {
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

  
  export function isSupportFullScreen(params?: {
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

  
  export function downloadCloudPlayBack(params: {
    
    deviceId: string
    
    url: string
    
    path: string
    
    prefix: number
    
    videoSegments: number[]
    
    videoSegmentSize: number
    complete?: () => void
    success?: (params: { path: string }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
  }): void

  
  export function cancelDownloadCloudPlayBack(params: {
    
    deviceId: string
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

  
  export function gotoCameraSettingsRouter(params: {
    
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

  
  export function couldChangeTalkbackMode(params: {
    
    devId: string
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

  
  export function doorbellCallConfig(params?: {
    
    ignoreWhenCalling?: boolean
    
    doorbellRingTimeOut?: number
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

  
  export function acceptDoorbellCall(params: {
    
    messageId: string
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

  
  export function refuseDoorbellCall(params: {
    
    messageId: string
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

  
  export function hangupDoorbellCall(params: {
    
    messageId: string
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

  
  export function isSupportedCustomDefaultAudioConfig(params?: {
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

  
  export function onCloudPlayBackDownloadProgress(
    listener: (params: cloudPlayBackDownloadProgressBody) => void
  ): void

  
  export function offCloudPlayBackDownloadProgress(
    listener: (params: cloudPlayBackDownloadProgressBody) => void
  ): void

  
  export function onDoorBellCallHangUp(
    listener: (params: DoorbellCallEventResponse) => void
  ): void

  
  export function offDoorBellCallHangUp(
    listener: (params: DoorbellCallEventResponse) => void
  ): void

  
  export function onDoorBellCallHangUpByOther(
    listener: (params: DoorbellCallEventResponse) => void
  ): void

  
  export function offDoorBellCallHangUpByOther(
    listener: (params: DoorbellCallEventResponse) => void
  ): void

  
  export function onDoorBellCallCancel(
    listener: (params: DoorbellCallEventResponse) => void
  ): void

  
  export function offDoorBellCallCancel(
    listener: (params: DoorbellCallEventResponse) => void
  ): void

  export type Object = {}

  export type cloudPlayBackDownloadProgressBody = {
    
    progress: number
  }

  export type DoorbellCallEventResponse = {
    
    response?: any
  }

  export type EnableAudioNSParams = {
    
    deviceId: string
    
    enable: boolean
  }

  export type EnableAudioAGCParams = {
    
    deviceId: string
    
    enable: boolean
  }

  export type EnableAudioAECParams = {
    
    deviceId: string
    
    enable: boolean
  }

  export type CameraConfigParams = {
    
    deviceId: string
  }

  export type CameraConfig = {
    
    supportedAudioMode: number
  }

  export type IdentifyingBirdsParams = {
    
    identifier: string
    
    deviceId: string
  }

  export type RecognizeBirdSuccessModel = {
    
    result: Object
  }

  export type DownloadCloudVideoParams = {
    
    deviceId: string
    
    url: string
    
    path: string
    
    prefix: number
    
    videoSegments: number[]
    
    videoSegmentSize: number
  }

  export type CloudPlayBackDownloadSuccessModel = {
    path: string
  }

  export type RouterParamsBean = {
    
    url: string
  }

  export type DeviceParamsBean = {
    
    devId: string
  }

  export type DoorbellConfigParams = {
    
    ignoreWhenCalling?: boolean
    
    doorbellRingTimeOut?: number
  }

  export type DoorbellCallParams = {
    
    messageId: string
  }
}
