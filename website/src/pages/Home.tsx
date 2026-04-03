import { useState, useEffect } from 'react';
import { Link, useLocation } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import { useLang } from '../contexts/LangContext';

export function Home() {
  const { user } = useAuth();
  const { i } = useLang();
  const location = useLocation();
  const [claimStatus, setClaimStatus] = useState<'idle' | 'loading' | 'done'>('idle');
  const [isHighlighting, setIsHighlighting] = useState(false);

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
              <div className="product-actions">
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
    </div>
  );
}
