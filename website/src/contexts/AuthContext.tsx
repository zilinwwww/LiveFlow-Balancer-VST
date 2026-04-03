import React, { useState, useEffect, createContext, useContext } from 'react';

export interface User { id: string; email: string; name?: string; created_at?: string; }
export interface AuthCtx {
  user: User | null;
  loading: boolean;
  refresh: () => void;
  logout: () => void;
}

const AuthContext = createContext<AuthCtx>({
  user: null, loading: true, refresh: () => {}, logout: () => {}
});

export const useAuth = () => useContext(AuthContext);

export function AuthProvider({ children }: { children: React.ReactNode }) {
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
