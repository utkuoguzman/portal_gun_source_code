#pragma once
#include <Arduino.h>

// Rick & Morty Portal Gun OTA Firmware Update Web Page
static const char PAGE_UPDATE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Portal Gun C-137 — OTA Firmware Updater</title>
    <style>
        :root {
            --portal-green: #39ff14;
            --portal-green-glow: rgba(57, 255, 20, 0.45);
            --portal-dark: #071009;
            --portal-card: rgba(12, 26, 16, 0.92);
            --portal-border: rgba(57, 255, 20, 0.35);
            --accent-cyan: #00e5ff;
            --accent-amber: #ffaa00;
            --accent-red: #ff3366;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, monospace, sans-serif;
            -webkit-tap-highlight-color: transparent;
        }

        body {
            background-color: var(--portal-dark);
            background-image: 
                linear-gradient(rgba(5, 16, 8, 0.85), rgba(5, 16, 8, 0.92)),
                url('site_assets/portal_background.jpg');
            background-size: cover;
            background-position: center top;
            background-repeat: no-repeat;
            background-attachment: fixed;
            color: #e0ffe5;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 16px;
        }

        .container {
            width: 100%;
            max-width: 440px;
            display: flex;
            flex-direction: column;
            gap: 16px;
        }

        .header-card {
            background: var(--portal-card);
            border: 1px solid var(--portal-border);
            border-radius: 14px;
            padding: 18px 16px;
            text-align: center;
            box-shadow: 0 0 20px var(--portal-green-glow);
            backdrop-filter: blur(8px);
        }

        .header-title {
            font-size: 1.35rem;
            font-weight: 800;
            letter-spacing: 2px;
            color: var(--portal-green);
            text-shadow: 0 0 10px var(--portal-green);
            text-transform: uppercase;
        }

        .header-sub {
            font-size: 0.75rem;
            letter-spacing: 1.5px;
            color: #8edaa0;
            margin-top: 4px;
        }

        .dim-card {
            background: var(--portal-card);
            border: 1px solid var(--portal-border);
            border-radius: 12px;
            padding: 16px;
            backdrop-filter: blur(8px);
        }

        .dim-title {
            font-size: 0.8rem;
            font-weight: 700;
            letter-spacing: 1px;
            color: var(--accent-cyan);
            margin-bottom: 12px;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }

        .sys-badge {
            background: rgba(0, 229, 255, 0.15);
            border: 1px solid var(--accent-cyan);
            color: var(--accent-cyan);
            font-size: 0.65rem;
            padding: 2px 8px;
            border-radius: 999px;
            font-weight: bold;
        }

        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 8px;
            font-size: 0.75rem;
            margin-bottom: 4px;
        }

        .info-item {
            background: rgba(0, 0, 0, 0.35);
            padding: 8px 10px;
            border-radius: 6px;
            border: 1px solid rgba(255, 255, 255, 0.08);
        }

        .info-label {
            color: #8edaa0;
            font-size: 0.65rem;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .info-val {
            color: #ffffff;
            font-weight: bold;
            font-size: 0.82rem;
            margin-top: 2px;
        }

        .drop-zone {
            border: 2px dashed var(--portal-border);
            border-radius: 10px;
            padding: 24px 16px;
            text-align: center;
            background: rgba(0, 0, 0, 0.4);
            cursor: pointer;
            transition: all 0.2s ease;
            position: relative;
        }

        .drop-zone:hover, .drop-zone.dragover {
            border-color: var(--portal-green);
            background: rgba(57, 255, 20, 0.08);
            box-shadow: 0 0 15px var(--portal-green-glow);
        }

        .drop-icon {
            font-size: 32px;
            margin-bottom: 8px;
            display: block;
        }

        .drop-text {
            font-size: 0.85rem;
            font-weight: bold;
            color: #e0ffe5;
        }

        .drop-sub {
            font-size: 0.7rem;
            color: #8edaa0;
            margin-top: 4px;
        }

        #file-input {
            position: absolute;
            top: 0; left: 0; width: 100%; height: 100%;
            opacity: 0;
            cursor: pointer;
        }

        .file-selected-box {
            display: none;
            background: rgba(57, 255, 20, 0.1);
            border: 1px solid var(--portal-green);
            border-radius: 8px;
            padding: 10px 14px;
            margin-top: 12px;
            font-size: 0.8rem;
            align-items: center;
            justify-content: space-between;
        }

        .btn-flash {
            display: none;
            width: 100%;
            margin-top: 14px;
            padding: 14px;
            background: linear-gradient(135deg, #2ecc71, #27ae60);
            border: 1px solid var(--portal-green);
            border-radius: 8px;
            color: #051a0c;
            font-size: 0.95rem;
            font-weight: 800;
            letter-spacing: 1.5px;
            cursor: pointer;
            box-shadow: 0 0 16px var(--portal-green-glow);
            text-shadow: 0 0 2px rgba(255,255,255,0.4);
            transition: all 0.2s;
        }

        .btn-flash:active {
            transform: scale(0.98);
        }

        .progress-card {
            display: none;
            margin-top: 14px;
        }

        .progress-bar-bg {
            width: 100%;
            height: 18px;
            background: rgba(0, 0, 0, 0.6);
            border-radius: 999px;
            border: 1px solid var(--portal-border);
            overflow: hidden;
            position: relative;
        }

        .progress-bar-fill {
            height: 100%;
            width: 0%;
            background: linear-gradient(90deg, #00e5ff, #39ff14);
            box-shadow: 0 0 12px var(--portal-green-glow);
            transition: width 0.1s linear;
        }

        .progress-meta {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-top: 8px;
            font-size: 0.75rem;
            color: #8edaa0;
        }

        .progress-pct {
            font-size: 0.9rem;
            font-weight: bold;
            color: var(--portal-green);
        }

        .status-msg {
            margin-top: 12px;
            font-size: 0.8rem;
            text-align: center;
            padding: 8px;
            border-radius: 6px;
            display: none;
        }

        .status-msg.success {
            display: block;
            background: rgba(57, 255, 20, 0.15);
            border: 1px solid var(--portal-green);
            color: var(--portal-green);
            font-weight: bold;
        }

        .status-msg.error {
            display: block;
            background: rgba(255, 51, 102, 0.15);
            border: 1px solid var(--accent-red);
            color: var(--accent-red);
            font-weight: bold;
        }

        .back-link {
            display: block;
            text-align: center;
            color: #8edaa0;
            font-size: 0.75rem;
            text-decoration: none;
            margin-top: 4px;
        }
        .back-link:hover {
            color: var(--portal-green);
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <div class="header-card">
            <div class="header-title">⚡ OTA UPDATER ⚡</div>
            <div class="header-sub">PORTAL GUN C-137 &bull; WIRELESS RE-FLASH</div>
        </div>

        <!-- Hardware System Status -->
        <div class="dim-card">
            <div class="dim-title">
                <span>🛸 Hardware System Diagnostics</span>
                <span class="sys-badge" id="ota-badge">A/B PROTECTED</span>
            </div>
            <div class="info-grid">
                <div class="info-item">
                    <div class="info-label">Target Hardware</div>
                    <div class="info-val" id="val-chip">Arduino Nano ESP32</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Flash Memory</div>
                    <div class="info-val" id="val-flash">16 MB (Dual 6.5MB)</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Active Partition</div>
                    <div class="info-val" id="val-part" style="color: var(--accent-cyan);">Detecting...</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Free Heap Space</div>
                    <div class="info-val" id="val-heap" style="color: var(--portal-green);">Detecting...</div>
                </div>
            </div>
        </div>

        <!-- Firmware Upload Card -->
        <div class="dim-card">
            <div class="dim-title">📦 Firmware Binary (.bin)</div>

            <div class="drop-zone" id="dropZone">
                <input type="file" id="file-input" accept=".bin">
                <span class="drop-icon">🚀</span>
                <div class="drop-text">Click or Drop <code>firmware.bin</code> here</div>
                <div class="drop-sub">Compiled PlatformIO / Arduino binary</div>
            </div>

            <div class="file-selected-box" id="fileBox">
                <div>
                    <span id="fileName" style="font-weight: bold; color: #fff;">firmware.bin</span>
                    <div id="fileSize" style="font-size: 0.7rem; color: #8edaa0;">0 KB</div>
                </div>
                <span style="color: var(--portal-green); font-size: 1.1rem;">✓</span>
            </div>

            <button class="btn-flash" id="btnFlash" onclick="startUpload()">⚡ INITIATE QUANTUM RE-FLASH</button>

            <!-- Progress Card -->
            <div class="progress-card" id="progressCard">
                <div class="progress-bar-bg">
                    <div class="progress-bar-fill" id="progressBar"></div>
                </div>
                <div class="progress-meta">
                    <span id="progressStatus">Uploading firmware to inactive slot...</span>
                    <span class="progress-pct" id="progressPct">0%</span>
                </div>
            </div>

            <div class="status-msg" id="statusMsg"></div>
        </div>

        <a href="/" class="back-link">← Return to Portal Gun Controls</a>
    </div>

    <script>
        let selectedFile = null;

        // Fetch Diagnostics on load
        fetch('/api/ota_info')
            .then(r => r.json())
            .then(data => {
                if (data.chip) document.getElementById('val-chip').innerText = data.chip;
                if (data.partition) document.getElementById('val-part').innerText = data.partition.toUpperCase();
                if (data.free_heap) document.getElementById('val-heap').innerText = Math.round(data.free_heap / 1024) + " KB";
                if (data.flash_size) document.getElementById('val-flash').innerText = data.flash_size + " MB (Dual A/B)";
            })
            .catch(() => {});

        const fileInput = document.getElementById('file-input');
        const fileBox = document.getElementById('fileBox');
        const fileName = document.getElementById('fileName');
        const fileSize = document.getElementById('fileSize');
        const btnFlash = document.getElementById('btnFlash');
        const dropZone = document.getElementById('dropZone');

        fileInput.addEventListener('change', (e) => {
            if (e.target.files.length > 0) {
                handleFile(e.target.files[0]);
            }
        });

        dropZone.addEventListener('dragover', (e) => { e.preventDefault(); dropZone.classList.add('dragover'); });
        dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragover'));
        dropZone.addEventListener('drop', (e) => {
            e.preventDefault();
            dropZone.classList.remove('dragover');
            if (e.dataTransfer.files.length > 0) {
                handleFile(e.dataTransfer.files[0]);
            }
        });

        function handleFile(file) {
            if (!file.name.endsWith('.bin')) {
                alert("Please select a valid .bin firmware file!");
                return;
            }
            selectedFile = file;
            fileName.innerText = file.name;
            fileSize.innerText = (file.size / 1024).toFixed(1) + " KB (" + (file.size / (1024*1024)).toFixed(2) + " MB)";
            fileBox.style.display = 'flex';
            btnFlash.style.display = 'block';
            dropZone.style.display = 'none';
        }

        function startUpload() {
            if (!selectedFile) return;

            btnFlash.disabled = true;
            btnFlash.style.opacity = '0.5';
            document.getElementById('progressCard').style.display = 'block';
            const statusMsg = document.getElementById('statusMsg');
            statusMsg.className = 'status-msg';
            statusMsg.style.display = 'none';

            const formData = new FormData();
            formData.append('firmware', selectedFile, selectedFile.name);

            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/update', true);

            xhr.upload.onprogress = (e) => {
                if (e.lengthComputable) {
                    const pct = Math.round((e.loaded / e.total) * 100);
                    document.getElementById('progressBar').style.width = pct + '%';
                    document.getElementById('progressPct').innerText = pct + '%';
                    document.getElementById('progressStatus').innerText = 
                        `Flashing: ${(e.loaded / 1024).toFixed(0)} KB / ${(e.total / 1024).toFixed(0)} KB`;
                }
            };

            xhr.onload = () => {
                if (xhr.status === 200) {
                    document.getElementById('progressBar').style.width = '100%';
                    document.getElementById('progressPct').innerText = '100%';
                    statusMsg.className = 'status-msg success';
                    statusMsg.innerHTML = "✨ FIRMWARE FLASHED SUCCESSFULLY!<br>Portal Gun rebooting into new dimension in <span id='cd'>5</span>s...";
                    
                    let count = 5;
                    const timer = setInterval(() => {
                        count--;
                        const cd = document.getElementById('cd');
                        if (cd) cd.innerText = count;
                        if (count <= 0) {
                            clearInterval(timer);
                            window.location.href = '/';
                        }
                    }, 1000);
                } else {
                    statusMsg.className = 'status-msg error';
                    statusMsg.innerText = "❌ Flash failed: " + (xhr.responseText || "HTTP " + xhr.status);
                    btnFlash.disabled = false;
                    btnFlash.style.opacity = '1';
                }
            };

            xhr.onerror = () => {
                statusMsg.className = 'status-msg error';
                statusMsg.innerText = "❌ Network error during transmission!";
                btnFlash.disabled = false;
                btnFlash.style.opacity = '1';
            };

            xhr.send(formData);
        }
    </script>
</body>
</html>
)rawliteral";

