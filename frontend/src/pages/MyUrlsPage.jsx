import React, { useEffect, useState } from 'react';
import { api } from '../api/client';

export default function MyUrlsPage() {
  const [urls, setUrls] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [analyticsData, setAnalyticsData] = useState(null);
  const [activeModalCode, setActiveModalCode] = useState(null);

  const fetchUrls = async () => {
    try {
      setLoading(true);
      const data = await api.getAllUrls();
      setUrls(data.urls || []);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchUrls();
  }, []);

  const handleDelete = async (shortCode) => {
    if (!window.confirm(`Are you sure you want to delete short code "${shortCode}"?`)) return;
    try {
      await api.deleteUrl(shortCode);
      setUrls(urls.filter((u) => u.shortCode !== shortCode));
    } catch (err) {
      alert(`Delete failed: ${err.message}`);
    }
  };

  const handleCopy = (shortCode) => {
    const fullUrl = `${window.location.protocol}//${window.location.hostname}:8080/${shortCode}`;
    navigator.clipboard.writeText(fullUrl);
    alert(`Copied ${fullUrl} to clipboard!`);
  };

  const handleViewAnalytics = async (shortCode) => {
    try {
      const data = await api.getUrlAnalytics(shortCode);
      setAnalyticsData(data);
      setActiveModalCode(shortCode);
    } catch (err) {
      alert(`Failed to fetch analytics: ${err.message}`);
    }
  };

  return (
    <div className="glass-card">
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '1.5rem' }}>
        <div>
          <h1 className="page-title">My Shortened URLs</h1>
          <p className="page-subtitle" style={{ marginBottom: 0 }}>
            Manage active URL redirects and track click statistics.
          </p>
        </div>
        <button onClick={fetchUrls} className="btn-secondary">↻ Refresh</button>
      </div>

      {error && <div className="alert-error">⚠️ {error}</div>}

      {loading ? (
        <div style={{ color: '#94a3b8', padding: '2rem 0' }}>Loading URLs...</div>
      ) : urls.length === 0 ? (
        <div style={{ color: '#94a3b8', padding: '2rem 0', textAlign: 'center' }}>
          No shortened URLs created yet.
        </div>
      ) : (
        <div className="table-container">
          <table className="data-table">
            <thead>
              <tr>
                <th>Short Code</th>
                <th>Original URL</th>
                <th>Created At</th>
                <th>Clicks</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {urls.map((u) => (
                <tr key={u.id}>
                  <td>
                    <a
                      href={`http://localhost:8080/${u.shortCode}`}
                      target="_blank"
                      rel="noreferrer"
                      className="code-badge"
                      style={{ textDecoration: 'none' }}
                    >
                      {u.shortCode}
                    </a>
                  </td>
                  <td style={{ maxWidth: '300px', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
                    <a href={u.originalUrl} target="_blank" rel="noreferrer" style={{ color: '#94a3b8' }}>
                      {u.originalUrl}
                    </a>
                  </td>
                  <td style={{ fontSize: '0.85rem', color: '#64748b' }}>{u.createdAt}</td>
                  <td>
                    <strong style={{ color: '#10b981' }}>{u.clickCount}</strong>
                  </td>
                  <td>
                    <div style={{ display: 'flex', gap: '0.4rem' }}>
                      <button onClick={() => handleCopy(u.shortCode)} className="btn-secondary">
                        Copy
                      </button>
                      <button onClick={() => handleViewAnalytics(u.shortCode)} className="btn-secondary">
                        Analytics
                      </button>
                      <button onClick={() => handleDelete(u.shortCode)} className="btn-danger">
                        Delete
                      </button>
                    </div>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}

      {/* Analytics Modal */}
      {activeModalCode && analyticsData && (
        <div
          style={{
            position: 'fixed',
            top: 0,
            left: 0,
            right: 0,
            bottom: 0,
            backgroundColor: 'rgba(0,0,0,0.7)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            zIndex: 1000,
          }}
          onClick={() => setActiveModalCode(null)}
        >
          <div
            className="glass-card"
            style={{ width: '450px', maxWidth: '90%', margin: 0 }}
            onClick={(e) => e.stopPropagation()}
          >
            <h2 style={{ fontFamily: 'var(--font-heading)', fontSize: '1.4rem', marginBottom: '1rem' }}>
              Analytics: <span className="code-badge">{analyticsData.shortCode}</span>
            </h2>
            <div style={{ marginBottom: '1rem', fontSize: '0.9rem', color: '#94a3b8' }}>
              Original: <a href={analyticsData.originalUrl} target="_blank" rel="noreferrer" style={{ color: '#06b6d4' }}>{analyticsData.originalUrl}</a>
            </div>
            <div className="stats-grid" style={{ gridTemplateColumns: '1fr 1fr', marginBottom: '1.5rem' }}>
              <div className="stat-card">
                <div className="stat-header">Total Clicks</div>
                <div className="stat-value" style={{ color: '#10b981' }}>{analyticsData.totalClicks}</div>
              </div>
              <div className="stat-card">
                <div className="stat-header">Created At</div>
                <div className="stat-value" style={{ fontSize: '0.9rem', marginTop: '0.5rem' }}>{analyticsData.createdAt}</div>
              </div>
            </div>
            <button onClick={() => setActiveModalCode(null)} className="btn-primary" style={{ width: '100%' }}>
              Close
            </button>
          </div>
        </div>
      )}
    </div>
  );
}
