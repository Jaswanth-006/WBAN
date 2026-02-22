import { WebSocketGateway, WebSocketServer, SubscribeMessage, MessageBody } from '@nestjs/websockets';
import { Server, Socket } from 'socket.io';

@WebSocketGateway({
  cors: {
    origin: '*', // Allows React to connect
  },
})
export class AppGateway {
  @WebSocketServer()
  server: Server;

  // Called when a React client connects
  handleConnection(client: Socket) {
    console.log(`💻 Client connected to Dashboard: ${client.id}`);
  }

  // --- Push Functions (Called by the Controller) ---
  
  // Pushes Panic Alerts instantly
  pushPanicAlert(data: any) {
    this.server.emit('new-panic-alert', data);
  }

  // Pushes the 5-minute Heartbeat telemetry instantly
  pushTelemetry(data: any) {
    this.server.emit('new-telemetry', data);
  }

  // --- Receive Functions (Called by React) ---
  
  // React can send a "Are you safe?" message down to the watch (Future Feature)
  @SubscribeMessage('send-watch-message')
  handleWatchMessage(@MessageBody() data: any) {
    console.log('✉️ Sending message to Nesso N1:', data);
    // In the future, we will use MQTT or a similar protocol to push this down to the ESP32.
    // For now, we just log it.
  }
}