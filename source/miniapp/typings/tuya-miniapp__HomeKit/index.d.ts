
declare namespace ty.home {
  
  export function getLocalDeviceConfigWithDevId(params: {
    
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

  
  export function updateHomeInfoData(params: {
    
    homeName: string
    
    homeId: string
    
    longitude: string
    
    latitude: string
    
    address: string
    
    admin: boolean
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

  
  export function getCurrentHomeInfo(params?: {
    complete?: () => void
    success?: (params: {
      
      homeName: string
      
      homeId: string
      
      longitude: string
      
      latitude: string
      
      address: string
      
      admin: boolean
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

  
  export function getDeviceRoomInfo(params: {
    
    deviceId: string
    complete?: () => void
    success?: (params: {
      
      roomId: number
      
      name: string
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

  
  export function getGroupRoomInfo(params: {
    
    groupId: string
    complete?: () => void
    success?: (params: {
      
      roomId: number
      
      name: string
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

  
  export function getDeviceIdList(params: {
    
    ownerId: number
    complete?: () => void
    success?: (params: {
      
      devIds: string[]
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

  
  export function getRoomList(params: {
    
    ownerId: number
    complete?: () => void
    success?: (params: {
      
      roomDatas: RoomData[]
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

  
  export function onTicketSuccess(params?: {
    
    map?: Record<string, string>
    complete?: () => void
    success?: (params: {
      
      map?: Record<string, string>
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

  
  export function openRecommendSceneDetail(params: {
    
    source: string
    
    sceneModel: Record<string, any>
    complete?: () => void
    success?: (params: {
      
      status?: boolean
      
      type: number
      
      data?: Record<string, any>
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

  
  export function openDeviceExecutionAndAnutomation(params: {
    
    deviceId: string
    
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

  export type RoomData = {
    
    roomId: number
    
    name: string
    
    deviceIds: string[]
  }

  export type HomeInfoData = {
    
    homeName: string
    
    homeId: string
    
    longitude: string
    
    latitude: string
    
    address: string
    
    admin: boolean
  }

  export type GetDeviceRoomInfoParams = {
    
    deviceId: string
  }

  export type GetDeviceRoomInfoResponse = {
    
    roomId: number
    
    name: string
  }

  export type GroupRoomInfoParams = {
    
    groupId: string
  }

  export type GroupRoomInfoResponse = {
    
    roomId: number
    
    name: string
  }

  export type OwnerId = {
    
    ownerId: number
  }

  export type DeviceIdList = {
    
    devIds: string[]
  }

  export type RoomList = {
    
    roomDatas: RoomData[]
  }

  export type TicketModel = {
    
    map?: Record<string, string>
  }

  export type RecommendSceneParams = {
    
    source: string
    
    sceneModel: Record<string, any>
  }

  export type RecommendSceneCallBack = {
    
    status?: boolean
    
    type: number
    
    data?: Record<string, any>
  }

  export type OpenDeviceExecutionAndAnutomationParams = {
    
    deviceId: string
    
    title?: string
  }
}
