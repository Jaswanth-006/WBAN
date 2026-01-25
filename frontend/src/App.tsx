import { useEffect, useState } from 'react';
import { MapContainer, TileLayer, Marker, Popup } from 'react-leaflet';
import 'leaflet/dist/leaflet.css';
import './App.css';

// Fix Icons
import L from 'leaflet';
import icon from 'leaflet/dist/images/marker-icon.png';
import iconShadow from 'leaflet/dist/images/marker-shadow.png';
const DefaultIcon = L.icon({
    iconUrl: icon, shadowUrl: iconShadow,
    iconSize: [25, 41], iconAnchor: [12, 41]
});
L.Marker.prototype.options.icon = DefaultIcon;

function App() {
  const [activeAlert, setActiveAlert] = useState<any>(null);
  const defaultPosition = { lat: 10.9027, lng: 76.9006 }; 

  const fetchAlerts = async () => {
    try {
      // FIX 5: URL matches the backend (Singular)
      const response = await fetch('http://localhost:3000/api/alert');
      
      if (response.ok) {
        const data = await response.json();
        if (Array.isArray(data) && data.length > 0) {
          console.log("🔥 New Alert Found:", data[0]);
          setActiveAlert(data[0]);
        }
      }
    } catch (error) {
      console.error("Connection Error:", error);
    }
  };

  useEffect(() => {
    fetchAlerts();
    const interval = setInterval(fetchAlerts, 2000); 
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="dashboard">
       <div style={{
         padding: '20px', 
         backgroundColor: activeAlert ? '#ff4d4d' : '#4caf50', 
         color: 'white', textAlign: 'center', fontSize: '24px', fontWeight: 'bold'
       }}>
         {activeAlert ? `⚠️ SOS TRIGGERED: ${activeAlert.deviceId}` : "✅ System Active & Monitoring"}
       </div>

       <div className="map-container" style={{ height: '80vh', width: '100%' }}>
         <MapContainer center={[defaultPosition.lat, defaultPosition.lng]} zoom={13} style={{ height: '100%' }}>
           <TileLayer url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png" />
           {activeAlert && (
             <Marker position={[defaultPosition.lat, defaultPosition.lng]}>
               <Popup>
                 <strong style={{color:'red'}}>SOS ALERT!</strong><br/>
                 Device: {activeAlert.deviceId}<br/>
                 Steps: {activeAlert.steps}<br/>
                 Battery: {activeAlert.battery}%
               </Popup>
             </Marker>
           )}
         </MapContainer>
       </div>
    </div>
  );
}
export default App;