import io from 'socket.io-client'

let socket: any | null

export const initiateSocket = (uri: string): void => {
  socket = io(uri)
}

export const disconnectSocket = (): void => {
  if (socket) {
    socket.disconnect()
  }
}

export const subscribeTo = (event: string, callback: (eventName: string, data: object) => void): void => {
  if (socket) {
    socket.on(event, (eventName: string | object, data?: object) => {
      if (data) {
        callback(eventName as string, data)
      } else {
        callback(event, eventName as object)
      }
    })
  }
}

export const sendData = (eventName: string, data: object): void => {
  if (socket) {
    socket.emit(eventName, data)
  }
}
