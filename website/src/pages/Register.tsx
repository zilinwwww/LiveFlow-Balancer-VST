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
  const [code, setCode] = useState('');
  
  const [error, setError] = useState('');
  const [info, setInfo] = useState('');
  const [loading, setLoading] = useState(false);
  const [success, setSuccess] = useState(false);

  const [sendCodeStatus, setSendCodeStatus] = useState<'idle' | 'sending' | 'cooldown'>('idle');
  const [countdown, setCountdown] = useState(60);

  const handleSendCode = async () => {
    if (!email) {
      setError("Please enter your email first.");
      return;
    }
    setError(''); setInfo('');
    setSendCodeStatus('sending');
    try {
      const res = await fetch('/api/auth/send-code', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email }),
      });
      const data = await res.json();
      if (data.status === 'ok') {
        if (data.mocked) {
          setInfo(`[Local Test] Code available in server console.`);
        } else {
          setInfo(i('reg.codeSent'));
        }
        
        setSendCodeStatus('cooldown');
        setCountdown(60);
        const timer = setInterval(() => {
          setCountdown(c => {
            if (c <= 1) { clearInterval(timer); setSendCodeStatus('idle'); return 60; }
            return c - 1;
          });
        }, 1000);
      } else {
        setError(data.code === 'EMAIL_EXISTS' ? i('reg.emailExists') : data.message || "Failed to send code");
        setSendCodeStatus('idle');
      }
    } catch { 
      setError(i('reg.networkErr'));
      setSendCodeStatus('idle');
    }
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(''); setInfo(''); setLoading(true);
    try {
      const res = await fetch('/api/auth/register', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, password, name, code: code.toUpperCase() }), credentials: 'same-origin',
      });
      const data = await res.json();
      if (data.status === 'ok') {
        setSuccess(true);
        refresh();
        setTimeout(() => navigate('/', { state: { highlightClaim: true } }), 3000);
      } else {
        if (data.code === 'EMAIL_EXISTS') setError(i('reg.emailExists'));
        else if (data.code === 'CODE_INVALID') setError(i('reg.invalidCode'));
        else setError(data.message || i('reg.failed'));
      }
    } catch { setError(i('reg.networkErr')); }
    setLoading(false);
  };

  return (
    <div className="page auth-page">
      <form className="auth-card" onSubmit={handleSubmit}>
        <h2>{i('reg.title')}</h2>
        {success && <div className="alert alert-success">{i('reg.success')}</div>}
        {(!success && info) && <div className="alert alert-success">{info}</div>}
        {error && <div className="alert alert-error">{error}</div>}
        
        <input type="text" placeholder={i('reg.name')} value={name} onChange={e => setName(e.target.value)} />
        
        <div style={{ display: 'flex', gap: '8px', marginBottom: '14px' }}>
          <input type="email" placeholder={i('reg.email')} value={email} onChange={e => setEmail(e.target.value)} required style={{ marginBottom: 0, flex: 1 }} />
          <button 
            type="button" 
            className="btn btn-secondary btn-sm" 
            onClick={handleSendCode} 
            disabled={sendCodeStatus !== 'idle' || !email} 
            style={{ whiteSpace: 'nowrap', padding: '0 12px' }}
          >
            {sendCodeStatus === 'sending' ? i('reg.sending') : sendCodeStatus === 'cooldown' ? i('reg.resendIn').replace('{s}', countdown.toString()) : i('reg.sendCode')}
          </button>
        </div>

        <input type="text" placeholder={i('reg.code')} value={code} onChange={e => setCode(e.target.value.toUpperCase())} required minLength={6} maxLength={6} style={{ letterSpacing: '2px', fontFamily: 'monospace' }} />
        
        <input type="password" placeholder={i('reg.password')} value={password} onChange={e => setPassword(e.target.value)} required minLength={8} />
        
        <button type="submit" className="btn btn-primary btn-full" disabled={loading || success}>
          {loading ? i('reg.loading') : i('reg.submit')}
        </button>
        <p className="auth-switch">{i('reg.hasAccount')} <Link to="/login">{i('reg.login')}</Link></p>
      </form>
    </div>
  );
}
