import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { useState, useEffect, createContext, useContext } from 'react';
import './index.css';

// ── Auth Context ──
interface User { id: string; email: string; }
interface AuthCtx { user: User | null; loading: boolean; refresh: () => void; logout: () => void; }
const AuthContext = createContext<AuthCtx>({ user: null, loading: true, refresh: () => {}, logout: () => {} });
const useAuth = () => useContext(AuthContext);

function AuthProvider({ children }: { children: React.ReactNode }) {
  const [user, setUser] = useState<User | null>(null);
  const [loading, setLoading] = useState(true);

  const refresh = () => {
    fetch('/api/auth/me', { credentials: 'same-origin' })
      .then(r => r.json())
      .then(d => { setUser(d.status === 'ok' ? d.user : null); setLoading(false); })
      .catch(() => { setUser(null); setLoading(false); });
  };

  const logout = async () => {
    await fetch('/api/auth/logout', { method: 'POST', credentials: 'same-origin' });
    setUser(null);
  };

  useEffect(() => { refresh(); }, []);
  return <AuthContext.Provider value={{ user, loading, refresh, logout }}>{children}</AuthContext.Provider>;
}

// ── Navbar ──
function Navbar() {
  const { user, logout } = useAuth();
  return (
    <nav className="navbar">
      <a href="/" className="nav-brand">
        <span className="brand-live">Live</span><span className="brand-flow">Flow</span>
      </a>
      <div className="nav-links">
        <a href="/">Products</a>
        {user ? (
          <>
            <a href="/dashboard">My Licenses</a>
            <span className="nav-email">{user.email}</span>
            <button className="btn btn-ghost" onClick={logout}>Logout</button>
          </>
        ) : (
          <>
            <a href="/login" className="btn btn-ghost">Login</a>
            <a href="/register" className="btn btn-primary">Sign Up</a>
          </>
        )}
      </div>
    </nav>
  );
}

// ── Footer ──
function Footer() {
  return (
    <footer className="footer">
      <div className="footer-inner">
        <div className="footer-brand">
          <span className="brand-live">Live</span><span className="brand-flow">Flow</span>
          <span className="footer-by">by <a href="https://micro-grav.com" target="_blank" rel="noreferrer">Micro-Grav Studio</a></span>
        </div>
        <div className="footer-links">
          <a href="https://micro-grav.com" target="_blank" rel="noreferrer">micro-grav.com</a>
          <a href="https://micrograv.net" target="_blank" rel="noreferrer">micrograv.net</a>
          <a href="https://github.com/zilinwwww/LiveFlow-Balancer-VST" target="_blank" rel="noreferrer">GitHub</a>
        </div>
        <div className="footer-copy">© {new Date().getFullYear()} Micro-Grav Studio. All rights reserved.</div>
      </div>
    </footer>
  );
}

// ── Home Page ──
function Home() {
  return (
    <div className="page home-page">
      <section className="hero">
        <div className="hero-text">
          <h1><span className="brand-live">Live</span><span className="brand-flow">Flow</span> Series</h1>
          <p className="hero-subtitle">Professional audio tools crafted by Micro-Grav Studio</p>
        </div>
      </section>

      <section className="products">
        <h2 className="section-title">Products</h2>
        <div className="product-grid">
          <div className="product-card">
            <div className="product-screenshot">
              <img src="/plugin-screenshot.png" alt="LiveFlow Balancer" />
            </div>
            <div className="product-info">
              <h3>LiveFlow Balancer</h3>
              <p className="product-tag">Adaptive Vocal-Aware Backing Rider</p>
              <ul className="feature-list">
                <li>Real-time vocal detection with intelligent gain riding</li>
                <li>Interactive LUFS scatter plot with direct curve manipulation</li>
                <li>Configurable dynamics, speed, and range controls</li>
                <li>Expert mode with advanced parameters</li>
              </ul>
              <div className="product-actions">
                <a href="/register" className="btn btn-primary">Get Free License</a>
                <a href="https://github.com/zilinwwww/LiveFlow-Balancer-VST" className="btn btn-ghost" target="_blank" rel="noreferrer">GitHub</a>
              </div>
            </div>
          </div>
        </div>
        <p className="coming-soon">More plugins coming soon.</p>
      </section>
    </div>
  );
}

// ── Login Page ──
function Login() {
  const { refresh } = useAuth();
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(''); setLoading(true);
    try {
      const res = await fetch('/api/auth/login', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password }), credentials: 'same-origin',
      });
      const data = await res.json();
      if (data.status === 'ok') { refresh(); window.location.href = '/dashboard'; }
      else setError(data.code === 'INVALID_CREDENTIALS' ? 'Invalid email or password' : data.message || 'Login failed');
    } catch { setError('Network error'); }
    setLoading(false);
  };

  return (
    <div className="page auth-page">
      <form className="auth-card" onSubmit={handleSubmit}>
        <h2>Login</h2>
        {error && <div className="alert alert-error">{error}</div>}
        <input type="email" placeholder="Email" value={email} onChange={e => setEmail(e.target.value)} required />
        <input type="password" placeholder="Password" value={password} onChange={e => setPassword(e.target.value)} required />
        <button type="submit" className="btn btn-primary btn-full" disabled={loading}>
          {loading ? 'Logging in...' : 'Login'}
        </button>
        <p className="auth-switch">Don't have an account? <a href="/register">Sign up</a></p>
      </form>
    </div>
  );
}

