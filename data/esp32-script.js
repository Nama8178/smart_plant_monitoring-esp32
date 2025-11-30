// Plant data - Will be populated from ESP32 sensor data
let plants = [
    {
        id: 1,
        name: 'Monstera Deliciosa',
        imageUrl: 'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200"><rect fill="%2310b981" width="300" height="200"/><text x="50%" y="50%" fill="white" font-size="20" text-anchor="middle" dy=".3em">Plant 1</text></svg>',
        temperature: 0,
        humidity: 0,
        soilMoisture: 0,
        soilRaw: 0,
        status: 'healthy',
        lastUpdated: new Date()
    },
    {
        id: 2,
        name: 'Snake Plant',
        imageUrl: 'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200"><rect fill="%23059669" width="300" height="200"/><text x="50%" y="50%" fill="white" font-size="20" text-anchor="middle" dy=".3em">Plant 2</text></svg>',
        temperature: 0,
        humidity: 0,
        soilMoisture: 0,
        soilRaw: 0,
        status: 'healthy',
        lastUpdated: new Date()
    },
    {
        id: 3,
        name: 'Peace Lily',
        imageUrl: 'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200"><rect fill="%2322c55e" width="300" height="200"/><text x="50%" y="50%" fill="white" font-size="20" text-anchor="middle" dy=".3em">Plant 3</text></svg>',
        temperature: 0,
        humidity: 0,
        soilMoisture: 0,
        soilRaw: 0,
        status: 'healthy',
        lastUpdated: new Date()
    }
];

let globalTemperature = 0;
let globalHumidity = 0;

// Check WiFi setup on load - Call ESP32 API
window.addEventListener('DOMContentLoaded', async () => {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();
        
        if (data.wifiConfigured) {
            showDashboard();
            updateWifiStatus(true, data.ssid);
        } else {
            showSetup();
            updateWifiStatus(false, 'Access Point');
        }
    } catch (error) {
        console.error('Error checking WiFi status:', error);
        showSetup();
        updateWifiStatus(false, 'Not Connected');
    }
});

// Update WiFi status display
function updateWifiStatus(connected, ssid) {
    const wifiStatus = document.getElementById('wifiStatus');
    if (connected) {
        wifiStatus.textContent = `Connected to ${ssid}`;
        wifiStatus.style.background = 'var(--success)';
    } else {
        wifiStatus.textContent = ssid;
        wifiStatus.style.background = 'var(--warning)';
    }
}

// WiFi Setup - Send to ESP32
document.getElementById('wifiForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;
    
    if (!ssid || !password) {
        showToast('Please enter both SSID and password');
        return;
    }
    
    const connectBtn = document.getElementById('connectBtn');
    connectBtn.textContent = 'Connecting...';
    connectBtn.disabled = true;
    
    try {
        const response = await fetch('/api/savewifi', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ ssid, password })
        });
        
        const data = await response.json();
        
        if (data.success) {
            showToast('Connected to WiFi successfully!');
            setTimeout(() => {
                showDashboard();
                updateWifiStatus(true, ssid);
            }, 2000);
        } else {
            showToast('Failed to connect: ' + data.message);
            connectBtn.textContent = 'Connect to WiFi';
            connectBtn.disabled = false;
        }
    } catch (error) {
        console.error('Error connecting to WiFi:', error);
        showToast('Connection error. Please try again.');
        connectBtn.textContent = 'Connect to WiFi';
        connectBtn.disabled = false;
    }
});

function showSetup() {
    document.getElementById('setupPage').classList.add('active');
    document.getElementById('dashboardPage').classList.remove('active');
}

function showDashboard() {
    document.getElementById('setupPage').classList.remove('active');
    document.getElementById('dashboardPage').classList.add('active');
    renderDashboard();
    startSensorUpdates();
}

function resetWifi() {
    showSetup();
}

// Render Dashboard
function renderDashboard() {
    updateStats();
    renderPlants();
    updateEnvironmentDisplay();
}

function updateStats() {
    const total = plants.length;
    const healthy = plants.filter(p => p.status === 'healthy').length;
    const warning = plants.filter(p => p.status === 'warning' || p.status === 'critical').length;
    
    document.getElementById('totalPlants').textContent = total;
    document.getElementById('healthyPlants').textContent = healthy;
    document.getElementById('warningPlants').textContent = warning;
}

