const API_BASE = 'http://localhost:8080/api';

async function fetchJson(url, options = {}) {
  const res = await fetch(url, {
    headers: {
      'Content-Type': 'application/json',
      ...options.headers,
    },
    ...options,
  });

  const data = await res.json();
  if (!res.ok) {
    throw new Error(data.error || `HTTP error ${res.status}`);
  }
  return data;
}

export const api = {
  async createUrl(url, customAlias = '') {
    return fetchJson(`${API_BASE}/urls`, {
      method: 'POST',
      body: JSON.stringify({ url, customAlias }),
    });
  },

  async getAllUrls() {
    return fetchJson(`${API_BASE}/urls`);
  },

  async getUrl(shortCode) {
    return fetchJson(`${API_BASE}/urls/${shortCode}`);
  },

  async deleteUrl(shortCode) {
    return fetchJson(`${API_BASE}/urls/${shortCode}`, {
      method: 'DELETE',
    });
  },

  async getMetrics() {
    return fetchJson(`${API_BASE}/metrics`);
  },

  async getUrlAnalytics(shortCode) {
    return fetchJson(`${API_BASE}/urls/${shortCode}/analytics`);
  },

  async getLoadBalancer() {
    return fetchJson(`${API_BASE}/load-balancer`);
  },

  async setAlgorithm(algorithm) {
    return fetchJson(`${API_BASE}/load-balancer/algorithm`, {
      method: 'POST',
      body: JSON.stringify({ algorithm }),
    });
  },

  async addServer() {
    return fetchJson(`${API_BASE}/servers`, {
      method: 'POST',
    });
  },

  async removeServer(id) {
    return fetchJson(`${API_BASE}/servers/${id}`, {
      method: 'DELETE',
    });
  },

  async killServer(id) {
    return fetchJson(`${API_BASE}/servers/${id}/kill`, {
      method: 'POST',
    });
  },

  async restoreServer(id) {
    return fetchJson(`${API_BASE}/servers/${id}/restore`, {
      method: 'POST',
    });
  },

  async simulateRequests(count = 1000) {
    return fetchJson(`${API_BASE}/load-balancer/simulate`, {
      method: 'POST',
      body: JSON.stringify({ count }),
    });
  },
};