// ── Register Page ──
function Register() {
  const { refresh } = useAuth();
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [name, setName] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(''); setLoading(true);
    try {
      const res = await fetch('/api/auth/register', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password, name }), credentials: 'same-origin',
      });
      const data = await res.json();
      if (data.status === 'ok') { refresh(); window.location.href = '/dashboard'; }
      else setError(data.code === 'EMAIL_EXISTS' ? 'Email already registered' : data.message || 'Registration failed');
    } catch { setError('Network error'); }
    setLoading(false);
  };

  return (
    <div className="page auth-page">
      <form className="auth-card" onSubmit={handleSubmit}>
        <h2>Create Account</h2>
        {error && <div className="alert alert-error">{error}</div>}
        <input type="text" placeholder="Name (optional)" value={name} onChange={e => setName(e.target.value)} />
        <input type="email" placeholder="Email" value={email} onChange={e => setEmail(e.target.value)} required />
        <input type="password" placeholder="Password (min 8 chars)" value={password} onChange={e => setPassword(e.target.value)} required minLength={8} />
        <button type="submit" className="btn btn-primary btn-full" disabled={loading}>
          {loading ? 'Creating account...' : 'Sign Up'}
        </button>
        <p className="auth-switch">Already have an account? <a href="/login">Login</a></p>
      </form>
    </div>
  );
}

// ── Dashboard Page ──
interface License {
  id: string; product: string; key: string; status: string;
  machine_id: string | null; bound_at: string | null; created_at: string;
}

function Dashboard() {
  const { user } = useAuth();
  const [licenses, setLicenses] = useState<License[]>([]);
  const [loading, setLoading] = useState(true);
  const [claiming, setClaiming] = useState(false);
  const [unbinding, setUnbinding] = useState<string | null>(null);

  const fetchLicenses = async () => {
    try {
      const res = await fetch('/api/licenses/list', { credentials: 'same-origin' });
      const data = await res.json();
      if (data.status === 'ok') setLicenses(data.licenses);
    } catch { /* silent */ }
    setLoading(false);
  };

  useEffect(() => { if (user) fetchLicenses(); }, [user]);

  const claimLicense = async (product: string) => {
    setClaiming(true);
    try {
      const res = await fetch('/api/licenses/claim', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ product }), credentials: 'same-origin',
      });
      const data = await res.json();
      if (data.status === 'ok') fetchLicenses();
    } catch { /* silent */ }
    setClaiming(false);
  };

  const unbindLicense = async (licenseId: string) => {
    setUnbinding(licenseId);
    try {
      await fetch('/api/licenses/unbind', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ licenseId }), credentials: 'same-origin',
      });
      fetchLicenses();
    } catch { /* silent */ }
    setUnbinding(null);
  };

  if (!user) return <Navigate to="/login" />;

  const hasBalancer = licenses.some(l => l.product === 'balancer');

  return (
    <div className="page dashboard-page">
      <div className="dashboard-card">
        <div className="dashboard-header">
          <h2>My Licenses</h2>
          {!hasBalancer && (
            <button className="btn btn-primary" onClick={() => claimLicense('balancer')} disabled={claiming}>
              {claiming ? 'Claiming...' : 'Claim Balancer License'}
            </button>
          )}
        </div>

        {loading ? (
          <p className="text-muted">Loading...</p>
        ) : licenses.length === 0 ? (
          <div className="empty-state">
            <p>You don't have any licenses yet.</p>
            <p>Click "Claim Balancer License" above to get started — it's free!</p>
          </div>
        ) : (
          <table className="license-table">
            <thead>
              <tr>
                <th>Product</th>
                <th>License Key</th>
                <th>Status</th>
                <th>Machine</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {licenses.map(lic => (
                <tr key={lic.id}>
                  <td className="product-name">
                    {lic.product === 'balancer' ? 'LiveFlow Balancer' : lic.product}
                  </td>
                  <td className="license-key">{lic.key}</td>
                  <td>
                    <span className={`status-badge status-${lic.status}`}>
                      {lic.status === 'bound' ? '✅ Active' : lic.status === 'available' ? '🔓 Available' : '❌ Revoked'}
                    </span>
                  </td>
                  <td className="machine-id">
                    {lic.machine_id ? lic.machine_id.substring(0, 12) + '...' : '—'}
                  </td>
                  <td>
                    {lic.status === 'bound' && (
                      <button className="btn btn-ghost btn-sm"
                        onClick={() => unbindLicense(lic.id)}
                        disabled={unbinding === lic.id}>
                        {unbinding === lic.id ? 'Unbinding...' : 'Unbind'}
                      </button>
                    )}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>
    </div>
  );
}

// ── App ──
function App() {
  return (
    <AuthProvider>
      <BrowserRouter>
        <div className="app-layout">
          <Navbar />
          <main className="main-content">
            <Routes>
              <Route path="/" element={<Home />} />
              <Route path="/login" element={<Login />} />
              <Route path="/register" element={<Register />} />
              <Route path="/dashboard" element={<Dashboard />} />
            </Routes>
          </main>
          <Footer />
        </div>
      </BrowserRouter>
    </AuthProvider>
  );
}

export default App;
