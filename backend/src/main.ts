// src/main.ts
import { NestFactory } from '@nestjs/core';
import { AppModule } from './app.module';

async function bootstrap() {
  const app = await NestFactory.create(AppModule);
  
  // Enable CORS so React can talk to this server
  app.enableCors();
  
  // Start on Port 3000
  await app.listen(3000);
  console.log(`🚀 Server is running on http://localhost:3000`);
}
bootstrap();