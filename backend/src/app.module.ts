import { Module } from '@nestjs/common';
import { AppController } from './app.controller';

@Module({
  imports: [],
  controllers: [AppController], 
  providers: [], // IMPORTANT: This must be empty (No AppGateway)
})
export class AppModule {}