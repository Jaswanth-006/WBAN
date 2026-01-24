// frontend/src/App.tsx
import { useEffect, useState } from 'react';
import { MapContainer, TileLayer, Marker, Popup } from 'react-leaflet';
import io from 'socket.io-client';
import 'leaflet/dist/leaflet.css'; // Import map styles
import './App.css';

// 1. Connect to your NestJS Backend
const socket = io('http://localhost:3000'); 

interface AlertData {
  lat: number;
  lng: number;
  deviceId: string;
}

function App() {
  const [alert, setAlert] = useState<AlertData | null>(null);

  useEffect(() => {
    // 2. Listen for 'panic-alert' event from Backend
    socket.on('panic-alert', (data: AlertData) => {
      console.log("🚨 ALARM RECEIVED:", data);
      setAlert(data);
      
      // Optional: Browser Alert Sound
      // const audio = new Audio('https://www.soundjay.com/buttons/beep-01a.mp3');
      // audio.play();
    });

    return () => {
      socket.off('panic-alert');
    };
  }, []);

  // Default Location (e.g., Your College or City Center)
  const defaultPosition = { lat: 11.0168, lng: 76.9558 }; // Coimbatore

  return (
    <div className="dashboard">
      {/* Header Changes Color on Alert */}
      <div className={`header ${alert ? 'danger' : 'safe'}`}>
        {alert ? `⚠️ SOS TRIGGERED: ${alert.deviceId}` : "✅ System Active & Monitoring"}
      </div>

      <div className="map-container">
        <MapContainer 
          center={[alert ? alert.lat : defaultPosition.lat, alert ? alert.lng : defaultPosition.lng]} 
          zoom={15} 
          scrollWheelZoom={true}
        >
          {/* Load OpenStreetMap Tiles */}
          <TileLayer
            attribution='&copy; OpenStreetMap contributors'
            url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
          />

          {/* If Alert exists, show Red Marker */}
          {alert && (
            <Marker position={[alert.lat, alert.lng]}>
              <Popup>
                <strong>HELP NEEDED HERE!</strong> <br />
                Device: {alert.deviceId} <br />
                Coordinates: {alert.lat}, {alert.lng}
              </Popup>
            </Marker>
          )}
        </MapContainer>
      </div>
    </div>
  );
}

export default App;