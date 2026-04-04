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

  const reviewsRef = useRef<HTMLDivElement>(null);
  const teamRef = useRef<HTMLDivElement>(null);

  const scroll = (ref: React.RefObject<HTMLDivElement | null>, dir: 'left' | 'right') => {
    if (ref.current) {
      const scrollAmount = ref.current.clientWidth * 0.8;
      ref.current.scrollBy({ left: dir === 'left' ? -scrollAmount : scrollAmount, behavior: 'smooth' });
    }
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
                <a href="https://download.micro-grav.com/LiveFlow%20Balancer-v1.0.0-rc1-win64.exe" target="_blank" rel="noreferrer" className="btn btn-secondary">
                  {i('home.downloadWin')}
                </a>
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