function updateEnvironmentDisplay() {
    document.getElementById('globalTemperature').textContent = globalTemperature.toFixed(1) + '°C';
    document.getElementById('globalHumidity').textContent = globalHumidity.toFixed(1) + '%';
}

function renderPlants() {
    const grid = document.getElementById('plantsGrid');
    grid.innerHTML = '';
    
    plants.forEach(plant => {
        const card = createPlantCard(plant);
        grid.appendChild(card);
    });
}

function createPlantCard(plant) {
    const card = document.createElement('div');
    card.className = `plant-card ${plant.status}`;
    
    const moistureStatus = getMoistureStatus(plant.soilMoisture);
    
    card.innerHTML = `
        <div class="plant-image">
            <img src="${plant.imageUrl}" alt="${plant.name}">
            <div class="upload-overlay" onclick="uploadImage(${plant.id})">
                <svg fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M3 9a2 2 0 012-2h.93a2 2 0 001.664-.89l.812-1.22A2 2 0 0110.07 4h3.86a2 2 0 011.664.89l.812 1.22A2 2 0 0018.07 7H19a2 2 0 012 2v9a2 2 0 01-2 2H5a2 2 0 01-2-2V9z"/>
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M15 13a3 3 0 11-6 0 3 3 0 016 0z"/>
                </svg>
            </div>
            <span class="status-badge status-${plant.status}">${plant.status}</span>
        </div>
        <div class="plant-info">
            <div>
                <h3>${plant.name}</h3>
                <p class="timestamp">Updated ${plant.lastUpdated.toLocaleTimeString()}</p>
            </div>
            <div class="metrics-grid">
                <div class="metric">
                    <svg class="metric-icon" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z"/>
                    </svg>
                    <span class="metric-label">Temp</span>
                    <span class="metric-value">${plant.temperature.toFixed(1)}°C</span>
                </div>
                <div class="metric">
                    <svg class="metric-icon" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M3 4a1 1 0 011-1h16a1 1 0 011 1v2.586a1 1 0 01-.293.707l-6.414 6.414a1 1 0 00-.293.707V17l-4 4v-6.586a1 1 0 00-.293-.707L3.293 7.293A1 1 0 013 6.586V4z"/>
                    </svg>
                    <span class="metric-label">Humidity</span>
                    <span class="metric-value">${plant.humidity.toFixed(1)}%</span>
                </div>
                <div class="metric">
                    <svg class="metric-icon" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 16V4m0 0L3 8m4-4l4 4m6 0v12m0 0l4-4m-4 4l-4-4"/>
                    </svg>
                    <span class="metric-label">Moisture</span>
                    <span class="metric-value">${plant.soilMoisture.toFixed(1)}%</span>
                </div>
            </div>
            <div style="font-size: 12px; color: var(--text-secondary); text-align: center;">
                Raw: ${plant.soilRaw} | Status: <span class="moisture-status status-${moistureStatus.class}">${moistureStatus.text}</span>
            </div>
            <button class="btn btn-outline" onclick="showDetail(${plant.id})">View Details</button>
        </div>
    `;
    
    return card;
}

// Image Upload
function uploadImage(plantId) {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = 'image/*';
    input.capture = 'environment';
    
    input.onchange = (e) => {
        const file = e.target.files[0];
        if (file) {
            const reader = new FileReader();
            reader.onloadend = () => {
                const plant = plants.find(p => p.id === plantId);
                if (plant) {
                    plant.imageUrl = reader.result;
                    renderPlants();
                    showToast('Image updated successfully!');
                }
            };
            reader.readAsDataURL(file);
        }
    };
    
    input.click();
}

