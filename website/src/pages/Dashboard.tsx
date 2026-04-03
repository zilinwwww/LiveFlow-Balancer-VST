import { useState, useEffect } from 'react';
import { Navigate } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import { useLang } from '../contexts/LangContext';

interface License {
  id: string; product: string; key: string; status: string;
  machine_id: string | null; bound_at: string | null; created_at: string;
}

export function Dashboard() {
  const { user, loading: authLoading, refresh } = useAuth();
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

          </div>
        )}

      </div>
    </div>
  );
}
