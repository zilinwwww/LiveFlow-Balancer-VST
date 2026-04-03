import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { useState, useEffect, createContext, useContext, useCallback } from 'react';
import './index.css';

// ── i18n ──
type Lang = 'en' | 'zh';

const t: Record<string, Record<Lang, string>> = {
  // Navbar
  'nav.products':     { en: 'Products',         zh: '产品' },
  'nav.dashboard':    { en: 'My Dashboard',     zh: '我的面板' },
  'nav.licenses':     { en: 'My Licenses',      zh: '我的许可证' },
  'nav.logout':       { en: 'Log Out',          zh: '退出登录' },
  'nav.login':        { en: 'Login',            zh: '登录' },
  'nav.signup':       { en: 'Sign Up',          zh: '注册' },

  // Home
  'home.series':      { en: 'Series',           zh: '系列' },
  'home.subtitle':    { en: 'Professional audio tools crafted by Micro-Grav Studio', zh: '由 Micro-Grav Studio 精心打造的专业音频工具' },
  'home.products':    { en: 'Products',         zh: '产品' },
  'home.tag':         { en: 'Adaptive Vocal-Aware Backing Rider', zh: '自适应人声感知伴奏增益骑行器' },
  'home.f1':          { en: 'Real-time vocal detection with intelligent gain riding', zh: '实时人声检测 + 智能增益骑行' },
  'home.f2':          { en: 'Interactive LUFS scatter plot with direct curve manipulation', zh: '交互式 LUFS 散点图，支持直接曲线操控' },
  'home.f3':          { en: 'Configurable dynamics, speed, and range controls', zh: '可配置的动态范围、速度和区间控制' },
  'home.f4':          { en: 'Expert mode with advanced parameters', zh: '专家模式与高级参数' },
  'home.getLicense':  { en: 'Get Free License',  zh: '免费获取许可证' },
  'home.more':        { en: 'More plugins coming soon.', zh: '更多插件即将推出。' },

  // Login
  'login.title':      { en: 'Login',            zh: '登录' },
  'login.email':      { en: 'Email',            zh: '邮箱' },
  'login.password':   { en: 'Password',         zh: '密码' },
  'login.submit':     { en: 'Login',            zh: '登录' },
  'login.loading':    { en: 'Logging in...',    zh: '登录中...' },
  'login.noAccount':  { en: "Don't have an account?", zh: '还没有账号？' },
  'login.signup':     { en: 'Sign up',          zh: '注册' },
  'login.invalidCred':{ en: 'Invalid email or password', zh: '邮箱或密码错误' },
  'login.failed':     { en: 'Login failed',     zh: '登录失败' },
  'login.networkErr': { en: 'Network error',    zh: '网络错误' },

  // Register
  'reg.title':        { en: 'Create Account',   zh: '创建账号' },
  'reg.name':         { en: 'Name (optional)',   zh: '姓名（选填）' },
  'reg.email':        { en: 'Email',            zh: '邮箱' },
  'reg.password':     { en: 'Password (min 8 chars)', zh: '密码（至少8位）' },
  'reg.submit':       { en: 'Sign Up',          zh: '注册' },
  'reg.loading':      { en: 'Creating account...', zh: '正在创建...' },
  'reg.hasAccount':   { en: 'Already have an account?', zh: '已有账号？' },
  'reg.login':        { en: 'Login',            zh: '登录' },
  'reg.emailExists':  { en: 'Email already registered', zh: '该邮箱已注册' },
  'reg.failed':       { en: 'Registration failed', zh: '注册失败' },
  'reg.networkErr':   { en: 'Network error',    zh: '网络错误' },
  'reg.success':      { en: '🎉 Registration successful! Redirecting...', zh: '🎉 注册成功！正在跳转...' },

  // Dashboard
  'dash.title':       { en: 'My Licenses',      zh: '我的许可证' },
  'dash.claim':       { en: 'Claim Balancer License', zh: '领取 Balancer 许可证' },
  'dash.claiming':    { en: 'Claiming...',      zh: '领取中...' },
  'dash.loading':     { en: 'Loading...',       zh: '加载中...' },
  'dash.empty1':      { en: "You don't have any licenses yet.", zh: '你还没有任何许可证。' },
  'dash.empty2':      { en: 'Click "Claim Balancer License" above to get started — it\'s free!', zh: '点击上方「领取 Balancer 许可证」即可免费开始！' },
  'dash.product':     { en: 'Product',          zh: '产品' },
  'dash.key':         { en: 'License Key',      zh: '许可证密钥' },
  'dash.status':      { en: 'Status',           zh: '状态' },
  'dash.machine':     { en: 'Machine',          zh: '机器' },
  'dash.actions':     { en: 'Actions',          zh: '操作' },
  'dash.active':      { en: '✅ Active',         zh: '✅ 已激活' },
  'dash.available':   { en: '🔓 Available',     zh: '🔓 可用' },
  'dash.revoked':     { en: '❌ Revoked',        zh: '❌ 已撤销' },
  'dash.unbind':      { en: 'Unbind',           zh: '解绑' },
  'dash.unbinding':   { en: 'Unbinding...',     zh: '解绑中...' },

  // Footer
  'footer.rights':    { en: 'All rights reserved.', zh: '保留所有权利。' },
};

