#pragma once
#include <Arduino.h>

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AgroSolo - Painel de Monitoramento</title>
    <style>
        :root {
            --primary: #2d6a4f;
            --primary-dark: #1b4332;
            --primary-light: #52b788;
            --accent: #d8f3dc;
            --bg: #f8fafc;
            --card-bg: #ffffff;
            --text-main: #1e293b;
            --text-muted: #64748b;
            --danger: #ef4444;
            --warning: #f59e0b;
            --success: #10b981;
            --radius: 16px;
            --shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.08), 0 8px 10px -6px rgba(0, 0, 0, 0.04);
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        }

        body {
            background-color: var(--bg);
            color: var(--text-main);
            min-height: 100vh;
            padding-bottom: 40px;
        }

        /* Top Navbar */
        .top-navbar {
            background: linear-gradient(135deg, var(--primary-dark), var(--primary));
            color: white;
            padding: 14px 20px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            box-shadow: 0 2px 10px rgba(0,0,0,0.15);
            position: sticky;
            top: 0;
            z-index: 100;
        }

        .brand-container {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .brand-icon {
            font-size: 1.6rem;
        }

        .brand-name {
            font-size: 1.25rem;
            font-weight: 800;
            letter-spacing: -0.5px;
            display: flex;
            align-items: center;
            gap: 6px;
        }

        .brand-badge {
            background: rgba(255,255,255,0.2);
            padding: 2px 8px;
            border-radius: 12px;
            font-size: 0.7rem;
            font-weight: 600;
            text-transform: uppercase;
        }

        .connection-status {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 0.85rem;
            font-weight: 600;
            background: rgba(0,0,0,0.2);
            padding: 6px 12px;
            border-radius: 20px;
        }

        .status-dot {
            width: 9px;
            height: 9px;
            border-radius: 50%;
            background-color: var(--success);
            box-shadow: 0 0 8px var(--success);
            animation: pulse 2s infinite;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; transform: scale(1); }
            50% { opacity: 0.4; transform: scale(0.85); }
        }

        /* Container */
        .container {
            max-width: 1000px;
            margin: 0 auto;
            padding: 16px;
        }

        /* Hero Banner */
        .hero-banner {
            background: linear-gradient(135deg, #1b4332 0%, #2d6a4f 100%);
            border-radius: var(--radius);
            padding: 24px;
            color: white;
            margin-bottom: 20px;
            box-shadow: var(--shadow);
            position: relative;
            overflow: hidden;
        }

        .hero-banner::after {
            content: "🌱";
            position: absolute;
            right: 15px;
            bottom: -15px;
            font-size: 7rem;
            opacity: 0.12;
            pointer-events: none;
        }

        .hero-banner h1 {
            font-size: 1.5rem;
            font-weight: 800;
            margin-bottom: 6px;
        }

        .hero-banner p {
            color: var(--accent);
            font-size: 0.9rem;
            max-width: 500px;
            line-height: 1.4;
        }

        /* Quick Action Bar */
        .action-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            flex-wrap: wrap;
            gap: 10px;
            margin-bottom: 20px;
        }

        .last-update-text {
            font-size: 0.85rem;
            color: var(--text-muted);
            font-weight: 500;
        }

        .btn {
            background: var(--primary);
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 10px;
            font-size: 0.85rem;
            font-weight: 600;
            cursor: pointer;
            display: inline-flex;
            align-items: center;
            gap: 6px;
            transition: all 0.2s;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }

        .btn:hover {
            background: var(--primary-dark);
            transform: translateY(-1px);
        }

        /* Cards Grid */
        .cards-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 16px;
            margin-bottom: 20px;
        }

        .card {
            background: var(--card-bg);
            border-radius: var(--radius);
            padding: 22px;
            box-shadow: var(--shadow);
            border-top: 4px solid var(--primary);
            position: relative;
        }

        .card.card-temp { border-top-color: #f97316; }
        .card.card-humidity { border-top-color: #0284c7; }
        .card.card-hardware { border-top-color: #8b5cf6; }

        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
        }

        .card-title {
            font-size: 0.85rem;
            font-weight: 700;
            color: var(--text-muted);
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .card-icon {
            font-size: 1.6rem;
        }

        .card-value {
            font-size: 2.8rem;
            font-weight: 800;
            line-height: 1;
            margin-bottom: 10px;
            color: var(--text-main);
        }

        .card-unit {
            font-size: 1.3rem;
            font-weight: 600;
            color: var(--text-muted);
            margin-left: 2px;
        }

        .badge {
            display: inline-block;
            padding: 4px 10px;
            border-radius: 20px;
            font-size: 0.75rem;
            font-weight: 700;
            text-transform: uppercase;
        }

        .badge-success { background: #dcfce7; color: #166534; }
        .badge-warning { background: #fef3c7; color: #92400e; }
        .badge-danger { background: #fee2e2; color: #991b1b; }
        .badge-info { background: #e0f2fe; color: #075985; }

        /* Progress Bar */
        .progress-container {
            width: 100%;
            height: 10px;
            background-color: #e2e8f0;
            border-radius: 99px;
            margin: 12px 0 8px 0;
            overflow: hidden;
        }

        .progress-bar {
            height: 100%;
            width: 0%;
            background: linear-gradient(90deg, #38bdf8, #0284c7);
            border-radius: 99px;
            transition: width 0.6s cubic-bezier(0.4, 0, 0.2, 1);
        }

        .card-footer-text {
            font-size: 0.8rem;
            color: var(--text-muted);
            margin-top: 10px;
        }

        /* History Table */
        .history-card {
            background: var(--card-bg);
            border-radius: var(--radius);
            padding: 22px;
            box-shadow: var(--shadow);
        }

        .history-card h2 {
            font-size: 1.1rem;
            font-weight: 700;
            margin-bottom: 14px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .table-responsive {
            width: 100%;
            overflow-x: auto;
        }

        table {
            width: 100%;
            border-collapse: collapse;
            font-size: 0.85rem;
        }

        th {
            text-align: left;
            padding: 10px 12px;
            background-color: #f8fafc;
            color: var(--text-muted);
            font-weight: 600;
            border-bottom: 2px solid #e2e8f0;
        }

        td {
            padding: 10px 12px;
            border-bottom: 1px solid #f1f5f9;
        }

        /* Modal QR Code */
        .modal-overlay {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0,0,0,0.65);
            z-index: 999;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }

        .modal-box {
            background: white;
            border-radius: 20px;
            padding: 24px;
            max-width: 340px;
            width: 100%;
            text-align: center;
            box-shadow: 0 20px 25px -5px rgba(0,0,0,0.3);
            animation: popIn 0.25s ease-out;
        }

        @keyframes popIn {
            from { transform: scale(0.9); opacity: 0; }
            to { transform: scale(1); opacity: 1; }
        }

        .modal-box h3 {
            color: var(--primary-dark);
            margin-bottom: 8px;
        }

        .modal-box p {
            font-size: 0.85rem;
            color: var(--text-muted);
            margin-bottom: 16px;
        }

        .qr-wrapper {
            background: #f8fafc;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
            padding: 12px;
            display: inline-block;
            margin-bottom: 14px;
        }

        .qr-wrapper img {
            width: 180px;
            height: 180px;
            display: block;
        }

        .footer {
            text-align: center;
            font-size: 0.8rem;
            color: var(--text-muted);
            margin-top: 30px;
        }
    </style>
</head>
<body>

    <!-- Top Navigation -->
    <header class="top-navbar">
        <div class="brand-container">
            <span class="brand-icon">🌱</span>
            <div class="brand-name">
                AgroSolo
                <span class="brand-badge">2P</span>
            </div>
        </div>
        <div class="connection-status">
            <span id="status-indicator" class="status-dot"></span>
            <span id="status-text">Online</span>
        </div>
    </header>

    <div class="container">
        <!-- Hero Banner -->
        <section class="hero-banner">
            <h1>Monitoramento do Solo</h1>
            <p>Dados de temperatura e umidade em tempo real direto do microcontrolador ESP32.</p>
        </section>

        <!-- Ações Rápidas -->
        <section class="action-bar">
            <span class="last-update-text">Atualizado a cada 2 segundos</span>
            <button class="btn" id="btn-open-qr">
                <span>📱</span> Compartilhar / QR Code
            </button>
        </section>

        <!-- Grade de Indicadores Principais -->
        <main class="cards-grid">
            <!-- Temperatura -->
            <article class="card card-temp">
                <div class="card-header">
                    <span class="card-title">Temperatura do Solo</span>
                    <span class="card-icon">🌡️</span>
                </div>
                <div class="card-value">
                    <span id="val-temperatura">--</span><span class="card-unit">°C</span>
                </div>
                <div>
                    <span id="badge-temperatura" class="badge badge-info">Lendo...</span>
                </div>
                <p class="card-footer-text">Sensor: Sonda DS18B20 (GPIO 4)</p>
            </article>

            <!-- Umidade -->
            <article class="card card-humidity">
                <div class="card-header">
                    <span class="card-title">Umidade do Solo</span>
                    <span class="card-icon">💧</span>
                </div>
                <div class="card-value">
                    <span id="val-umidade">--</span><span class="card-unit">%</span>
                </div>
                <div class="progress-container">
                    <div id="progress-umidade" class="progress-bar"></div>
                </div>
                <div style="margin-top: 6px;">
                    <span id="badge-umidade" class="badge badge-info">Lendo...</span>
                </div>
                <p class="card-footer-text">Higrômetro Analógico (GPIO 34)</p>
            </article>

            <!-- Diagnóstico Hardware -->
            <article class="card card-hardware">
                <div class="card-header">
                    <span class="card-title">Sinal Elétrico ADC</span>
                    <span class="card-icon">⚡</span>
                </div>
                <div class="card-value">
                    <span id="val-adc">--</span><span class="card-unit">pts</span>
                </div>
                <p style="font-weight: 600; font-size: 0.95rem; margin-top: 6px;">
                    Tensão: <span id="val-tensao">--</span> V
                </p>
                <p class="card-footer-text">Última leitura: <span id="val-timestamp">--:--:--</span></p>
            </article>
        </main>

        <!-- Histórico das últimas leituras -->
        <section class="history-card">
            <h2>
                <span>Leituras Recentes</span>
                <span id="log-count" style="font-size: 0.8rem; font-weight: normal; color: var(--text-muted);">0 registros</span>
            </h2>
            <div class="table-responsive">
                <table>
                    <thead>
                        <tr>
                            <th>Horário</th>
                            <th>Temperatura</th>
                            <th>Umidade</th>
                            <th>ADC</th>
                            <th>Status</th>
                        </tr>
                    </thead>
                    <tbody id="history-tbody">
                        <tr>
                            <td colspan="5" style="text-align: center; color: var(--text-muted); padding: 18px;">
                                Coletando primeiros dados...
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </section>

        <footer class="footer">
            <p>Projeto Bettiu 2P — AgroSolo &copy; 2026. Sistema Embarcado ESP32.</p>
        </footer>
    </div>

    <!-- Modal QR Code -->
    <div id="qr-modal" class="modal-overlay">
        <div class="modal-box">
            <h3>Acesso no Smartphone</h3>
            <p>Aponte a câmera do seu celular para abrir este painel diretamente na sua mão:</p>
            <div class="qr-wrapper">
                <img id="qr-img" src="" alt="QR Code AgroSolo">
            </div>
            <p id="qr-url-text" style="font-size: 0.75rem; font-family: monospace; color: var(--primary); word-break: break-all; margin-bottom: 16px; font-weight: bold;"></p>
            <button class="btn" id="btn-close-qr" style="width: 100%; justify-content: center;">Fechar</button>
        </div>
    </div>

    <!-- JavaScript Assíncrono com Fetch e JSON -->
    <script>
        const elTemp = document.getElementById('val-temperatura');
        const elBadgeTemp = document.getElementById('badge-temperatura');
        const elHum = document.getElementById('val-umidade');
        const elBadgeHum = document.getElementById('badge-umidade');
        const elProgressHum = document.getElementById('progress-umidade');
        const elAdc = document.getElementById('val-adc');
        const elTensao = document.getElementById('val-tensao');
        const elTimestamp = document.getElementById('val-timestamp');
        const elStatusDot = document.getElementById('status-indicator');
        const elStatusText = document.getElementById('status-text');
        const elHistoryTbody = document.getElementById('history-tbody');
        const elLogCount = document.getElementById('log-count');

        const elBtnOpenQr = document.getElementById('btn-open-qr');
        const elBtnCloseQr = document.getElementById('btn-close-qr');
        const elQrModal = document.getElementById('qr-modal');
        const elQrImg = document.getElementById('qr-img');
        const elQrUrlText = document.getElementById('qr-url-text');

        let historico = [];
        const MAX_HISTORICO = 7;

        // Modal QR Code
        elBtnOpenQr.addEventListener('click', () => {
            const currentUrl = window.location.origin;
            elQrImg.src = `https://api.qrserver.com/v1/create-qr-code/?size=220x220&data=${encodeURIComponent(currentUrl)}`;
            elQrUrlText.textContent = currentUrl;
            elQrModal.style.display = 'flex';
        });

        elBtnCloseQr.addEventListener('click', () => {
            elQrModal.style.display = 'none';
        });

        elQrModal.addEventListener('click', (e) => {
            if (e.target === elQrModal) elQrModal.style.display = 'none';
        });

        function renderizar(dados) {
            const agora = new Date().toLocaleTimeString('pt-BR');

            // 1. Temperatura
            if (dados.temperatura <= -100) {
                elTemp.textContent = "ERRO";
                elBadgeTemp.textContent = "Desconectado";
                elBadgeTemp.className = "badge badge-danger";
            } else {
                elTemp.textContent = dados.temperatura.toFixed(1);
                if (dados.temperatura > 32) {
                    elBadgeTemp.textContent = "Quente";
                    elBadgeTemp.className = "badge badge-warning";
                } else if (dados.temperatura < 15) {
                    elBadgeTemp.textContent = "Frio";
                    elBadgeTemp.className = "badge badge-info";
                } else {
                    elBadgeTemp.textContent = "Ideal";
                    elBadgeTemp.className = "badge badge-success";
                }
            }

            // 2. Umidade
            const hum = Math.max(0, Math.min(100, dados.umidade));
            elHum.textContent = hum.toFixed(1);
            elProgressHum.style.width = `${hum}%`;

            if (hum < 30) {
                elBadgeHum.textContent = "Seco (Irrigar)";
                elBadgeHum.className = "badge badge-danger";
            } else if (hum <= 70) {
                elBadgeHum.textContent = "Ideal";
                elBadgeHum.className = "badge badge-success";
            } else {
                elBadgeHum.textContent = "Encharcado";
                elBadgeHum.className = "badge badge-warning";
            }

            // 3. Hardware
            elAdc.textContent = dados.adc;
            elTensao.textContent = (dados.tensao !== undefined) ? dados.tensao.toFixed(2) : ((dados.adc / 4095) * 3.3).toFixed(2);
            elTimestamp.textContent = agora;

            // 4. Histórico
            adicionarHistorico({
                hora: agora,
                temp: dados.temperatura <= -100 ? "N/D" : `${dados.temperatura.toFixed(1)} °C`,
                umidade: `${hum.toFixed(1)} %`,
                adc: dados.adc,
                status: dados.status || (hum < 30 ? "SECO" : hum <= 70 ? "IDEAL" : "SATURADO")
            });
        }

        function adicionarHistorico(r) {
            historico.unshift(r);
            if (historico.length > MAX_HISTORICO) historico.pop();
            elLogCount.textContent = `${historico.length} leituras`;

            let html = '';
            historico.forEach(item => {
                let badgeClass = 'badge-success';
                if (item.status === 'SECO') badgeClass = 'badge-danger';
                if (item.status === 'SATURADO') badgeClass = 'badge-warning';

                html += `
                    <tr>
                        <td><strong>${item.hora}</strong></td>
                        <td>${item.temp}</td>
                        <td>${item.umidade}</td>
                        <td><code>${item.adc}</code></td>
                        <td><span class="badge ${badgeClass}">${item.status}</span></td>
                    </tr>
                `;
            });
            elHistoryTbody.innerHTML = html;
        }

        async function fetchDados() {
            try {
                // Busca o endpoint JSON relativo no próprio ESP32
                const res = await fetch('/api/data');
                if (!res.ok) throw new Error("HTTP " + res.status);
                const dados = await res.json();
                elStatusDot.style.backgroundColor = "var(--success)";
                elStatusDot.style.boxShadow = "0 0 8px var(--success)";
                elStatusText.textContent = "Online";
                renderizar(dados);
            } catch (err) {
                elStatusDot.style.backgroundColor = "var(--danger)";
                elStatusDot.style.boxShadow = "0 0 8px var(--danger)";
                elStatusText.textContent = "Desconectado";
            }
        }

        // Inicia busca
        fetchDados();
        setInterval(fetchDados, 2000);
    </script>
</body>
</html>
)rawliteral";
