import React from 'react';
import { MapContainer, TileLayer, Marker, Popup, useMapEvents, Circle } from 'react-leaflet';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';

delete (L.Icon.Default.prototype as any)._getIconUrl;
L.Icon.Default.mergeOptions({
  iconRetinaUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-icon-2x.png',
  iconUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-icon.png',
  shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-shadow.png',
});

interface Property {
  location: string;
  price: number;
  area: number;
  bedrooms: number;
  bbox: { x_min: number; y_min: number; x_max: number; y_max: number; };
}

interface InteractiveMapProps {
  properties: Property[];
  selectedLat: number;
  selectedLng: number;
  searchRadiusKm: number;
  onMapClick: (lat: number, lng: number) => void;
}

function MapEvents({ onMapClick }: { onMapClick: (lat: number, lng: number) => void }) {
  useMapEvents({
    click: (e) => {
      onMapClick(e.latlng.lat, e.latlng.lng);
    }
  });
  return null;
}

const formatPrice = (price: number) => {
  if (price >= 1000000) return `$${(price / 1000000).toFixed(1)}M`;
  if (price >= 1000) return `$${(price / 1000).toFixed(0)}k`;
  return `$${price}`;
};

// Dark, sleek custom marker
const createPriceIcon = (price: number) => {
  return L.divIcon({
    className: 'custom-price-marker',
    html: `<div class="bg-neutral-900 border border-brand-500/50 shadow-[0_0_10px_rgba(6,182,212,0.3)] text-xs font-black px-2 py-1 rounded-sm text-brand-50 hover:bg-brand-500 hover:text-black transition-all whitespace-nowrap">${formatPrice(price)}</div>`,
    iconSize: [0, 0],
    iconAnchor: [20, 15],
  });
};

export default function InteractiveMap({ properties, selectedLat, selectedLng, searchRadiusKm, onMapClick }: InteractiveMapProps) {
  const radiusMeters = searchRadiusKm * 1000;

  return (
    <div className="relative w-full h-[500px] rounded-xl overflow-hidden shadow-2xl border border-white/10 z-0">
      <MapContainer 
        center={[40.7128, -74.0060]} 
        zoom={11} 
        style={{ height: '100%', width: '100%', backgroundColor: '#0a0a0a' }}
        className="z-0"
      >
        <TileLayer
          className="dark-map-tiles"
          attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
          url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
        />
        <MapEvents onMapClick={onMapClick} />
        
        {/* Draw a subtle white marker where the user clicked */}
        <Marker 
          position={[selectedLat, selectedLng]} 
          icon={L.divIcon({
            className: 'custom-center-marker',
            html: `<div class="w-4 h-4 bg-white rounded-full border-4 border-brand-500 shadow-[0_0_15px_rgba(6,182,212,0.8)]"></div>`,
            iconSize: [16, 16],
            iconAnchor: [8, 8]
          })}
        >
          <Popup className="dark-popup"><div className="font-bold text-slate-800">Search Center</div></Popup>
        </Marker>

        {/* Draw a circle indicating the search radius with a neon cyan glow */}
        {radiusMeters > 0 && (
          <Circle 
            center={[selectedLat, selectedLng]} 
            radius={radiusMeters} 
            pathOptions={{ color: '#06b6d4', weight: 1, fillColor: '#06b6d4', fillOpacity: 0.05 }}
          />
        )}

        {/* Draw properties */}
        {properties.map((prop, idx) => {
          const lat = (prop.bbox.y_min + prop.bbox.y_max) / 2;
          const lng = (prop.bbox.x_min + prop.bbox.x_max) / 2;
          
          return (
            <Marker key={idx} position={[lat, lng]} icon={createPriceIcon(prop.price)}>
              <Popup className="custom-popup">
                <div className="font-sans">
                  <img 
                    src={`https://images.unsplash.com/photo-1512917774080-9991f1c4c750?auto=format&fit=crop&w=400&q=80&sig=${idx}`} 
                    alt="Property" 
                    className="w-full h-32 object-cover rounded-md mb-2 filter contrast-125"
                  />
                  <h3 className="font-bold text-lg leading-tight mb-1 text-slate-800">{prop.location}</h3>
                  <p className="text-xl font-black text-brand-600 mb-2">${prop.price.toLocaleString()}</p>
                  <div className="flex space-x-3 text-sm text-slate-600">
                    <span><strong>{prop.bedrooms}</strong> Beds</span>
                    <span><strong>{Math.round(prop.area).toLocaleString()}</strong> sqft</span>
                  </div>
                </div>
              </Popup>
            </Marker>
          );
        })}
      </MapContainer>
    </div>
  );
}
