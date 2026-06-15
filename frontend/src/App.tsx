import { useState } from 'react'
import InteractiveMap from './InteractiveMap'

interface Property {
  location: string;
  price: number;
  area: number;
  bedrooms: number;
  distance_km?: number;
  bbox: { x_min: number; y_min: number; x_max: number; y_max: number; };
}

function App() {
  const [properties, setProperties] = useState<Property[]>([]);
  const [loading, setLoading] = useState(false);
  const [loadingMore, setLoadingMore] = useState(false);
  const [searched, setSearched] = useState(false);
  const [total, setTotal] = useState(0);
  const [page, setPage] = useState(1);
  const [hasMore, setHasMore] = useState(false);
  const [queryTime, setQueryTime] = useState<number | null>(null);

  // Default to New York
  const [form, setForm] = useState({
    x: '-74.0060', // Longitude
    y: '40.7128',  // Latitude
    distance: '10', // 10 km radius default
    maxPrice: '1000000',
    minArea: '500',
    minBedrooms: '1'
  });

  const API_BASE = import.meta.env.VITE_API_URL || 'http://localhost:8080';

  const fetchProperties = async (pageNum: number, append: boolean) => {
    const query = new URLSearchParams({
      x: form.x,
      y: form.y,
      distance_km: form.distance,
      max_price: form.maxPrice,
      min_area: form.minArea,
      min_bedrooms: form.minBedrooms,
      page: pageNum.toString(),
      per_page: '50'
    });

    const res = await fetch(`${API_BASE}/api/properties/search?${query.toString()}`);
    const data = await res.json();

    const newProps = data.properties || [];
    if (append) {
      setProperties(prev => [...prev, ...newProps]);
    } else {
      setProperties(newProps);
    }
    setTotal(data.total ?? newProps.length);
    setHasMore(data.has_more ?? false);
    setPage(pageNum);
    setQueryTime(data.query_time_ms ?? null);
  };

  const handleSearch = async (e: React.FormEvent) => {
    e.preventDefault();
    setLoading(true);
    setSearched(true);
    try {
      await fetchProperties(1, false);
    } catch (err) {
      console.error(err);
    } finally {
      setLoading(false);
    }
  };

  const handleLoadMore = async () => {
    setLoadingMore(true);
    try {
      await fetchProperties(page + 1, true);
    } catch (err) {
      console.error(err);
    } finally {
      setLoadingMore(false);
    }
  };

  const handleMapClick = (lat: number, lng: number) => {
    setForm({
      ...form,
      x: lng.toFixed(4),
      y: lat.toFixed(4)
    });
  };

  return (
    <div className="min-h-screen font-sans selection:bg-brand-500 selection:text-black pb-20">
      {/* Header / Hero */}
      <div className="relative bg-black overflow-hidden border-b border-white/5">
        <div className="absolute inset-0">
          <img
            className="w-full h-full object-cover opacity-40 filter grayscale contrast-125"
            src="https://images.unsplash.com/photo-1600596542815-ffad4c1539a9?auto=format&fit=crop&q=80"
            alt="Luxury Real Estate"
          />
          <div className="absolute inset-0 bg-gradient-to-b from-transparent to-neutral-950"></div>
        </div>
        <div className="relative max-w-7xl mx-auto py-24 px-4 sm:py-32 sm:px-6 lg:px-8 flex flex-col items-center text-center">
          <div className="animate-spring-up" style={{ animationDelay: '0.1s' }}>
            <span className="text-brand-500 font-black tracking-[0.2em] uppercase text-sm mb-4 block">Exclusive Listings</span>
            <h1 className="text-5xl font-black tracking-tight text-white sm:text-6xl lg:text-7xl">
              Discover <span className="text-transparent bg-clip-text bg-gradient-to-r from-brand-100 to-brand-500">Luxury.</span>
            </h1>
          </div>
          <p className="mt-6 text-lg text-neutral-400 max-w-2xl animate-spring-up" style={{ animationDelay: '0.2s' }}>
            Pinpoint your ideal coordinates on the global map below and unlock a curated selection of premium properties engineered for the modern lifestyle.
          </p>
        </div>
      </div>

      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 -mt-16 relative z-10 space-y-12 animate-spring-up" style={{ animationDelay: '0.3s' }}>
        
        {/* Interactive Map Section */}
        <div className="glass-panel rounded-2xl p-2">
          <div className="px-5 py-4 border-b border-white/5 rounded-t-xl flex justify-between items-center bg-black/40">
            <h2 className="text-lg font-bold text-white flex items-center">
              <svg className="w-5 h-5 mr-2 text-brand-500" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M3.055 11H5a2 2 0 012 2v1a2 2 0 002 2 2 2 0 012 2v2.945M8 3.935V5.5A2.5 2.5 0 0010.5 8h.5a2 2 0 012 2 2 2 0 104 0 2 2 0 012-2h1.064M15 20.488V18a2 2 0 012-2h3.064M21 12a9 9 0 11-18 0 9 9 0 0118 0z"></path></svg>
              Global Explorer
            </h2>
            <span className="text-xs font-black tracking-widest uppercase bg-brand-500/10 text-brand-500 px-3 py-1.5 rounded-full border border-brand-500/20">Click map to drop pin</span>
          </div>
          <InteractiveMap 
            properties={properties}
            selectedLat={parseFloat(form.y)}
            selectedLng={parseFloat(form.x)}
            searchRadiusKm={parseFloat(form.distance)}
            onMapClick={handleMapClick}
          />
        </div>

        {/* Search Box */}
        <div className="glass-panel rounded-2xl p-6 sm:p-10 transition-all hover:border-white/20">
          <h3 className="text-xl font-bold text-white mb-6 flex items-center">
            <svg className="w-6 h-6 mr-3 text-brand-500" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4"></path></svg>
            Refine Parameters
          </h3>
          <form onSubmit={handleSearch} className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            <div>
              <label className="block text-xs font-bold text-neutral-400 uppercase tracking-wider mb-2">Longitude (X)</label>
              <input type="text" value={form.x} onChange={e => setForm({...form, x: e.target.value})} className="glass-input block w-full rounded-lg sm:text-sm px-4 py-3" />
            </div>
            <div>
              <label className="block text-xs font-bold text-neutral-400 uppercase tracking-wider mb-2">Latitude (Y)</label>
              <input type="text" value={form.y} onChange={e => setForm({...form, y: e.target.value})} className="glass-input block w-full rounded-lg sm:text-sm px-4 py-3" />
            </div>
            <div>
              <label className="block text-xs font-bold text-neutral-400 uppercase tracking-wider mb-2">Search Radius (km)</label>
              <input type="text" value={form.distance} onChange={e => setForm({...form, distance: e.target.value})} className="glass-input block w-full rounded-lg sm:text-sm px-4 py-3" />
            </div>
            <div>
              <label className="block text-xs font-bold text-neutral-400 uppercase tracking-wider mb-2">Max Price ($)</label>
              <input type="text" value={form.maxPrice} onChange={e => setForm({...form, maxPrice: e.target.value})} className="glass-input block w-full rounded-lg sm:text-sm px-4 py-3" />
            </div>
            <div>
              <label className="block text-xs font-bold text-neutral-400 uppercase tracking-wider mb-2">Min Area (sq. ft)</label>
              <input type="text" value={form.minArea} onChange={e => setForm({...form, minArea: e.target.value})} className="glass-input block w-full rounded-lg sm:text-sm px-4 py-3" />
            </div>
            <div>
              <label className="block text-xs font-bold text-neutral-400 uppercase tracking-wider mb-2">Min Bedrooms</label>
              <input type="text" value={form.minBedrooms} onChange={e => setForm({...form, minBedrooms: e.target.value})} className="glass-input block w-full rounded-lg sm:text-sm px-4 py-3" />
            </div>
            <div className="md:col-span-2 lg:col-span-3 flex justify-end pt-4">
              <button type="submit" disabled={loading} className="inline-flex justify-center items-center py-3 px-10 border border-brand-500 shadow-[0_0_15px_rgba(6,182,212,0.4)] text-sm font-bold uppercase tracking-wider rounded-lg text-black bg-brand-500 hover:bg-white hover:border-white hover:shadow-[0_0_20px_rgba(255,255,255,0.6)] focus:outline-none transition-all transform active:scale-95 disabled:opacity-50">
                {loading ? 'Processing...' : 'Execute Search'}
              </button>
            </div>
          </form>
        </div>

        {/* Results */}
        <div className="mt-12">
          {!searched && !loading && (
            <div className="text-center text-neutral-500 mt-16 font-light italic">
              Awaiting coordinates. Drop a pin on the map to begin.
            </div>
          )}
          
          {loading && (
            <div className="flex justify-center items-center py-20">
              <div className="animate-spin rounded-full h-12 w-12 border-t-2 border-r-2 border-brand-500 shadow-[0_0_15px_rgba(6,182,212,0.5)]"></div>
            </div>
          )}

          {searched && !loading && properties.length === 0 && (
            <div className="text-center text-neutral-400 mt-10 p-12 glass-panel rounded-2xl">
              <svg className="w-12 h-12 mx-auto text-neutral-600 mb-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="1" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z"></path></svg>
              <h3 className="text-xl font-bold text-white">No listings located</h3>
              <p className="mt-2 text-sm">Expand your search radius or adjust the parameters.</p>
            </div>
          )}

          {searched && !loading && properties.length > 0 && (
            <div>
              {/* Results Header */}
              <div className="mb-8 flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4 border-b border-white/10 pb-4">
                <h2 className="text-2xl font-black tracking-tight text-white">
                  <span className="text-brand-500">{properties.length}</span>
                  {total > properties.length && (
                    <span className="text-neutral-500 text-lg font-medium"> of {total}</span>
                  )}
                  {' '}Premium Listings Found
                </h2>
                {queryTime !== null && (
                  <span className="text-xs font-mono tracking-wide bg-neutral-800/80 text-neutral-400 px-3 py-1.5 rounded-full border border-white/10 flex items-center gap-2">
                    <span className="inline-block w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse"></span>
                    {queryTime < 1 ? '<1' : queryTime.toFixed(1)}ms
                  </span>
                )}
              </div>

              {/* Property Grid */}
              <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-8">
                {properties.map((p, i) => (
                  <div 
                    key={`${p.location}-${i}`} 
                    className="group glass-panel rounded-xl overflow-hidden flex flex-col h-full transform hover:-translate-y-2 hover:border-brand-500/50 hover:shadow-[0_10px_30px_rgba(6,182,212,0.15)] transition-all duration-300 animate-spring-up"
                    style={{ animationDelay: `${Math.min(i * 0.05, 0.4)}s` }}
                  >
                    <div className="h-56 bg-neutral-900 overflow-hidden relative border-b border-white/10">
                      <img src={`https://images.unsplash.com/photo-1512917774080-9991f1c4c750?auto=format&fit=crop&w=800&q=80&sig=${i}`} alt="Property" className="w-full h-full object-cover transition-transform duration-700 group-hover:scale-110 filter brightness-90 group-hover:brightness-110" />
                      <div className="absolute inset-0 bg-gradient-to-t from-black/80 via-transparent to-transparent"></div>
                      <div className="absolute bottom-4 left-4 right-4 flex justify-between items-end">
                        <div className="bg-black/60 backdrop-blur-md border border-white/20 px-3 py-1 rounded-md text-sm font-black tracking-wide text-brand-50 shadow-lg">
                          ${p.price.toLocaleString()}
                        </div>
                        {p.distance_km !== undefined && (
                          <div className="bg-black/60 backdrop-blur-md border border-white/20 px-2.5 py-1 rounded-md text-xs font-bold text-neutral-300">
                            {p.distance_km < 1 ? `${(p.distance_km * 1000).toFixed(0)}m` : `${p.distance_km.toFixed(1)}km`}
                          </div>
                        )}
                      </div>
                    </div>
                    <div className="p-6 flex flex-col flex-grow bg-neutral-900/50">
                      <h3 className="text-lg font-bold text-white mb-4 line-clamp-1 group-hover:text-brand-500 transition-colors">{p.location}</h3>
                      <div className="flex justify-between text-sm text-neutral-400 font-medium border-t border-white/5 pt-4">
                        <div className="flex items-center">
                          <svg className="w-4 h-4 mr-2 text-brand-500" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6"></path></svg>
                          {p.bedrooms} Beds
                        </div>
                        <div className="flex items-center">
                          <svg className="w-4 h-4 mr-2 text-brand-500" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 8V4m0 0h4M4 4l5 5m11-1V4m0 0h-4m4 0l-5 5M4 16v4m0 0h4m-4 0l5-5m11 5l-5-5m5 5v-4m0 4h-4"></path></svg>
                          {Math.round(p.area).toLocaleString()} sq.ft.
                        </div>
                      </div>
                    </div>
                  </div>
                ))}
              </div>

              {/* Load More Button */}
              {hasMore && (
                <div className="flex justify-center mt-12">
                  <button
                    onClick={handleLoadMore}
                    disabled={loadingMore}
                    className="group inline-flex items-center gap-3 py-3.5 px-10 border border-white/15 text-sm font-bold uppercase tracking-wider rounded-lg text-neutral-300 bg-white/5 backdrop-blur-sm hover:bg-white/10 hover:border-brand-500/40 hover:text-brand-500 hover:shadow-[0_0_20px_rgba(6,182,212,0.15)] focus:outline-none transition-all duration-300 disabled:opacity-50"
                  >
                    {loadingMore ? (
                      <>
                        <div className="animate-spin rounded-full h-4 w-4 border-t-2 border-r-2 border-brand-500"></div>
                        Loading...
                      </>
                    ) : (
                      <>
                        <svg className="w-4 h-4 transition-transform group-hover:translate-y-0.5" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19 9l-7 7-7-7"></path></svg>
                        Load More ({total - properties.length} remaining)
                      </>
                    )}
                  </button>
                </div>
              )}

              {/* All loaded indicator */}
              {!hasMore && total > 0 && properties.length >= total && (
                <div className="text-center mt-10 text-neutral-600 text-sm font-medium tracking-wide uppercase">
                  — All {total} listings loaded —
                </div>
              )}
            </div>
          )}
        </div>
      </div>
    </div>
  )
}

export default App
