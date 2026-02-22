import { useEffect, useState } from 'react';
import { MapContainer, TileLayer, Marker, Popup } from 'react-leaflet';
import { io } from 'socket.io-client';
import { Battery, Activity, ShieldAlert, HeartPulse, MapPin, CheckCircle } from 'lucide-react';
import 'leaflet/dist/leaflet.css';

// Fix Leaflet Icons
import L from 'leaflet';
import icon from 'leaflet/dist/images/marker-icon.png';
import iconShadow from 'leaflet/dist/images/marker-shadow.png';
const DefaultIcon = L.icon({
  iconUrl: icon, shadowUrl: iconShadow,
  iconSize: [25, 41], iconAnchor: [12, 41]
});
L.Marker.prototype.options.icon = DefaultIcon;

// Connect to the NestJS WebSocket Gateway
const socket = io('http://localhost:3000');

function App() {
  const [activeAlert, setActiveAlert] = useState<any>(null);
  const [telemetry, setTelemetry] = useState<any>({ steps: 0, batteryLevel: '--', batteryVoltage: '--', isCharging: false });
  const [position, setPosition] = useState<{lat: number, lng: number}>({ lat: 10.9027, lng: 76.9006 }); // Default: Coimbatore

  useEffect(() => {
    // 1. Listen for Live Telemetry Heartbeats
    socket.on('new-telemetry', (data) => {
      console.log("📡 Live Telemetry:", data);
      setTelemetry(data);
    });

    // 2. Listen for Live Panic Alerts
    socket.on('new-panic-alert', (data) => {
      console.log("🚨 LIVE SOS:", data);
      setActiveAlert(data);
      
      // Parse coordinates from Google Maps link if available
      if (data.location && data.location.includes(',')) {
        const parts = data.location.split('1'); // Extracting from our C++ payload format
        if (parts.length > 1) {
            const coords = parts[1].split(',');
            if (coords.length === 2) {
                setPosition({ lat: parseFloat(coords[0]), lng: parseFloat(coords[1]) });
            }
        }
      }
    });

    return () => {
      socket.off('new-telemetry');
      socket.off('new-panic-alert');
    };
  }, []);

  const dismissAlert = () => setActiveAlert(null);

  return (
    <div className="min-h-screen bg-slate-50 text-slate-800 font-sans flex flex-col relative overflow-hidden">
      
      {/* HEADER */}
      <header className="bg-white border-b border-slate-200 px-6 py-4 flex justify-between items-center shadow-sm z-10">
        <div>
          <h1 className="text-2xl font-bold tracking-tight text-slate-900">Jaswanth Saravanan CB.SC.U4AIE24324</h1>
          <p className="text-sm text-slate-500 font-medium">Nesso N1 Command Center</p>
        </div>
        <div className="flex items-center gap-2 bg-emerald-50 text-emerald-700 px-4 py-2 rounded-full border border-emerald-200 shadow-sm">
          <CheckCircle size={18} className="animate-pulse" />
          <span className="font-semibold text-sm">System Active</span>
        </div>
      </header>

      {/* MAIN DASHBOARD LAYOUT */}
      <main className="flex-1 p-6 grid grid-cols-1 lg:grid-cols-4 gap-6 z-10">
        
        {/* LEFT COLUMN: TELEMETRY CARDS */}
        <div className="lg:col-span-1 flex flex-col gap-4">
          
          <div className="bg-white p-5 rounded-xl border border-slate-200 shadow-sm flex items-center gap-4">
            <div className="p-3 bg-blue-50 text-blue-600 rounded-lg"><Activity size={28} /></div>
            <div>
              <p className="text-sm text-slate-500 font-semibold uppercase tracking-wider">Step Count</p>
              <p className="text-3xl font-bold text-slate-800">{telemetry.steps}</p>
            </div>
          </div>

          <div className="bg-white p-5 rounded-xl border border-slate-200 shadow-sm flex items-center gap-4">
            <div className="p-3 bg-green-50 text-green-600 rounded-lg"><Battery size={28} /></div>
            <div>
              <p className="text-sm text-slate-500 font-semibold uppercase tracking-wider">Watch Battery</p>
              <div className="flex items-baseline gap-2">
                <p className="text-3xl font-bold text-slate-800">{telemetry.batteryLevel}%</p>
                <p className="text-sm text-slate-400">({telemetry.batteryVoltage}v)</p>
              </div>
              <p className={`text-xs font-bold mt-1 ${telemetry.isCharging ? 'text-green-600' : 'text-slate-400'}`}>
                {telemetry.isCharging ? '⚡ CHARGING' : 'DISCHARGING'}
              </p>
            </div>
          </div>

          <div className="bg-white p-5 rounded-xl border border-slate-200 shadow-sm flex items-center gap-4 opacity-70">
            <div className="p-3 bg-purple-50 text-purple-600 rounded-lg"><HeartPulse size={28} /></div>
            <div>
              <p className="text-sm text-slate-500 font-semibold uppercase tracking-wider">Dress Pressure</p>
              <p className="text-xl font-bold text-slate-800">Tracking...</p>
              <p className="text-xs text-slate-400 mt-1">Velostat Array</p>
            </div>
          </div>

        </div>

        {/* RIGHT COLUMN: LIVE MAP */}
        <div className="lg:col-span-3 bg-white rounded-xl border border-slate-200 shadow-sm overflow-hidden flex flex-col relative min-h-[500px]">
          <div className="px-4 py-3 border-b border-slate-100 flex items-center gap-2 bg-slate-50/50">
            <MapPin size={18} className="text-slate-500" />
            <h2 className="font-semibold text-slate-700">Live GPS / LBS Tracking</h2>
          </div>
          <div className="flex-1 w-full relative z-0">
            <MapContainer center={[position.lat, position.lng]} zoom={13} style={{ height: '100%', width: '100%' }}>
              <TileLayer url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png" />
              <Marker position={[position.lat, position.lng]}>
                <Popup>
                  <strong className="text-slate-800">Wearer Location</strong><br/>
                  Last Updated: Just now
                </Popup>
              </Marker>
            </MapContainer>
          </div>
        </div>

      </main>

      {/* 🔴 MASSIVE SOS OVERRIDE MODAL 🔴 */}
      {activeAlert && (
        <div className="absolute inset-0 z-50 flex items-center justify-center p-4 bg-red-900/90 backdrop-blur-sm animate-in fade-in duration-200">
          <div className="bg-white rounded-2xl shadow-2xl max-w-lg w-full overflow-hidden border-4 border-red-500 animate-in zoom-in-95 duration-200">
            <div className="bg-red-600 p-6 text-center text-white flex flex-col items-center">
              <ShieldAlert size={64} className="animate-bounce mb-2" />
              <h2 className="text-4xl font-black tracking-tight">SOS TRIGGERED</h2>
            </div>
            <div className="p-8 flex flex-col gap-4">
              <div className="bg-red-50 p-4 rounded-lg border border-red-100">
                <p className="text-sm text-red-600 font-bold uppercase tracking-wider mb-1">Trigger Source</p>
                <p className="text-2xl font-black text-red-900">{activeAlert.source}</p>
              </div>
              <div className="grid grid-cols-2 gap-4">
                <div className="bg-slate-50 p-4 rounded-lg border border-slate-100">
                  <p className="text-xs text-slate-500 font-bold uppercase">Device ID</p>
                  <p className="text-lg font-bold text-slate-800">{activeAlert.deviceId}</p>
                </div>
                <div className="bg-slate-50 p-4 rounded-lg border border-slate-100">
                  <p className="text-xs text-slate-500 font-bold uppercase">Watch Battery</p>
                  <p className="text-lg font-bold text-slate-800">{activeAlert.battery}%</p>
                </div>
              </div>
              <p className="text-sm text-slate-500 break-all mt-2"><strong>Raw Location Data:</strong> {activeAlert.location}</p>
              <button 
                onClick={dismissAlert}
                className="mt-6 w-full bg-slate-800 hover:bg-slate-900 text-white font-bold py-4 rounded-xl transition-colors shadow-lg active:scale-[0.98]"
              >
                Acknowledge & Dismiss Alert
              </button>
            </div>
          </div>
        </div>
      )}

    </div>
  );
}

export default App;