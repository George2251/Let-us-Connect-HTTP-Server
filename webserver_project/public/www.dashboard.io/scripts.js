document.addEventListener('DOMContentLoaded', () => {
    // DOM Element Hooks
    const uptimeElement = document.getElementById('uptime-counter');
    const threadsElement = document.getElementById('active-threads');
    const progressFill = document.getElementById('thread-progress');
    const queueElement = document.getElementById('queue-size');
    const memoryElement = document.getElementById('memory-usage');
    const tableBody = document.getElementById('log-stream-body');

    // 1. Live Runtime Uptime Engine Counter Clock
    let totalSeconds = 0;
    setInterval(() => {
        totalSeconds++;
        const hrs = String(Math.floor(totalSeconds / 3600)).padStart(2, '0');
        const mins = String(Math.floor((totalSeconds % 3600) / 60)).padStart(2, '0');
        const secs = String(totalSeconds % 60).padStart(2, '0');
        uptimeElement.textContent = `${hrs}:${mins}:${secs}`;
    }, 1000);

    // Simulated Constants for Presentation Flow
    const sampleIps = ['192.168.1.45', '::1', '10.0.0.12', '127.0.0.1', '2001:db8::ff00:42'];
    const methods = ['GET', 'POST', 'GET', 'DELETE', 'GET'];
    const paths = ['/', '/www.fakehub.com', '/style.css', '/old_notes.txt', '/dashboard.io/script.js'];
    const codes = [200, 303, 200, 200, 200];

    // 2. Simulated Dynamic Metric Ticker Loops
    setInterval(() => {
        // Randomly adjust working thread activity simulations
        const active = Math.floor(Math.random() * 8) + 1; // 1 to 8 busy threads
        threadsElement.textContent = `${active} / 20`;
        
        const percentage = (active / 20) * 100;
        progressFill.style.width = `${percentage}%`;

        // Manage progress bar intensity context colors
        if(active > 14) {
            progressFill.className = "progress-fill intensity-high";
        } else if (active > 7) {
            progressFill.className = "progress-fill intensity-med";
        } else {
            progressFill.className = "progress-fill intensity-low";
        }

        // Random adjustments to memory profiling data strings
        const baseMem = 12.1 + (Math.random() * 0.8);
        memoryElement.textContent = `${baseMem.toFixed(1)} MB`;

        // Simulate intermittent Queue spikes
        if (Math.random() > 0.7) {
            queueElement.textContent = Math.floor(Math.random() * 3);
            setTimeout(() => queueElement.textContent = '0', 800);
        }

        // 3. Inject new row into Traffic Stream Table Grid UI
        if (Math.random() > 0.4) {
            const idx = Math.floor(Math.random() * sampleIps.length);
            const methodClass = `badge-${methods[idx].toLowerCase()}`;
            const codeClass = codes[idx] < 400 ? 'code-green' : 'code-amber';

            const newRow = document.createElement('tr');
            newRow.innerHTML = `
                <td style="font-family: monospace;">${sampleIps[idx]}</td>
                <td><span class="badge ${methodClass}">${methods[idx]}</span></td>
                <td style="font-family: monospace; color:#9ca3af;">${paths[idx]}</td>
                <td><span class="status-code ${codeClass}">${codes[idx]}</span></td>
            `;

            // Insert at top of table
            if (tableBody.firstChild) {
                tableBody.insertBefore(newRow, tableBody.firstChild);
            } else {
                tableBody.appendChild(newRow);
            }

            // Keep list bounded to last 5 connections for visual layout stability
            if (tableBody.children.length > 5) {
                tableBody.removeChild(tableBody.lastChild);
            }
        }
    }, 2000);
});