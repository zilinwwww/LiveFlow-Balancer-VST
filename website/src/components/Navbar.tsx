import { Link, useNavigate } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import { useLang } from '../contexts/LangContext';

function LangToggle() {
  const { lang, setLang } = useLang();
  return (
    <button
      className="btn btn-ghost btn-lang"
      onClick={() => setLang(lang === 'en' ? 'zh' : 'en')}
      title="Switch language"
    >
      {lang === 'en' ? '中文' : 'EN'}
    </button>
  );
}

export function Navbar() {
  const { user, logout } = useAuth();
  const { i } = useLang();
  const navigate = useNavigate();

  return (
    <nav className="navbar">
      <Link to="/" className="nav-brand">
        <span className="brand-live">Live</span><span className="brand-flow">Flow</span>
      </Link>
      <div className="nav-links">
        <Link to="/">{i('nav.products')}</Link>
        {user ? (
          <div className="nav-user-menu">
            <span className="nav-email">{user.email} ▾</span>
            <div className="nav-dropdown">
              <Link to="/dashboard">{i('nav.dashboard')}</Link>
              <button onClick={() => { logout(); navigate('/'); }}>{i('nav.logout')}</button>
            </div>
          </div>
        ) : (
          <>
            <Link to="/login" className="btn btn-ghost">{i('nav.login')}</Link>
            <Link to="/register" className="btn btn-primary">{i('nav.signup')}</Link>
          </>
        )}
        <LangToggle />
      </div>
    </nav>
  );
}
