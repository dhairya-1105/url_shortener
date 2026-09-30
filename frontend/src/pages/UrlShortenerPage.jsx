import React, { useState } from 'react';
import { api } from '../api/client';

export default function UrlShortenerPage() {
  const [url, setUrl] = useState('');
  const [customAlias, setCustomAlias] = useState('');
  const [result, setResult] = useState(null);
  const [error, setError] = useState(null);
  const [loading, setLoading] = useState(false);
  const [copied, setCopied] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!url.trim()) return;

    setError(null);
    setResult(null);
    setLoading(true);
    setCopied(false);

    try {
      const data = await api.createUrl(url, customAlias);
      setResult(data);
      setUrl('');
      setCustomAlias('');
    } catch (err) {
      setError(err.message || 'Failed to shorten URL');
    } finally {
      setLoading(false);
    }
  };

  const handleCopy = () => {
    if (result?.shortUrl) {
      navigator.clipboard.writeText(result.shortUrl);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    }
  };

  return (
    <div className="glass-card">
      <h1 className="page-title">ShortX URL Shortener</h1>
      <p className="page-subtitle">
        High performance Base62 URL shortener powered by C++17, SQLite & custom LRU Cache.
      </p>

      {error && <div className="alert-error">⚠️ {error}</div>}

      <form onSubmit={handleSubmit}>
        <div className="form-group">
          <label className="form-label">Destination URL</label>
          <input
            type="text"
            className="input-field"
            placeholder="https://example.com/very/long/destination/url"
            value={url}
            onChange={(e) => setUrl(e.target.value)}
            required
          />
        </div>

        <div className="form-group">
          <label className="form-label">Custom Alias (Optional)</label>
          <input
            type="text"
            className="input-field"
            placeholder="e.g. my-custom-link"
            value={customAlias}
            onChange={(e) => setCustomAlias(e.target.value)}
          />
        </div>

        <button type="submit" className="btn-primary" disabled={loading}>
          {loading ? 'Shortening...' : 'Shorten URL'}
        </button>
      </form>

      {result && (
        <div className="result-box">
          <div>
            <div style={{ fontSize: '0.85rem', color: '#94a3b8', marginBottom: '0.25rem' }}>Your Short URL:</div>
            <a href={result.shortUrl} target="_blank" rel="noreferrer" className="result-url">
              {result.shortUrl}
            </a>
          </div>
          <button onClick={handleCopy} className="btn-secondary">
            {copied ? '✓ Copied!' : 'Copy Link'}
          </button>
        </div>
      )}
    </div>
  );
}
