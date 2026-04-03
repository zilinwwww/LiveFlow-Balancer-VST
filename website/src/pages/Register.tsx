import React, { useState } from 'react';
import { useNavigate, Link } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import { useLang } from '../contexts/LangContext';

export function Register() {
  const { refresh } = useAuth();
  const { i } = useLang();
  const navigate = useNavigate();
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [name, setName] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);
  const [success, setSuccess] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(''); setLoading(true);
    try {
      const res = await fetch('/api/auth/register', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password, name }), credentials: 'same-origin',
      });
      const data = await res.json();
      if (data.status === 'ok') {
        setSuccess(true);
        refresh();
        setTimeout(() => navigate('/dashboard'), 3000);
      } else {
        setError(data.code === 'EMAIL_EXISTS' ? i('reg.emailExists') : data.message || i('reg.failed'));
      }
    } catch { setError(i('reg.networkErr')); }
    setLoading(false);
  };

  return (
    <div className="page auth-page">
      <form className="auth-card" onSubmit={handleSubmit}>
        <h2>{i('reg.title')}</h2>
        {success && <div className="alert alert-success">{i('reg.success')}</div>}
        {error && <div className="alert alert-error">{error}</div>}
        <input type="text" placeholder={i('reg.name')} value={name} onChange={e => setName(e.target.value)} />
        <input type="email" placeholder={i('reg.email')} value={email} onChange={e => setEmail(e.target.value)} required />
        <input type="password" placeholder={i('reg.password')} value={password} onChange={e => setPassword(e.target.value)} required minLength={8} />
        <button type="submit" className="btn btn-primary btn-full" disabled={loading || success}>
          {loading ? i('reg.loading') : i('reg.submit')}
        </button>
        <p className="auth-switch">{i('reg.hasAccount')} <Link to="/login">{i('reg.login')}</Link></p>
      </form>
    </div>
  );
}