// Detail Modal
function showDetail(plantId) {
    const plant = plants.find(p => p.id === plantId);
    if (!plant) return;
    
    const modal = document.getElementById('detailModal');
    const content = document.getElementById('detailContent');
    
    const moistureStatus = getMoistureStatus(plant.soilMoisture);
    
    content.innerHTML = `
        <div class="detail-header">
            <h2 class="detail-title">
                <span>🌱</span>
                ${plant.name}
            </h2>
            <span class="status-badge status-${plant.status}">${plant.status}</span>
        </div>
        <p style="color: var(--text-secondary); margin-bottom: 24px;">Real-time monitoring data from your ESP32 device</p>
        
        <div class="detail-metrics">
            <div class="detail-metric">
                <div class="detail-metric-icon">
                    <svg fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z"/>
                    </svg>
                </div>
                <div class="detail-metric-info">
                    <div class="detail-metric-label">Temperature</div>
                    <div class="detail-metric-value">${plant.temperature.toFixed(1)}°C</div>
                    <div style="font-size: 12px; color: var(--text-secondary);">Shared DHT sensor</div>
                </div>
            </div>
            
            <div class="detail-metric">
                <div class="detail-metric-icon">
                    <svg fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M3 4a1 1 0 011-1h16a1 1 0 011 1v2.586a1 1 0 01-.293.707l-6.414 6.414a1 1 0 00-.293.707V17l-4 4v-6.586a1 1 0 00-.293-.707L3.293 7.293A1 1 0 013 6.586V4z"/>
                    </svg>
                </div>
                <div class="detail-metric-info">
                    <div class="detail-metric-label">Air Humidity</div>
                    <div class="detail-metric-value">${plant.humidity.toFixed(1)}%</div>
                    <div style="font-size: 12px; color: var(--text-secondary);">Shared DHT sensor</div>
                    <div class="progress-bar">
                        <div class="progress-fill" style="width: ${plant.humidity}%"></div>
                    </div>
                </div>
            </div>
            
            <div class="detail-metric">
                <div class="detail-metric-icon">
                    <svg fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 16V4m0 0L3 8m4-4l4 4m6 0v12m0 0l4-4m-4 4l-4-4"/>
                    </svg>
                </div>
                <div class="detail-metric-info">
                    <div class="detail-metric-label">Soil Moisture</div>
                    <div class="detail-metric-value">
                        ${plant.soilMoisture.toFixed(1)}% 
                        <span class="moisture-status status-${moistureStatus.class}">${moistureStatus.text}</span>
                    </div>
                    <div style="font-size: 12px; color: var(--text-secondary);">Raw value: ${plant.soilRaw}</div>
                    <div class="progress-bar">
                        <div class="progress-fill" style="width: ${plant.soilMoisture}%"></div>
                    </div>
                </div>
            </div>
            
            <div class="detail-metric">
                <div class="detail-metric-icon">
                    <svg fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 7V3m8 4V3m-9 8h10M5 21h14a2 2 0 002-2V7a2 2 0 00-2-2H5a2 2 0 00-2 2v12a2 2 0 002 2z"/>
                    </svg>
                </div>
                <div class="detail-metric-info">
                    <div class="detail-metric-label">Last Updated</div>
                    <div style="font-size: 18px; font-weight: 600;">${plant.lastUpdated.toLocaleString()}</div>
                </div>
            </div>
        </div>
    `;
    
    modal.classList.add('active');
}

function closeDetail() {
    document.getElementById('detailModal').classList.remove('active');
}

function getMoistureStatus(moisture) {
    if (moisture < 30) return { text: 'Too Dry', color: 'var(--danger)', class: 'critical' };
    if (moisture < 60) return { text: 'Optimal', color: 'var(--success)', class: 'healthy' };
    return { text: 'Too Wet', color: 'var(--warning)', class: 'warning' };
}

// Toast Notification
function showToast(message) {
    const toast = document.getElementById('toast');
    toast.textContent = message;
    toast.classList.add('show');
    
    setTimeout(() => {
        toast.classList.remove('show');
    }, 3000);
}

// Fetch actual sensor data from ESP32
async function fetchPlantData() {
    try {
        const response = await fetch('/api/plants');
        const data = await response.json();
        
        // Update plants array with real data from ESP32
        data.forEach((plantData, index) => {
            if (plants[index]) {
                plants[index].temperature = plantData.temperature;
                plants[index].humidity = plantData.humidity;
                plants[index].soilMoisture = plantData.soilMoisture;
                plants[index].soilRaw = plantData.soilRaw;
                plants[index].status = plantData.status;
                plants[index].lastUpdated = new Date();
            }
        });
        
        // Update global environment values (use first plant's shared values)
        if (data.length > 0) {
            globalTemperature = data[0].temperature;
            globalHumidity = data[0].humidity;
        }
        
        renderDashboard();
    } catch (error) {
        console.error('Error fetching plant data:', error);
    }
}

function startSensorUpdates() {
    // Fetch data immediately
    fetchPlantData();
    
    // Then fetch every 5 seconds
    setInterval(fetchPlantData, 5000);
}

// Close modal when clicking outside
window.onclick = (event) => {
    const modal = document.getElementById('detailModal');
    if (event.target === modal) {
        closeDetail();
    }
};