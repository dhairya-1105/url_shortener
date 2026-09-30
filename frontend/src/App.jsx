import React, { useState } from 'react';
import Navbar from './components/Navbar';
import UrlShortenerPage from './pages/UrlShortenerPage';
import MyUrlsPage from './pages/MyUrlsPage';
import DashboardPage from './pages/DashboardPage';

export default function App() {
  const [activePage, setActivePage] = useState('shortener');

  return (
    <div className="app-container">
      <Navbar activePage={activePage} setActivePage={setActivePage} />

      <main className="main-content">
        {activePage === 'shortener' && <UrlShortenerPage />}
        {activePage === 'my-urls' && <MyUrlsPage />}
        {activePage === 'dashboard' && <DashboardPage />}
      </main>

      <footer className="footer">
        <p>ShortX — Portfolio C++ URL Shortener & Load Balancer Simulator</p>
        <p style={{ marginTop: '0.4rem', color: '#64748b', fontSize: '0.8rem' }}>
          C++17 • SQLite • Base62 • Custom LRU Cache • ThreadPool • Async Analytics
        </p>
      </footer>
    </div>
  );
}
