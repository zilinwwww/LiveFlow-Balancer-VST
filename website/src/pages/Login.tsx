import React, { useState } from 'react';
import { useNavigate, Link } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import { useLang } from '../contexts/LangContext';

export function Login() {
  const { refresh } = useAuth();
  const { i } = useLang();
  const navigate = useNavigate();
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
      if (data.status === 'ok') { refresh(); navigate('/dashboard'); }
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
        <p className="auth-switch">{i('login.noAccount')} <Link to="/register">{i('login.signup')}</Link></p>
      </form>
    </div>
  );
}
