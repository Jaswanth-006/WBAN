import { Controller, Post, Body, Get } from '@nestjs/common';

// FIX 1: SINGULAR NAME 'api/alert' (No 's')
@Controller('api/alert') 
export class AlertsController {
  
  private alerts: any[] = [];

  // RECEIVE DATA (POST)
  @Post()
  createAlert(@Body() data: any) {
    console.log('🚨 ALERT RECEIVED:', data);
    
    const newAlert = {
      ...data,
      timestamp: new Date(),
      id: this.alerts.length + 1
    };
    this.alerts.unshift(newAlert); 
    return { status: 'Received', saved: true };
  }

  // SEND DATA (GET)
  @Get()
  getAllAlerts() {
    return this.alerts;
  }
}