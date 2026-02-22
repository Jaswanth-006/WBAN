import { Module } from '@nestjs/common';
import { AppController } from './app.controller';
import { AppGateway } from './app.gateway';

@Module({
  imports: [],
  controllers: [AppController],
  providers: [AppGateway], // IMPORTANT: We must register the Gateway here!
})
export class AppModule {}