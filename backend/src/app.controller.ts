import { Controller, Get, Post, Body } from '@nestjs/common';
import { AppGateway } from './app.gateway';

@Controller('api')
export class AppController {
  
  // In-Memory Storage (We will upgrade to a real DB later)
  private alerts: any[] = [];
  private latestTelemetry: any = {};

  // Inject the WebSocket Gateway
  constructor(private readonly appGateway: AppGateway) {}

  // --- 1. PANIC ALERTS ENDPOINT ---
  @Post('alert')
  receiveAlert(@Body() data: any) {
    console.log('\n🚨 [URGENT] PANIC ALERT RECEIVED:', data);
    
    const newAlert = {
      ...data,
      timestamp: new Date(),
      id: Date.now().toString() 
    };
    
    this.alerts.unshift(newAlert); // Save to local memory

    // INSTANTLY PUSH TO REACT DASHBOARD
    this.appGateway.pushPanicAlert(newAlert);
    
    return { status: 'Alert Received and Broadcasted', success: true };
  }

  // --- 2. TELEMETRY HEARTBEAT ENDPOINT ---
  @Post('telemetry')
  receiveTelemetry(@Body() data: any) {
    console.log('📡 [HEARTBEAT] Telemetry Received:', data);
    
    const telemetryData = {
      ...data,
      timestamp: new Date()
    };

    this.latestTelemetry = telemetryData; // Update the latest state

    // INSTANTLY PUSH TO REACT DASHBOARD
    this.appGateway.pushTelemetry(telemetryData);

    return { status: 'Telemetry Received', success: true };
  }

  // --- GETTER ENDPOINTS (For when React first loads) ---
  
  @Get('alerts')
  getAllAlerts() {
    return this.alerts;
  }

  @Get('telemetry')
  getLatestTelemetry() {
    return this.latestTelemetry;
  }
}