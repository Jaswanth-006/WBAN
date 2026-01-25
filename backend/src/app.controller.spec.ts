import { Controller, Get, Post, Body } from '@nestjs/common';

@Controller('api/alert')
export class AppController {

  // Memory Storage
  private alerts: any[] = [];

  // 1. RECEIVE (POST)
  @Post()
  receiveAlert(@Body() data: any) {
    console.log('🚨 ALERT RECEIVED:', data);
    
    // Create alert object
    const newAlert = {
      ...data,
      timestamp: new Date(),
      id: this.alerts.length + 1
    };
    
    // Save to list
    this.alerts.unshift(newAlert);
    
    return { status: 'Received', saved: true };
  }

  // 2. SEND (GET)
  @Get()
  sendAlerts() {
    return this.alerts;
  }
}