function detectLang(): Lang {
  const nav = navigator.language || '';
  return nav.startsWith('zh') ? 'zh' : 'en';
}

// ── Language Context ──
interface LangCtx { lang: Lang; setLang: (l: Lang) => void; i: (key: string) => string; }
const LangContext = createContext<LangCtx>({ lang: 'en', setLang: () => {}, i: (k) => k });
const useLang = () => useContext(LangContext);

function LangProvider({ children }: { children: React.ReactNode }) {
  const [lang, setLang] = useState<Lang>(detectLang);
  const i = useCallback((key: string) => t[key]?.[lang] ?? key, [lang]);
  return <LangContext.Provider value={{ lang, setLang, i }}>{children}</LangContext.Provider>;
}

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

// ── Lang Toggle ──
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

// ── Navbar ──
function Navbar() {
  const { user, logout } = useAuth();
  const { i } = useLang();
  return (
    <nav className="navbar">
      <a href="/" className="nav-brand">
        <span className="brand-live">Live</span><span className="brand-flow">Flow</span>
      </a>
      <div className="nav-links">
        <a href="/">{i('nav.products')}</a>
        {user ? (
          <div className="nav-user-menu">
            <span className="nav-email">{user.email} ▾</span>
            <div className="nav-dropdown">
              <a href="/dashboard">{i('nav.dashboard')}</a>
              <a href="/dashboard">{i('nav.licenses')}</a>
              <button onClick={() => { logout(); window.location.href = '/'; }}>{i('nav.logout')}</button>
            </div>
          </div>
        ) : (
          <>
            <a href="/login" className="btn btn-ghost">{i('nav.login')}</a>
            <a href="/register" className="btn btn-primary">{i('nav.signup')}</a>
          </>
        )}
        <LangToggle />
      </div>
    </nav>
  );
}

// ── Footer ──
function Footer() {
  const { i } = useLang();
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
        </div>
        <div className="footer-copy">© {new Date().getFullYear()} Micro-Grav Studio. {i('footer.rights')}</div>
      </div>
    </footer>
  );
}

// ── Home Page ──
function Home() {
  const { i } = useLang();
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
            <div className="product-screenshot">
              <img src="/plugin-screenshot.png" alt="LiveFlow Balancer" />
            </div>
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
                <a href="/register" className="btn btn-primary">{i('home.getLicense')}</a>
              </div>
            </div>
          </div>
        </div>
        <p className="coming-soon">{i('home.more')}</p>
      </section>
    </div>
  );
}

