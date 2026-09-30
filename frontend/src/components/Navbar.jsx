import React from 'react';

export default function Navbar({ activePage, setActivePage }) {
  return (
    <header className="navbar">
      <a href="#" className="logo-container" onClick={(e) => { e.preventDefault(); setActivePage('shortener'); }}>
        <span>ShortX</span>
        <span className="logo-badge">C++ CORE</span>
      </a>

      <nav className="nav-links">
        <button
          className={`nav-btn ${activePage === 'shortener' ? 'active' : ''}`}
          onClick={() => setActivePage('shortener')}
        >
          URL Shortener
        </button>
        <button
          className={`nav-btn ${activePage === 'my-urls' ? 'active' : ''}`}
          onClick={() => setActivePage('my-urls')}
        >
          My URLs
        </button>
        <button
          className={`nav-btn ${activePage === 'dashboard' ? 'active' : ''}`}
          onClick={() => setActivePage('dashboard')}
        >
          System Dashboard
        </button>
      </nav>
    </header>
  );
}
