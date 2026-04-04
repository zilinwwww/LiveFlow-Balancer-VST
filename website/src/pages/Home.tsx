import { useState, useEffect, useRef } from 'react';
import { Link, useLocation } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import { useLang } from '../contexts/LangContext';

export function Home() {
  const { user } = useAuth();
  const { i } = useLang();
  const location = useLocation();
  const [claimStatus, setClaimStatus] = useState<'idle' | 'loading' | 'done'>('idle');
  const [isHighlighting, setIsHighlighting] = useState(false);
  const [downloadingPlugin, setDownloadingPlugin] = useState<string | null>(null);

  const reviewsRef = useRef<HTMLDivElement>(null);
  const teamRef = useRef<HTMLDivElement>(null);

  const scroll = (ref: React.RefObject<HTMLDivElement | null>, dir: 'left' | 'right') => {
    if (ref.current) {
      const scrollAmount = ref.current.clientWidth * 0.8;
      ref.current.scrollBy({ left: dir === 'left' ? -scrollAmount : scrollAmount, behavior: 'smooth' });
    }
  };

  const [downloadUrl, setDownloadUrl] = useState("https://download.micro-grav.com/LiveFlow-Balancer-latest-win64.exe");
  const [downloadVersion, setDownloadVersion] = useState("Latest");

  useEffect(() => {
    fetch('https://download.micro-grav.com/latest.json?' + new Date().getTime())
      .then(res => res.json())
      .then((data: { version: string; file_url: string; exe_url?: string }) => {
        if (data.version && data.file_url) {
          setDownloadVersion(data.version);
          // Prefer exe_url if available, fallback to file_url
          setDownloadUrl(data.exe_url || data.file_url);
        }
      })
      .catch(e => console.warn('Could not fetch latest release:', e));
  }, []);

  useEffect(() => {
    const params = new URLSearchParams(window.location.search);
    const autoTarget = params.get('auto_download');
    // If autoTarget exists (e.g. 'balancer' or 'true'), trigger the overlay
    if (autoTarget && downloadUrl && !downloadUrl.includes('auto_download')) {
      const pluginName = autoTarget === 'true' ? 'Balancer' : (autoTarget.charAt(0).toUpperCase() + autoTarget.slice(1));
      setDownloadingPlugin(pluginName);
      const timer = setTimeout(() => {
        window.location.href = downloadUrl;
        setTimeout(() => setDownloadingPlugin(null), 2000); // hide overlay shortly after download triggers
      }, 1500); // 1.5s delay to let user see the nice overlay
      return () => clearTimeout(timer);
    }
  }, [downloadUrl]);

  const handleDownload = () => {
    window.location.href = downloadUrl;
  };

  useEffect(() => {
    if (location.state?.highlightClaim) {
      setIsHighlighting(true);
      window.history.replaceState({}, document.title);
    }
  }, [location]);

  const handleClaim = async () => {
    setClaimStatus('loading');
    try {
      const res = await fetch('/api/licenses/claim', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ product: 'balancer' }), credentials: 'same-origin',
      });
      const data = await res.json();
      if (data.status === 'ok' || data.code === 'ALREADY_CLAIMED') setClaimStatus('done');
      else setClaimStatus('idle');
    } catch { setClaimStatus('idle'); }
  };

  return (
    <div className="page home-page">
      {downloadingPlugin && (
        <div style={{
          position: 'fixed', top: 0, left: 0, right: 0, bottom: 0,
          background: 'rgba(9, 18, 30, 0.4)', backdropFilter: 'blur(16px)', WebkitBackdropFilter: 'blur(16px)',
          display: 'flex', alignItems: 'center', justifyContent: 'center', zIndex: 9999,
          animation: 'fadeIn 0.3s ease-out'
        }}>
          <div style={{
            background: 'linear-gradient(145deg, rgba(30, 41, 59, 0.7), rgba(15, 23, 42, 0.8))',
            border: '1px solid rgba(255, 255, 255, 0.15)',
            padding: '40px 60px', borderRadius: '24px', textAlign: 'center',
            boxShadow: '0 25px 50px -12px rgba(0, 0, 0, 0.5), inset 0 1px 0 rgba(255, 255, 255, 0.1)',
            transform: 'scale(1)'
          }}>
             <h2 style={{ color: '#fff', fontSize: '28px', margin: '0 0 12px 0', fontWeight: 600, letterSpacing: '0.5px' }}>
               <span style={{color: '#3ecfd5'}}>⤓</span> {i('home.series') === 'Series' ? 'Preparing Download...' : '正在准备下载...'}
             </h2>
             <p style={{ color: '#94a3b8', margin: 0, fontSize: '16px' }}>
               LiveFlow {downloadingPlugin} {i('home.series') === 'Series' ? 'is starting shortly' : '即将开始下载'}
             </p>
          </div>
        </div>
      )}

      <section className="hero">
        <div className="hero-text">
          <h1><span className="brand-live">Live</span><span className="brand-flow">Flow</span> {i('home.series')}</h1>
          <p className="hero-subtitle">{i('home.subtitle')}</p>
        </div>
      </section>

      <section className="products">
        <h2 className="section-title">{i('home.products')}</h2>
        <div className="product-grid">
          <div className="product-card">
            <div className="product-info">
              <h3>LiveFlow Balancer</h3>
              <p className="product-tag">{i('home.tag')}</p>
              <ul className="feature-list">
                <li>{i('home.f1')}</li>
                <li>{i('home.f2')}</li>
                <li>{i('home.f3')}</li>
                <li>{i('home.f4')}</li>
              </ul>
              <div className="product-actions" style={{ display: 'flex', gap: '1rem', alignItems: 'center' }}>
                <button onClick={handleDownload} className="btn btn-secondary">
                  {i('home.downloadWin')} ({downloadVersion})
                </button>
                {!user ? (
                  <Link to="/register" className="btn btn-primary">{i('home.regLicense')}</Link>
                ) : claimStatus === 'done' ? (
                  <Link to="/dashboard" className="btn btn-primary">{i('home.claimed')}</Link>
                ) : (
                  <button className={`btn btn-primary ${isHighlighting ? 'btn-pulse' : ''}`} onClick={handleClaim} disabled={claimStatus === 'loading'}>
                    {claimStatus === 'loading' ? i('home.claiming') : i('home.getLicense')}
                  </button>
                )}
              </div>
            </div>
            <div className="product-screenshot">
              <img src="/plugin-screenshot.png" alt="LiveFlow Balancer" />
            </div>
          </div>
        </div>
        <p className="coming-soon">{i('home.more')}</p>
      </section>

      <section className="testimonials">
        <h2 className="section-title">{i('home.reviews')}</h2>
        <div className="carousel-wrapper">
          <button className="carousel-btn prev" onClick={() => scroll(reviewsRef, 'left')}>‹</button>
          <div className="carousel-track" ref={reviewsRef}>
            <div className="review-card">
              <p className="review-text">{i('rev1.text')}</p>
              <p className="review-author">{i('rev1.author')}</p>
            </div>
            <div className="review-card">
              <p className="review-text">{i('rev2.text')}</p>
              <p className="review-author">{i('rev2.author')}</p>
            </div>
            <div className="review-card">
              <p className="review-text">{i('rev3.text')}</p>
              <p className="review-author">{i('rev3.author')}</p>
            </div>
            {/* You can copy and paste more .review-card elements here and they will slide naturally */}
          </div>
          <button className="carousel-btn next" onClick={() => scroll(reviewsRef, 'right')}>›</button>
        </div>
      </section>

      {false && (
      <section className="about-us">
        <h2 className="section-title">{i('home.team')}</h2>
        <p className="team-desc">{i('team.desc')}</p>
        <div className="carousel-wrapper" style={{ maxWidth: '900px' }}>
          <button className="carousel-btn prev" onClick={() => scroll(teamRef, 'left')}>‹</button>
          <div className="carousel-track" ref={teamRef}>
            <div className="team-card">
              <div className="team-avatar">Z</div>
              <h4>{i('team1.name')}</h4>
              <p>{i('team1.role')}</p>
            </div>
            <div className="team-card">
              <div className="team-avatar">A</div>
              <h4>{i('team2.name')}</h4>
              <p>{i('team2.role')}</p>
            </div>
            <div className="team-card">
              <div className="team-avatar">B</div>
              <h4>{i('team3.name')}</h4>
              <p>{i('team3.role')}</p>
            </div>
            {/* Same here, add more .team-card elements to let them slide */}
          </div>
          <button className="carousel-btn next" onClick={() => scroll(teamRef, 'right')}>›</button>
        </div>
      </section>
      )}
    </div>
  );
}
