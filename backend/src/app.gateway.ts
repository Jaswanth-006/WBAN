// src/app.gateway.ts
import { WebSocketGateway, WebSocketServer } from '@nestjs/websockets';
import { Server } from 'socket.io';

@WebSocketGateway({
  cors: {
    origin: '*', // Allows your React frontend to connect from anywhere
  },
})
export class AppGateway {
  @WebSocketServer()
  server: Server;

  // This function pushes data to the Dashboard instantly
  sendPanicAlert(data: any) {
    this.server.emit('panic-alert', data);
  }
}