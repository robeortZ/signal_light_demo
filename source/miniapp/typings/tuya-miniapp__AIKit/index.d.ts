
declare namespace ty.ai {
  
  export function createForegroundVideoService(params?: {
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function destroyForegroundVideoService(params?: {
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function downloadFaceLandmarkerModel(params?: {
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function detectFaceLandmarks(params: {
    
    path: string
    
    extendParam?: Object
    success?: (params: {
      
      faceLandmarks: FaceLandmark[]
      
      extData?: Object
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function processForegroundMedia(params: {
    
    sources: ForegroundMediaSource[]
    
    outputConfig: OutputConfig
    
    extendParam?: Object
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function processForegroundMediaByTemplate(params: {
    
    templateObject: ForegroundMediaTemplateObject
    
    mediaSource: string
    
    outputConfig?: OutputConfig
    
    extendParam?: Object
    success?: (params: {
      
      outputPath: string
      
      extData?: Object
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function getTranslateRecords(params?: {
    
    deviceId?: string
    
    lastId?: number
    
    pageSize?: number
    success?: (params: {
      
      list?: TranslateRecord[]
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function getTranslateRecord(params: {
    
    translateId: number
    success?: (params: {
      
      translateId: number
      
      deviceId: string
      
      originalLanguage?: string
      
      targetLanguage?: string
      
      recordId?: string
      
      agentId?: string
      
      name: string
      
      beginAt: number
      
      endAt: number
      
      duration: number
      
      visit: boolean
      
      remove: boolean
      
      wavFilePath?: string
      
      summaryStatus: number
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function updateTranslateRecord(params: {
    
    translateId: number
    
    name?: string
    
    summaryStatus?: string
    
    visit?: string
    
    remove?: string
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function removeTranslateRecord(params: {
    
    translateId: number
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function getTranslateRealTimeResult(params: {
    
    translateId: string
    success?: (params: {
      
      list?: TranslateRealTimeResult[]
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function processTranslateSummary(params: {
    
    translateId: number
    
    template: string
    
    language: string
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function getTranslateSummaryProcessStatus(params: {
    
    deviceId: string
    
    translateIds: string[]
    success?: (params: {
      
      success: string[]
      
      fail: string[]
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function getTranslateSummary(params: {
    
    translateId: number
    success?: (params: {
      
      text: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function backgroundMusicList(params?: {
    success?: (params: {
      
      musicList?: MusicModel[]
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function backgroundMusicDownload(params: {
    
    musicUrl: string
    
    musicPath: string
    success?: (params: {
      
      musicPath: string
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function objectDetectCreate(params?: {
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function objectDetectDestroy(params?: {
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function objectDetectForVideo(params: {
    
    inputVideoPath: string
    
    outputVideoPath: string
    
    videoConfig?: VideoConfig
    
    detectType?: DetectType
    
    imageEditType?: ImageEditType
    
    musicPath: string
    
    audioEditType?: AudioEditType
    
    originAudioVolume?: number
    
    overlayAudioVolume?: number
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
    complete?: () => void
  }): void

  
  export function objectDetectForVideoCancel(params?: {
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function objectDetectForImage(params: {
    
    inputPath: string
    
    outputPath: string
    
    detectType?: DetectType
    
    imageEditType?: ImageEditType
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
    complete?: () => void
  }): void

  
  export function privacyProtectDetectForVideo(params: {
    
    inputVideoPath: string
    
    outputVideoPath: string
    
    videoConfig?: VideoConfig
    
    detectType?: DetectType
    
    imageEditType?: ImageEditType
    
    musicPath: string
    
    audioEditType?: AudioEditType
    
    originAudioVolume?: number
    
    overlayAudioVolume?: number
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
    complete?: () => void
  }): void

  
  export function privacyProtectDetectForImage(params: {
    
    inputPath: string
    
    outputPath: string
    
    detectType?: DetectType
    
    imageEditType?: ImageEditType
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
    complete?: () => void
  }): void

  
  export function objectDetectForImageCancel(params?: {
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function petsDetectCreate(params?: {
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function petsDetectDestory(params?: {
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function petsPictureQualityDetectForImage(params: {
    
    inputImagePath: string
    
    labelAllow?: any
    
    objectAreaPercent: number
    
    objectFaceRotationAngle: number
    
    objectFaceSideAngle: number
    
    maximumPictureBrightness: number
    
    minimumPictureBrightness: number
    success?: (params: {
      
      imagePath: string
      
      lowQuality: boolean
      
      lowQualityReason: number
    }) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): void

  
  export function onVideoObjectDetectProgress(
    listener: (params: DetectProgressEvent) => void
  ): void

  
  export function offVideoObjectDetectProgress(
    listener: (params: DetectProgressEvent) => void
  ): void

  
  export function onPetsDetectProgress(
    listener: (params: DetectProgressEvent) => void
  ): void

  
  export function offPetsDetectProgress(
    listener: (params: DetectProgressEvent) => void
  ): void

  export type Object = {}

  export type FaceLandmark = {
    
    faceLandmarks: NormalizedLandmark[]
    
    extData?: Object
  }

  export type ForegroundMediaSource = {
    
    type: string
    
    path: string
    
    identifier: string
    
    loop?: boolean
    
    x?: number
    
    y?: number
    
    z_index?: number
    
    filter_chain?: string
    
    extendParam?: Object
  }

  export type OutputConfig = {
    
    path?: string
    
    name?: string
    
    min_duration?: number
    
    extendParam?: Object
  }

  export type ForegroundMediaTemplateObject = {
    
    type: string
    
    effect: ForegroundMediaTemplateEffectObject
    
    extendParam?: Object
  }

  export type TranslateRecord = {
    
    translateId: number
    
    deviceId: string
    
    originalLanguage?: string
    
    targetLanguage?: string
    
    recordId?: string
    
    agentId?: string
    
    name: string
    
    beginAt: number
    
    endAt: number
    
    duration: number
    
    visit: boolean
    
    remove: boolean
    
    wavFilePath?: string
    
    summaryStatus: number
  }

  export type TranslateRealTimeResult = {
    
    asrId: number
    
    translateId: number
    
    requestId: string
    
    recordId: string
    
    beginTime: number
    
    endTime: number
    
    text: string
    
    asr: string
    
    channel: number
  }

  export type MusicModel = {
    
    musicTitle: string
    
    musicUrl: string
  }

  export enum VideoConfig {
    
    Level_480 = 1,

    
    Level_540 = 2,

    
    Level_720 = 3,

    
    Level_1080 = 4,
  }

  export enum DetectType {
    
    VideoDetectMainBodyTypePet = 1,

    
    VideoDetectMainBodyTypePerson = 2,
  }

  export enum ImageEditType {
    
    ImageEditTypeNoEffect = 1,

    
    ImageEditTypeHighlight = 2,

    
    ImageEditTypeMosaic = 3,
  }

  export enum AudioEditType {
    
    audioEditTypeNULL = 1,

    
    audioEditTypeMix = 2,

    
    audioEditTypeMute = 3,
  }

  export type DetectProgressEvent = {
    
    progress: number
  }

  export type NormalizedLandmark = {
    x: number
    y: number
    z: number
    visibility?: number
    presence?: number
    
    extData?: Object
  }

  export type ForegroundMediaTemplateEffectObject = {
    
    code: string
    
    name: string
    
    ouputDuration: number
    
    image: string
    
    resource: string
    
    extendParam?: Object
  }

  export type GenerateTranslateTaskParams = {
    
    contextId?: string
    
    deviceId: string
    
    dataTimeout: number
    
    originalLanguage: string
    
    targetLanguage: string
    
    agentId: string
  }

  export type StartSpeakParams = {
    
    contextId: string
    
    channel: number
  }

  export type StopSpeakParams = {
    
    contextId: string
  }

  export type DisposeParams = {
    
    contextId: string
  }

  export type TranslateEvent = {
    
    contextId: string
  }

  export type GetTranslateRecordsParams = {
    
    deviceId?: string
    
    lastId?: number
    
    pageSize?: number
  }

  export type GetTranslateRecordsResponse = {
    
    list?: TranslateRecord[]
  }

  export type GetTranslateRecordParams = {
    
    translateId: number
  }

  export type UpdateTranslateRecordParams = {
    
    translateId: number
    
    name?: string
    
    summaryStatus?: string
    
    visit?: string
    
    remove?: string
  }

  export type RemoveTranslateRecordParams = {
    
    translateId: number
  }

  export type GetTranslateRealTimeResultParams = {
    
    translateId: string
  }

  export type GetTranslateRealTimeResultResponse = {
    
    list?: TranslateRealTimeResult[]
  }

  export type ProcessTranslateSummaryParams = {
    
    translateId: number
    
    template: string
    
    language: string
  }

  export type GetTranslateSummaryProcessStatusParams = {
    
    deviceId: string
    
    translateIds: string[]
  }

  export type GetTranslateSummaryProcessStatusResponse = {
    
    success: string[]
    
    fail: string[]
  }

  export type GetTranslateSummaryParams = {
    
    translateId: number
  }

  export type GetTranslateSummaryResponse = {
    
    text: string
  }

  export type MusicModelList = {
    
    musicList?: MusicModel[]
  }

  export type MusicParams = {
    
    musicUrl: string
    
    musicPath: string
  }

  export type MusicResponse = {
    
    musicPath: string
  }

  export type DetectVideoParams = {
    
    inputVideoPath: string
    
    outputVideoPath: string
    
    videoConfig?: VideoConfig
    
    detectType?: DetectType
    
    imageEditType?: ImageEditType
    
    musicPath: string
    
    audioEditType?: AudioEditType
    
    originAudioVolume?: number
    
    overlayAudioVolume?: number
  }

  export type DetectVideoResponse = {
    
    path: string
  }

  export type DetectImageParams = {
    
    inputPath: string
    
    outputPath: string
    
    detectType?: DetectType
    
    imageEditType?: ImageEditType
  }

  export type DetectImageResponse = {
    
    path: string
  }

  export type DetectParams = {
    
    inputImagePath: string
    
    labelAllow?: any
    
    objectAreaPercent: number
    
    objectFaceRotationAngle: number
    
    objectFaceSideAngle: number
    
    maximumPictureBrightness: number
    
    minimumPictureBrightness: number
  }

  export type DetectResult = {
    
    imagePath: string
    
    lowQuality: boolean
    
    lowQualityReason: number
  }

  
  interface TranslateContext {
    
    startSpeak(params: {
      
      channel: number
      success?: (params: null) => void
      fail?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
      complete?: () => void
    }): void

    
    stopSpeak(params: {
      success?: (params: null) => void
      fail?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
      complete?: () => void
    }): void

    
    dispose(params: {
      success?: (params: null) => void
      fail?: (params: {
        errorMsg: string
        errorCode: string | number
        innerError: {
          errorCode: string | number
          errorMsg: string
        }
      }) => void
      complete?: () => void
    }): void

    
    onTranslateError(
      listener: (params: {
        
        code: number
        
        message?: string
      }) => void
    ): void

    
    offTranslateError(
      listener: (params: {
        
        code: number
        
        message?: string
      }) => void
    ): void

    
    onTranslateRealTimeStatusUpdate(
      listener: (params: {
        
        recordId: string
        
        requestId: string
        
        asrId: number
        
        channel: number
        
        phase: number
        
        status: number
        
        text?: string
        
        errorCode?: number
        
        errorMessage?: string
      }) => void
    ): void

    
    offTranslateRealTimeStatusUpdate(
      listener: (params: {
        
        recordId: string
        
        requestId: string
        
        asrId: number
        
        channel: number
        
        phase: number
        
        status: number
        
        text?: string
        
        errorCode?: number
        
        errorMessage?: string
      }) => void
    ): void
  }
  
  export function generateTranslateTask(params: {
    
    deviceId: string
    
    dataTimeout: number
    
    originalLanguage: string
    
    targetLanguage: string
    
    agentId: string
    success?: (params: null) => void
    fail?: (params: {
      errorMsg: string
      errorCode: string | number
      innerError: {
        errorCode: string | number
        errorMsg: string
      }
    }) => void
    complete?: () => void
  }): TranslateContext
}
