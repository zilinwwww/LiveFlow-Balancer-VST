import { useState, useEffect } from 'react';
import { Navigate, useNavigate } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import { useLang } from '../contexts/LangContext';

interface License {
  id: string; product: string; key: string; status: string;
  machine_id: string | null; machine_name: string | null; machine_info: string | null; bound_at: string | null; created_at: string;
}

export function Dashboard() {
  const navigate = useNavigate();
  const { user, loading: authLoading, refresh, logout } = useAuth();
  const { i } = useLang();
  
  const [activeTab, setActiveTab] = useState<'licenses' | 'settings'>('licenses');
  
  // Licenses State
  const [licenses, setLicenses] = useState<License[]>([]);
  const [loading, setLoading] = useState(true);
  const [unbinding, setUnbinding] = useState<string | null>(null);
  const [showRedeem, setShowRedeem] = useState(false);
  const [redeemKey, setRedeemKey] = useState('');
  const [redeeming, setRedeeming] = useState(false);
  const [redeemMsg, setRedeemMsg] = useState<{ type: 'ok' | 'err'; text: string } | null>(null);
  
  const [copiedKey, setCopiedKey] = useState<string | null>(null);

  // Settings State
  const [name, setName] = useState(user?.name || '');
  const [profileSaving, setProfileSaving] = useState(false);
  const [profileMsg, setProfileMsg] = useState<{ type: 'ok' | 'err'; text: string } | null>(null);

  const [oldPwd, setOldPwd] = useState('');
  const [newPwd, setNewPwd] = useState('');
  const [pwdSaving, setPwdSaving] = useState(false);
  const [pwdMsg, setPwdMsg] = useState<{ type: 'ok' | 'err'; text: string } | null>(null);

  const [showDeleteConfirm, setShowDeleteConfirm] = useState(false);
  const [deleteCode, setDeleteCode] = useState('');
  const [deleteCodeStatus, setDeleteCodeStatus] = useState<'idle' | 'sending' | 'cooldown'>('idle');
  const [deleteCountdown, setDeleteCountdown] = useState(60);
  const [deleting, setDeleting] = useState(false);
  const [deleteMsg, setDeleteMsg] = useState<{ type: 'ok' | 'err'; text: string } | null>(null);

  const handleSendDeleteCode = async () => {
    if (!user?.email) return;
    setDeleteMsg(null); setDeleteCodeStatus('sending');
    try {
      const res = await fetch('/api/auth/send-code', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email: user.email, intent: 'delete' }),
      });
      const data = await res.json();
      if (data.status === 'ok') {
        if (data.mocked) setDeleteMsg({ type: 'ok', text: `[Local] Code in console` });
        else setDeleteMsg({ type: 'ok', text: i('reg.codeSent') });
        setDeleteCodeStatus('cooldown'); setDeleteCountdown(60);
        const timer = setInterval(() => {
          setDeleteCountdown(c => {
            if (c <= 1) { clearInterval(timer); setDeleteCodeStatus('idle'); return 60; }
            return c - 1;
          });
        }, 1000);
      } else {
        setDeleteMsg({ type: 'err', text: data.message || "Failed" });
        setDeleteCodeStatus('idle');
      }
    } catch {
      setDeleteMsg({ type: 'err', text: i('reg.networkErr') });
      setDeleteCodeStatus('idle');
    }
  };

  const handleDeleteAccount = async () => {
    if (deleteCode.length !== 6) return;
    setDeleting(true); setDeleteMsg(null);
    try {
      const res = await fetch('/api/auth/delete-account', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code: deleteCode }), credentials: 'same-origin'
      });
      const data = await res.json();
      if (data.status === 'ok') {
        logout();
        navigate('/');
      } else {
        setDeleteMsg({ type: 'err', text: data.code === 'CODE_INVALID' ? i('reg.invalidCode') : data.message || "Deletion failed" });
      }
    } catch {
      setDeleteMsg({ type: 'err', text: i('reg.networkErr') });
    }
    setDeleting(false);
  };


  useEffect(() => {
    if (user && user.name) setName(user.name);
  }, [user]);

  const fetchLicenses = async () => {
    try {
      const res = await fetch('/api/licenses/list', { credentials: 'same-origin' });
      const data = await res.json();
      if (data.status === 'ok') setLicenses(data.licenses);
    } catch { /* silent */ }
    setLoading(false);
  };

  useEffect(() => { if (user && activeTab === 'licenses') fetchLicenses(); }, [user, activeTab]);

  const redeemLicense = async () => {
    if (!redeemKey.trim()) return;
    setRedeeming(true); setRedeemMsg(null);
    try {
      const res = await fetch('/api/licenses/redeem', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ key: redeemKey.trim() }), credentials: 'same-origin',
      });
      const data = await res.json();
      if (data.status === 'ok') {
        setRedeemMsg({ type: 'ok', text: i('dash.redeemOk') });
        setRedeemKey(''); fetchLicenses();
        setTimeout(() => { setShowRedeem(false); setRedeemMsg(null); }, 2000);
      } else {
        setRedeemMsg({ type: 'err', text: i('dash.redeemErr') });
      }
    } catch { setRedeemMsg({ type: 'err', text: 'Network error' }); }
    setRedeeming(false);
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

  const handleCopy = (key: string) => {
    if (navigator.clipboard) {
      navigator.clipboard.writeText(key);
      setCopiedKey(key);
      setTimeout(() => setCopiedKey(null), 2000);
    }
  };

  const handleUpdateProfile = async (e: React.FormEvent) => {
    e.preventDefault();
    setProfileSaving(true); setProfileMsg(null);
    try {
      const res = await fetch('/api/auth/update-profile', {
        method: 'PUT', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name }), credentials: 'same-origin',
      });
      if (res.ok) {
        setProfileMsg({ type: 'ok', text: i('dash.saveOk') });
        refresh(); // Refresh user context
        setTimeout(() => setProfileMsg(null), 3000);
      } else {
        setProfileMsg({ type: 'err', text: 'Failed to update profile' });
      }
    } catch { setProfileMsg({ type: 'err', text: 'Network error' }); }
    setProfileSaving(false);
  };

  const handleUpdatePassword = async (e: React.FormEvent) => {
    e.preventDefault();
    setPwdSaving(true); setPwdMsg(null);
    try {
      const res = await fetch('/api/auth/update-password', {
        method: 'PUT', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ oldPassword: oldPwd, newPassword: newPwd }), credentials: 'same-origin',
      });
      const data = await res.json();
      if (data.status === 'ok') {
        setPwdMsg({ type: 'ok', text: i('dash.pwdOk') });
        setOldPwd(''); setNewPwd('');
        setTimeout(() => setPwdMsg(null), 3000);
      } else {
        setPwdMsg({ type: 'err', text: data.code === 'INVALID_OLD_PASSWORD' ? i('dash.pwdErr') : 'Failed to update password' });
      }
    } catch { setPwdMsg({ type: 'err', text: 'Network error' }); }
    setPwdSaving(false);
  };

  if (authLoading) return <div className="page"><p className="text-muted">{i('dash.loading')}</p></div>;
  if (!user) return <Navigate to="/login" />;

  return (
    <div className="page dashboard-page">
      <div className="dashboard-card">
        
        <div className="dashboard-tabs">
          <button 
            className={`tab-btn ${activeTab === 'licenses' ? 'active' : ''}`} 
            onClick={() => setActiveTab('licenses')}
          >
            {i('dash.tabLicenses')}
          </button>
          <button 
            className={`tab-btn ${activeTab === 'settings' ? 'active' : ''}`} 
            onClick={() => setActiveTab('settings')}
          >
            {i('dash.tabSettings')}
          </button>
        </div>

        {activeTab === 'licenses' && (
          <div className="tab-content">
            <div className="dashboard-header">
              <h2>{i('dash.tabLicenses')}</h2>
              <button className="btn btn-primary" onClick={() => setShowRedeem(!showRedeem)}>
                {i('dash.redeem')}
              </button>
            </div>

            {showRedeem && (
              <div className="redeem-section">
                {redeemMsg && <div className={`alert alert-${redeemMsg.type === 'ok' ? 'success' : 'error'}`}>{redeemMsg.text}</div>}
                <div className="redeem-row">
                  <input type="text" placeholder={i('dash.redeemPlc')} value={redeemKey}
                    onChange={e => setRedeemKey(e.target.value.toUpperCase())} />
                  <button className="btn btn-primary" onClick={redeemLicense} disabled={redeeming || !redeemKey.trim()}>
                    {redeeming ? i('dash.redeeming') : i('dash.redeemBtn')}
                  </button>
                </div>
              </div>
            )}

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
                      <td className="license-key" onClick={() => handleCopy(lic.key)} title="Click to copy">
                        {lic.key}
                        <span className="copy-icon">
                          {copiedKey === lic.key ? (
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg>
                          ) : (
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg>
                          )}
                        </span>
                        {copiedKey === lic.key && <span className="copy-tooltip">Copied!</span>}
                      </td>
                      <td>
                        <span className={`status-badge status-${lic.status}`}>
                          {lic.status === 'bound' ? i('dash.active') : lic.status === 'available' ? i('dash.available') : i('dash.revoked')}
                        </span>
                      </td>
                      <td className="machine-id">
                        {lic.machine_id ? (
                          <div style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
                            <span>
                              {lic.machine_name ? lic.machine_name : 'Unknown Host'}
                              <span style={{ color: 'var(--text-dim)', fontSize: '0.8em', marginLeft: '5px' }}>
                                ({lic.machine_id.substring(0, 8)})
                              </span>
                            </span>
                            {lic.machine_info && (
                              <div className="tooltip-container" style={{ position: 'relative', display: 'inline-block', cursor: 'help' }}>
                                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" style={{ color: 'var(--accent)', verticalAlign: 'middle' }}>
                                  <circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line>
                                </svg>
                                <div className="tooltip-content" style={{ display: 'none', position: 'absolute', backgroundColor: 'var(--bg-card)', border: '1px solid var(--border)', padding: '10px', borderRadius: '8px', zIndex: 10, width: 'max-content', maxWidth: '300px', bottom: '120%', left: '50%', transform: 'translateX(-50%)', boxShadow: '0 4px 12px rgba(0,0,0,0.5)' }}>
                                  <pre style={{ margin: 0, fontSize: '11px', color: 'var(--text-muted)', whiteSpace: 'pre-wrap', wordBreak: 'break-all' }}>
                                    {lic.machine_info}
                                  </pre>
                                </div>
                              </div>
                            )}
                          </div>
                        ) : '—'}
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
        )}

        {activeTab === 'settings' && (
          <div className="tab-content settings-content">
            
            <div className="settings-section">
              <h3>{i('dash.profile')}</h3>
              {profileMsg && <div className={`alert alert-${profileMsg.type === 'ok' ? 'success' : 'error'}`}>{profileMsg.text}</div>}
              <form onSubmit={handleUpdateProfile} className="settings-form">
                <div className="form-group">
                  <label>{i('dash.email')}</label>
                  <input type="email" value={user.email} disabled />
                </div>
                <div className="form-group">
                  <label>{i('dash.dispName')}</label>
                  <input type="text" value={name} onChange={e => setName(e.target.value)} placeholder="E.g. John Doe" />
                </div>
                <button type="submit" className="btn btn-primary" disabled={profileSaving}>
                  {profileSaving ? i('dash.saving') : i('dash.save')}
                </button>
              </form>
            </div>

            <hr className="settings-divider" />

            <div className="settings-section">
              <h3>{i('dash.pwdTitle')}</h3>
              {pwdMsg && <div className={`alert alert-${pwdMsg.type === 'ok' ? 'success' : 'error'}`}>{pwdMsg.text}</div>}
              <form onSubmit={handleUpdatePassword} className="settings-form">
                <div className="form-group">
                  <label>{i('dash.oldPwd')}</label>
                  <input type="password" value={oldPwd} onChange={e => setOldPwd(e.target.value)} required />
                </div>
                <div className="form-group">
                  <label>{i('dash.newPwd')}</label>
                  <input type="password" value={newPwd} onChange={e => setNewPwd(e.target.value)} required minLength={8} />
                </div>
                <button type="submit" className="btn btn-primary" disabled={pwdSaving || !oldPwd || !newPwd}>
                  {pwdSaving ? i('dash.updating') : i('dash.updatePwd')}
                </button>
              </form>
            </div>

            <hr className="settings-divider" />

            <div className="settings-section">
              <h3 style={{ color: 'var(--error)' }}>{i('dash.dangerZone')}</h3>
              <p className="text-muted" style={{ marginBottom: '20px' }}>{i('dash.deleteWarn')}</p>
              
              {!showDeleteConfirm ? (
                <button className="btn btn-primary" style={{ background: 'rgba(255, 107, 107, 0.1)', color: 'var(--error)', border: '1px solid var(--error)' }} onClick={() => setShowDeleteConfirm(true)}>
                  {i('dash.deleteBtn')}
                </button>
              ) : (
                <div style={{ background: 'rgba(255, 107, 107, 0.05)', padding: '24px', borderRadius: 'var(--radius)', border: '1px solid rgba(255, 107, 107, 0.3)' }}>
                  {deleteMsg && <div className={`alert alert-${deleteMsg.type === 'ok' ? 'success' : 'error'}`}>{deleteMsg.text}</div>}
                  <p style={{ marginBottom: '15px' }}>{i('dash.deleteCode')}</p>
                  
                  <div style={{ display: 'flex', gap: '8px', marginBottom: '14px', maxWidth: '400px' }}>
                    <input type="email" value={user.email} disabled className="settings-form" style={{ padding: '10px', background: 'var(--bg-input)', border: '1px solid var(--border)', borderRadius: 'var(--radius-sm)', color: 'var(--text-muted)', flex: 1, cursor: 'not-allowed' }} />
                    <button 
                      type="button" 
                      className="btn" 
                      onClick={handleSendDeleteCode} 
                      disabled={deleteCodeStatus !== 'idle'} 
                      style={{ whiteSpace: 'nowrap', padding: '0 16px', background: 'transparent', border: '1px solid var(--error)', color: 'var(--error)' }}
                    >
                      {deleteCodeStatus === 'sending' ? i('reg.sending') : deleteCodeStatus === 'cooldown' ? i('reg.resendIn').replace('{s}', deleteCountdown.toString()) : i('reg.sendCode')}
                    </button>
                  </div>
                  
                  <input 
                    type="text" 
                    placeholder={i('reg.code')} 
                    value={deleteCode} 
                    onChange={e => setDeleteCode(e.target.value.toUpperCase())} 
                    style={{ letterSpacing: '2px', fontFamily: 'monospace', width: '100%', maxWidth: '400px', marginBottom: '20px', padding: '12px', background: 'var(--bg-input)', border: '1px solid var(--border)', borderRadius: 'var(--radius-sm)', color: 'var(--text-main)' }} 
                    maxLength={6}
                  />
                  
                  <div style={{ display: 'flex', gap: '12px' }}>
                    <button className="btn" onClick={() => { setShowDeleteConfirm(false); setDeleteCode(''); setDeleteMsg(null); }} style={{ background: 'var(--bg-card)', border: '1px solid var(--border)' }}>
                      取消 (Cancel)
                    </button>
                    <button className="btn btn-primary" style={{ background: 'var(--error)', color: '#fff', border: 'none' }} onClick={handleDeleteAccount} disabled={deleting || deleteCode.length !== 6}>
                      {deleting ? i('dash.deleting') : i('dash.deleteConfirm')}
                    </button>
                  </div>
                </div>
              )}
            </div>

          </div>
        )}

      </div>
    </div>
  );
}