// ── Login Page ──
function Login() {
  const { refresh } = useAuth();
  const { i } = useLang();
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
      else setError(data.code === 'INVALID_CREDENTIALS' ? i('login.invalidCred') : data.message || i('login.failed'));
    } catch { setError(i('login.networkErr')); }
    setLoading(false);
  };

  return (
    <div className="page auth-page">
      <form className="auth-card" onSubmit={handleSubmit}>
        <h2>{i('login.title')}</h2>
        {error && <div className="alert alert-error">{error}</div>}
        <input type="email" placeholder={i('login.email')} value={email} onChange={e => setEmail(e.target.value)} required />
        <input type="password" placeholder={i('login.password')} value={password} onChange={e => setPassword(e.target.value)} required />
        <button type="submit" className="btn btn-primary btn-full" disabled={loading}>
          {loading ? i('login.loading') : i('login.submit')}
        </button>
        <p className="auth-switch">{i('login.noAccount')} <a href="/register">{i('login.signup')}</a></p>
      </form>
    </div>
  );
}

// ── Register Page ──
function Register() {
  const { refresh } = useAuth();
  const { i } = useLang();
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [name, setName] = useState('');
  const [error, setError] = useState('');
  const [success, setSuccess] = useState('');
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(''); setSuccess(''); setLoading(true);
    try {
      const res = await fetch('/api/auth/register', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password, name }), credentials: 'same-origin',
      });
      const data = await res.json();
      if (data.status === 'ok') {
        setSuccess(i('reg.success'));
        refresh();
        setTimeout(() => { window.location.href = '/dashboard'; }, 3000);
      }
      else setError(data.code === 'EMAIL_EXISTS' ? i('reg.emailExists') : data.message || i('reg.failed'));
    } catch { setError(i('reg.networkErr')); }
    setLoading(false);
  };

  return (
    <div className="page auth-page">
      <form className="auth-card" onSubmit={handleSubmit}>
        <h2>{i('reg.title')}</h2>
        {error && <div className="alert alert-error">{error}</div>}
        {success && <div className="alert alert-success">{success}</div>}
        <input type="text" placeholder={i('reg.name')} value={name} onChange={e => setName(e.target.value)} />
        <input type="email" placeholder={i('reg.email')} value={email} onChange={e => setEmail(e.target.value)} required />
        <input type="password" placeholder={i('reg.password')} value={password} onChange={e => setPassword(e.target.value)} required minLength={8} />
        <button type="submit" className="btn btn-primary btn-full" disabled={loading || !!success}>
          {loading ? i('reg.loading') : i('reg.submit')}
        </button>
        <p className="auth-switch">{i('reg.hasAccount')} <a href="/login">{i('reg.login')}</a></p>
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
  const { user, loading: authLoading } = useAuth();
  const { i } = useLang();
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

  if (authLoading) return <div className="page"><p className="text-muted">{i('dash.loading')}</p></div>;
  if (!user) return <Navigate to="/login" />;

  const hasBalancer = licenses.some(l => l.product === 'balancer');

  return (
    <div className="page dashboard-page">
      <div className="dashboard-card">
        <div className="dashboard-header">
          <h2>{i('dash.title')}</h2>
          {!hasBalancer && (
            <button className="btn btn-primary" onClick={() => claimLicense('balancer')} disabled={claiming}>
              {claiming ? i('dash.claiming') : i('dash.claim')}
            </button>
          )}
        </div>

        {loading ? (
          <p className="text-muted">{i('dash.loading')}</p>
        ) : licenses.length === 0 ? (
          <div className="empty-state">
            <p>{i('dash.empty1')}</p>
            <p>{i('dash.empty2')}</p>
          </div>
        ) : (
          <table className="license-table">
            <thead>
              <tr>
                <th>{i('dash.product')}</th>
                <th>{i('dash.key')}</th>
                <th>{i('dash.status')}</th>
                <th>{i('dash.machine')}</th>
                <th>{i('dash.actions')}</th>
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
                      {lic.status === 'bound' ? i('dash.active') : lic.status === 'available' ? i('dash.available') : i('dash.revoked')}
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
                        {unbinding === lic.id ? i('dash.unbinding') : i('dash.unbind')}
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
    <LangProvider>
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
    </LangProvider>
  );
}

export default App;
