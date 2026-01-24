// src/app.controller.ts
import { Body, Controller, Get, Post } from '@nestjs/common';
import { AppGateway } from './app.gateway';

@Controller('api') // This prefixes all routes with /api
export class AppController {
  constructor(private readonly gateway: AppGateway) {}

  @Get()
  checkStatus(): string {
    return 'WBAN Server is Online 🟢';
  }

  // The SIM800L sends data here
  @Post('alert')
  receiveAlert(@Body() data: any) {
    console.log('🚨 ALERT RECEIVED:', data);

    // 1. Send to Frontend via WebSocket
    this.gateway.sendPanicAlert(data);

    // 2. Reply to SIM800L
    return { status: 'Received', timestamp: new Date() };
  }
}