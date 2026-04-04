import { BrowserRouter, Routes, Route, Navigate, useLocation } from 'react-router-dom';
import { useEffect } from 'react';
import { LangProvider } from './contexts/LangContext';
import { AuthProvider } from './contexts/AuthContext';
import { Navbar } from './components/Navbar';
import { Footer } from './components/Footer';
import { Home } from './pages/Home';
import { Login } from './pages/Login';
import { Register } from './pages/Register';
import { Dashboard } from './pages/Dashboard';

function PageTitle() {
  const location = useLocation();
  useEffect(() => {
    const p = location.pathname;
    if (p.startsWith('/login')) document.title = "LiveFlow | Login";
    else if (p.startsWith('/register')) document.title = "LiveFlow | Sign Up";
    else if (p.startsWith('/dashboard')) document.title = "LiveFlow | Dashboard";
    else document.title = "LiveFlow | Balancer";
  }, [location.pathname]);
  return null;
}

export default function App() {
  return (
    <LangProvider>
      <AuthProvider>
        <BrowserRouter>
          <PageTitle />
          <div className="app-container">
            <Navbar />
            <main className="main-content">
              <Routes>
                <Route path="/" element={<Home />} />
                <Route path="/login" element={<Login />} />
                <Route path="/register" element={<Register />} />
                <Route path="/dashboard" element={<Dashboard />} />
                <Route path="*" element={<Navigate to="/" />} />
              </Routes>
            </main>
            <Footer />
          </div>
        </BrowserRouter>
      </AuthProvider>
    </LangProvider>
  );
}
