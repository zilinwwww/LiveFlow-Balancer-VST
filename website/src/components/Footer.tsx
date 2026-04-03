import { useLang } from '../contexts/LangContext';

export function Footer() {
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
