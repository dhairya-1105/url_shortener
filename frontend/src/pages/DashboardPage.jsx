import React, { useEffect, useState } from 'react';
import { api } from '../api/client';

export default function DashboardPage() {
  const [metrics, setMetrics] = useState({
    totalUrls: 0,
    totalRedirects: 0,
    cacheHits: 0,
    cacheMisses: 0,
    cacheHitRate: 0,
  });

  const [loadBalancer, setLoadBalancer] = useState({
    algorithm: 'ROUND_ROBIN',
    servers: [],
  });

  const [loading, setLoading] = useState(true);
  const [simulating, setSimulating] = useState(false);
  const [simResults, setSimResults] = useState(null);
  const [error, setError] = useState(null);

  const refreshData = async () => {
    try {
      setLoading(true);
      const [mRes, lbRes] = await Promise.all([
        api.getMetrics(),
        api.getLoadBalancer(),
      ]);
      setMetrics(mRes);
      setLoadBalancer(lbRes);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    refreshData();
    const interval = setInterval(refreshData, 3000);
    return () => clearInterval(interval);
  }, []);

  const handleAlgorithmChange = async (e) => {
    const newAlgo = e.target.value;
    try {
      await api.setAlgorithm(newAlgo);
      setLoadBalancer((prev) => ({ ...prev, algorithm: newAlgo }));
    } catch (err) {
      alert(`Failed to update algorithm: ${err.message}`);
    }
  };

  const handleAddServer = async () => {
    try {
      await api.addServer();
      refreshData();
    } catch (err) {
      alert(`Failed to add server: ${err.message}`);
    }
  };

  const handleKillServer = async (id) => {
    try {
      await api.killServer(id);
      refreshData();
    } catch (err) {
      alert(`Failed to kill server: ${err.message}`);
    }
  };

  const handleRestoreServer = async (id) => {
    try {
      await api.restoreServer(id);
      refreshData();
    } catch (err) {
      alert(`Failed to restore server: ${err.message}`);
    }
  };

  const handleRemoveServer = async (id) => {
    try {
      await api.removeServer(id);
      refreshData();
    } catch (err) {
      alert(`Failed to remove server: ${err.message}`);
    }
  };

  const handleSimulate = async () => {
    try {
      setSimulating(true);
      const res = await api.simulateRequests(1000);
      setSimResults(res.distribution || {});
      setLoadBalancer((prev) => ({ ...prev, servers: res.servers || prev.servers }));
      refreshData();
    } catch (err) {
      alert(`Simulation failed: ${err.message}`);
    } finally {
      setSimulating(false);
    }
  };

  // Calculate max request count for scaling bar chart
  const maxReqs = Math.max(1, ...loadBalancer.servers.map((s) => s.totalRequests));

  return (
    <div>
      <div className="glass-card">
        <h1 className="page-title">System Metrics & Dashboard</h1>
        <p className="page-subtitle">
          Real-time metrics from C++ backend, LRU Cache performance & Load Balancer simulation.
        </p>

        {error && <div className="alert-error">⚠️ {error}</div>}

        {/* System Overview Cards */}
        <div className="stats-grid">
          <div className="stat-card">
            <div className="stat-header">Total URLs</div>
            <div className="stat-value">{metrics.totalUrls}</div>
          </div>
          <div className="stat-card">
            <div className="stat-header">Total Redirects</div>
            <div className="stat-value" style={{ color: '#06b6d4' }}>
              {metrics.totalRedirects}
            </div>
          </div>
          <div className="stat-card">
            <div className="stat-header">Cache Hit Rate</div>
            <div className="stat-value" style={{ color: '#10b981' }}>
              {metrics.cacheHitRate.toFixed(1)}%
            </div>
          </div>
          <div className="stat-card">
            <div className="stat-header">Cache Misses</div>
            <div className="stat-value" style={{ color: '#f59e0b' }}>
              {metrics.cacheMisses}
            </div>
          </div>
        </div>
      </div>

      {/* Load Balancer Simulation Card */}
      <div className="glass-card">
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '1.5rem' }}>
          <div>
            <h2 style={{ fontFamily: 'var(--font-heading)', fontSize: '1.5rem' }}>Load Balancer Simulation</h2>
            <p style={{ color: '#94a3b8', fontSize: '0.9rem' }}>
              Simulates distributed request routing across virtual server nodes.
            </p>
          </div>

          <div style={{ display: 'flex', gap: '0.75rem', alignItems: 'center' }}>
            <span style={{ fontSize: '0.9rem', color: '#94a3b8', fontWeight: 600 }}>Algorithm:</span>
            <select
              value={loadBalancer.algorithm}
              onChange={handleAlgorithmChange}
              className="input-field"
              style={{ width: '220px', padding: '0.5rem 0.8rem' }}
            >
              <option value="ROUND_ROBIN">Round Robin</option>
              <option value="LEAST_CONNECTIONS">Least Connections</option>
              <option value="CONSISTENT_HASHING">Consistent Hashing</option>
            </select>
          </div>
        </div>

        {/* Action Controls */}
        <div style={{ display: 'flex', gap: '1rem', marginBottom: '2rem' }}>
          <button onClick={handleAddServer} className="btn-secondary">
            + Add Server
          </button>
          <button onClick={handleSimulate} className="btn-primary" disabled={simulating}>
            {simulating ? 'Simulating...' : '🚀 Generate 1000 Requests'}
          </button>
        </div>

        {/* Visual Request Distribution Bar Chart */}
        <div style={{ marginBottom: '2rem' }}>
          <h3 style={{ fontSize: '1rem', color: '#94a3b8', marginBottom: '1rem', fontWeight: 600 }}>
            Server Load Distribution (Total Requests)
          </h3>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '0.8rem' }}>
            {loadBalancer.servers.map((server) => {
              const pct = maxReqs > 0 ? (server.totalRequests / maxReqs) * 100 : 0;
              return (
                <div key={server.id} style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
                  <div style={{ width: '90px', fontSize: '0.9rem', fontWeight: 600 }}>
                    Server {server.id}
                  </div>
                  <div
                    style={{
                      flex: 1,
                      background: 'rgba(255,255,255,0.05)',
                      borderRadius: '6px',
                      height: '24px',
                      overflow: 'hidden',
                      position: 'relative',
                    }}
                  >
                    <div
                      style={{
                        width: `${pct}%`,
                        height: '100%',
                        background: server.healthy
                          ? 'linear-gradient(90deg, #6366f1, #06b6d4)'
                          : 'rgba(244, 63, 94, 0.4)',
                        transition: 'width 0.4s ease',
                      }}
                    />
                  </div>
                  <div style={{ width: '110px', fontSize: '0.9rem', textAlign: 'right', fontFamily: 'var(--font-code)' }}>
                    {server.totalRequests} reqs
                  </div>
                </div>
              );
            })}
          </div>
        </div>

        {/* Simulated Servers Table */}
        <div className="table-container">
          <table className="data-table">
            <thead>
              <tr>
                <th>Server Node</th>
                <th>Status</th>
                <th>Active Connections</th>
                <th>Total Requests Handled</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {loadBalancer.servers.map((server) => (
                <tr key={server.id}>
                  <td style={{ fontWeight: 600 }}>Server {server.id}</td>
                  <td>
                    <span className={`status-badge ${server.healthy ? 'healthy' : 'killed'}`}>
                      {server.healthy ? '● HEALTHY' : '✖ KILLED'}
                    </span>
                  </td>
                  <td>{server.activeConnections}</td>
                  <td style={{ fontFamily: 'var(--font-code)' }}>{server.totalRequests}</td>
                  <td>
                    <div style={{ display: 'flex', gap: '0.5rem' }}>
                      {server.healthy ? (
                        <button onClick={() => handleKillServer(server.id)} className="btn-danger">
                          Kill Server
                        </button>
                      ) : (
                        <button onClick={() => handleRestoreServer(server.id)} className="btn-success">
                          Restore
                        </button>
                      )}
                      <button onClick={() => handleRemoveServer(server.id)} className="btn-secondary">
                        Remove
                      </button>
                    </div>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}
