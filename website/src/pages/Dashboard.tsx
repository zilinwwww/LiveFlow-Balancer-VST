import { useState, useEffect } from 'react';
import { Navigate } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import { useLang } from '../contexts/LangContext';

interface License {
  id: string; product: string; key: string; status: string;
  machine_id: string | null; bound_at: string | null; created_at: string;
}

export function Dashboard() {
  const { user, loading: authLoading } = useAuth();
  const { i } = useLang();
  const [licenses, setLicenses] = useState<License[]>([]);
  const [loading, setLoading] = useState(true);
  const [unbinding, setUnbinding] = useState<string | null>(null);
  const [showRedeem, setShowRedeem] = useState(false);
  const [redeemKey, setRedeemKey] = useState('');
  const [redeeming, setRedeeming] = useState(false);
  const [redeemMsg, setRedeemMsg] = useState<{ type: 'ok' | 'err'; text: string } | null>(null);

  const fetchLicenses = async () => {
    try {
      const res = await fetch('/api/licenses/list', { credentials: 'same-origin' });
      const data = await res.json();
      if (data.status === 'ok') setLicenses(data.licenses);
    } catch { /* silent */ }
    setLoading(false);
  };

  useEffect(() => { if (user) fetchLicenses(); }, [user]);

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

  if (authLoading) return <div className="page"><p className="text-muted">{i('dash.loading')}</p></div>;
  if (!user) return <Navigate to="/login" />;

  return (
    <div className="page dashboard-page">
      <div className="dashboard-card">
        <div className="dashboard-header">
          <h2>{i('dash.title')}</h2>
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
