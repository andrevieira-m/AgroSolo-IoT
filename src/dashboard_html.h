#pragma once
#include <Arduino.h>

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
    <title>AgroSolo - Monitoramento Inteligente de Solo</title>
    <!-- Fonte Inter para tipografia moderna e limpa estilo Finexy / FinTech -->
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">
    <!-- Chart.js para renderizacao de graficos limpos no navegador -->
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        :root {
            --bg-body: #f4f5f8;
            --bg-card: #ffffff;
            --bg-card-featured: #1b4332;
            --border: #eef0f4;
            --border-hover: #e2e5ec;
            --text-heading: #111827;
            --text-body: #4b5563;
            --text-muted: #9ca3af;
            --primary: #1b4332;
            --primary-accent: #2d6a4f;
            --badge-green-bg: #ecfdf5;
            --badge-green-text: #065f46;
            --badge-amber-bg: #fffbeb;
            --badge-amber-text: #92400e;
            --badge-red-bg: #fef2f2;
            --badge-red-text: #991b1b;
            --badge-blue-bg: #eff6ff;
            --badge-blue-text: #1e40af;
            --radius-outer: 24px;
            --radius-card: 20px;
            --radius-pill: 9999px;
            --shadow-subtle: 0 4px 20px -2px rgba(17, 24, 39, 0.03), 0 2px 6px -1px rgba(17, 24, 39, 0.02);
            --shadow-modal: 0 25px 50px -12px rgba(0, 0, 0, 0.25);
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            -webkit-font-smoothing: antialiased;
            touch-action: manipulation;
        }

        body {
            background-color: var(--bg-body);
            color: var(--text-body);
            min-height: 100vh;
            padding: 24px 20px;
        }

        .dashboard-wrapper {
            max-width: 1400px;
            margin: 0 auto;
            display: flex;
            flex-direction: column;
            gap: 20px;
        }

        /* ==========================================================================
           TOP NAVIGATION BAR (Clean, Modern Pill Header)
           ========================================================================== */
        .top-navbar {
            background: var(--bg-card);
            border-radius: var(--radius-pill);
            padding: 10px 18px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            box-shadow: var(--shadow-subtle);
            border: 1px solid var(--border);
            flex-wrap: wrap;
            gap: 14px;
        }

        .nav-brand {
            display: flex;
            align-items: center;
            gap: 10px;
            text-decoration: none;
        }

        .brand-badge {
            background: var(--primary);
            padding: 6px 14px;
            border-radius: var(--radius-pill);
            display: flex;
            align-items: center;
            justify-content: center;
        }

        .brand-badge img {
            height: 22px;
            width: auto;
            display: block;
        }

        .brand-title {
            font-weight: 700;
            font-size: 1.05rem;
            color: var(--text-heading);
            letter-spacing: -0.3px;
        }

        .brand-subtitle {
            font-size: 0.75rem;
            color: var(--text-muted);
            font-weight: 500;
            margin-left: 2px;
            padding: 2px 8px;
            background: #f3f4f6;
            border-radius: var(--radius-pill);
        }

        .nav-pills {
            display: flex;
            align-items: center;
            gap: 6px;
            background: #f8fafc;
            padding: 4px;
            border-radius: var(--radius-pill);
            border: 1px solid var(--border);
        }

        .nav-pill {
            padding: 8px 16px;
            border-radius: var(--radius-pill);
            font-size: 0.85rem;
            font-weight: 500;
            color: var(--text-body);
            text-decoration: none;
            transition: all 0.2s ease;
            cursor: pointer;
            border: none;
            background: transparent;
            white-space: nowrap;
        }

        .nav-pill.active {
            background: var(--text-heading);
            color: #ffffff;
            font-weight: 600;
        }

        .nav-pill:hover:not(.active) {
            color: var(--text-heading);
            background: #e2e8f0;
        }

        .nav-actions {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .status-pill {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            padding: 6px 14px;
            background: #f9fafb;
            border: 1px solid var(--border);
            border-radius: var(--radius-pill);
            font-size: 0.8rem;
            font-weight: 600;
            color: var(--text-heading);
            white-space: nowrap;
        }

        .pulse-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background-color: #10b981;
            box-shadow: 0 0 0 2px rgba(16, 185, 129, 0.2);
            animation: pulse-ring 2s infinite;
        }

        @keyframes pulse-ring {
            0% { transform: scale(0.95); opacity: 0.8; }
            50% { transform: scale(1.15); opacity: 1; box-shadow: 0 0 0 6px rgba(16, 185, 129, 0.25); }
            100% { transform: scale(0.95); opacity: 0.8; }
        }

        .btn-pill {
            padding: 8px 16px;
            border-radius: var(--radius-pill);
            font-size: 0.85rem;
            font-weight: 600;
            cursor: pointer;
            border: 1px solid var(--border);
            background: #ffffff;
            color: var(--text-heading);
            transition: all 0.2s ease;
            display: inline-flex;
            align-items: center;
            gap: 6px;
            white-space: nowrap;
        }

        .btn-pill:hover {
            background: #f8fafc;
            border-color: #cbd5e1;
        }

        .btn-pill-dark {
            background: var(--text-heading);
            color: #ffffff;
            border-color: var(--text-heading);
        }

        .btn-pill-dark:hover {
            background: #1f2937;
        }

        /* ==========================================================================
           HEADER / TITLE SECTION
           ========================================================================== */
        .page-header {
            display: flex;
            justify-content: space-between;
            align-items: flex-end;
            padding: 4px 6px;
            flex-wrap: wrap;
            gap: 12px;
        }

        .page-header h1 {
            font-size: clamp(1.4rem, 4vw, 1.85rem);
            font-weight: 800;
            color: var(--text-heading);
            letter-spacing: -0.6px;
        }

        .page-header p {
            font-size: clamp(0.82rem, 2.5vw, 0.95rem);
            color: var(--text-muted);
            margin-top: 3px;
            font-weight: 400;
        }

        .sync-tag {
            font-size: 0.78rem;
            color: var(--text-body);
            background: #ffffff;
            padding: 5px 12px;
            border-radius: var(--radius-pill);
            border: 1px solid var(--border);
            font-weight: 500;
            white-space: nowrap;
        }

        /* ==========================================================================
           MAIN GRID LAYOUT (3 Columns: Balance Style, 2x2 Cards, Chart)
           ========================================================================== */
        .main-grid {
            display: grid;
            grid-template-columns: 320px 1fr 1fr;
            gap: 20px;
        }

        @media (max-width: 1200px) {
            .main-grid {
                grid-template-columns: 1fr 1fr;
            }
            .main-grid > .col-chart {
                grid-column: span 2;
            }
        }

        /* ==========================================================================
           COLUMN 1: HERO SOIL BALANCE CARD (Total Balance Style from Pin)
           ========================================================================== */
        .card-balance {
            background: var(--bg-card);
            border-radius: var(--radius-card);
            padding: 24px;
            border: 1px solid var(--border);
            box-shadow: var(--shadow-subtle);
            display: flex;
            flex-direction: column;
            justify-content: space-between;
            gap: 20px;
        }

        .card-label-subtle {
            font-size: 0.8rem;
            font-weight: 600;
            color: var(--text-muted);
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .balance-value-row {
            margin-top: 6px;
        }

        .balance-large {
            font-size: clamp(1.8rem, 5vw, 2.2rem);
            font-weight: 800;
            color: var(--text-heading);
            letter-spacing: -1px;
            line-height: 1.1;
        }

        .balance-status-tag {
            display: inline-flex;
            align-items: center;
            gap: 6px;
            padding: 4px 10px;
            border-radius: var(--radius-pill);
            font-size: 0.75rem;
            font-weight: 600;
            margin-top: 8px;
            background: var(--badge-green-bg);
            color: var(--badge-green-text);
        }

        .btn-group-balance {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-top: 4px;
        }

        .btn-balance {
            padding: 11px;
            border-radius: var(--radius-pill);
            font-size: 0.85rem;
            font-weight: 600;
            cursor: pointer;
            text-align: center;
            border: none;
            transition: all 0.2s;
        }

        .btn-balance-dark {
            background: var(--text-heading);
            color: #ffffff;
        }

        .btn-balance-dark:hover {
            background: #1f2937;
        }

        .btn-balance-light {
            background: #f3f4f6;
            color: var(--text-heading);
        }

        .btn-balance-light:hover {
            background: #e5e7eb;
        }

        /* Sub-metric Pill Containers (Wallets Style from Pin) */
        .submetrics-list {
            display: flex;
            flex-direction: column;
            gap: 10px;
            border-top: 1px solid var(--border);
            padding-top: 18px;
        }

        .submetric-item {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 10px 14px;
            background: #f9fafb;
            border-radius: 14px;
            border: 1px solid var(--border);
        }

        .submetric-meta {
            display: flex;
            flex-direction: column;
        }

        .submetric-name {
            font-size: 0.75rem;
            font-weight: 500;
            color: var(--text-muted);
        }

        .submetric-val {
            font-size: 1rem;
            font-weight: 700;
            color: var(--text-heading);
        }

        /* Spending Limit / Progress Style */
        .moisture-limit-box {
            border-top: 1px solid var(--border);
            padding-top: 18px;
        }

        .moisture-header-row {
            display: flex;
            justify-content: space-between;
            font-size: 0.8rem;
            font-weight: 600;
            color: var(--text-heading);
            margin-bottom: 8px;
        }

        .progress-track {
            width: 100%;
            height: 10px;
            background: #f1f5f9;
            border-radius: var(--radius-pill);
            overflow: hidden;
            position: relative;
        }

        .progress-fill {
            height: 100%;
            background: var(--primary);
            border-radius: var(--radius-pill);
            transition: width 0.5s ease;
            width: 0%;
        }

        /* ==========================================================================
           COLUMN 2: 2x2 METRIC CARDS (Total Earnings, Spending, Income, Revenue)
           ========================================================================== */
        .metric-cards-2x2 {
            display: grid;
            grid-template-columns: 1fr 1fr;
            grid-template-rows: 1fr 1fr;
            gap: 16px;
        }

        .card-stat {
            background: var(--bg-card);
            border-radius: var(--radius-card);
            padding: 22px;
            border: 1px solid var(--border);
            box-shadow: var(--shadow-subtle);
            display: flex;
            flex-direction: column;
            justify-content: space-between;
            transition: all 0.2s ease;
        }

        .card-stat:hover {
            border-color: var(--border-hover);
            transform: translateY(-2px);
        }

        /* Featured Highlight Card (Deep Green style from orange pin) */
        .card-stat-featured {
            background: linear-gradient(145deg, #1b4332 0%, #2d6a4f 100%);
            color: #ffffff;
            border: none;
        }

        .card-stat-featured .card-label-subtle {
            color: rgba(255, 255, 255, 0.75);
        }

        .card-stat-featured .stat-number {
            color: #ffffff;
        }

        .card-stat-featured .stat-pill {
            background: rgba(255, 255, 255, 0.18);
            color: #ffffff;
        }

        .stat-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .stat-dot-indicator {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: #cbd5e1;
        }

        .card-stat-featured .stat-dot-indicator {
            background: #52b788;
        }

        .stat-body {
            margin: 12px 0 8px 0;
        }

        .stat-number {
            font-size: clamp(1.6rem, 4.5vw, 2rem);
            font-weight: 800;
            color: var(--text-heading);
            letter-spacing: -1px;
            line-height: 1;
        }

        .stat-unit {
            font-size: 0.8rem;
            font-weight: 500;
            color: var(--text-muted);
            margin-left: 3px;
        }

        .card-stat-featured .stat-unit {
            color: rgba(255, 255, 255, 0.7);
        }

        .stat-footer {
            display: flex;
            align-items: center;
            justify-content: space-between;
        }

        .stat-pill {
            display: inline-flex;
            align-items: center;
            gap: 4px;
            padding: 3px 9px;
            border-radius: var(--radius-pill);
            font-size: 0.75rem;
            font-weight: 600;
            background: #f3f4f6;
            color: var(--text-body);
        }

        .stat-pill.pill-success {
            background: var(--badge-green-bg);
            color: var(--badge-green-text);
        }

        .stat-pill.pill-warning {
            background: var(--badge-amber-bg);
            color: var(--badge-amber-text);
        }

        .stat-pill.pill-danger {
            background: var(--badge-red-bg);
            color: var(--badge-red-text);
        }

        .stat-pill.pill-info {
            background: var(--badge-blue-bg);
            color: var(--badge-blue-text);
        }

        /* ==========================================================================
           COLUMN 3: CHART CARD (Total Income Style from Pin)
           ========================================================================== */
        .card-chart {
            background: var(--bg-card);
            border-radius: var(--radius-card);
            padding: 24px;
            border: 1px solid var(--border);
            box-shadow: var(--shadow-subtle);
            display: flex;
            flex-direction: column;
            justify-content: space-between;
        }

        .chart-header {
            display: flex;
            justify-content: space-between;
            align-items: flex-start;
            margin-bottom: 16px;
            flex-wrap: wrap;
            gap: 10px;
        }

        .chart-title-group h2 {
            font-size: 1.1rem;
            font-weight: 700;
            color: var(--text-heading);
            letter-spacing: -0.3px;
        }

        .chart-title-group p {
            font-size: 0.8rem;
            color: var(--text-muted);
            margin-top: 2px;
        }

        .chart-legend {
            display: flex;
            align-items: center;
            gap: 14px;
            font-size: 0.75rem;
            font-weight: 600;
            flex-wrap: wrap;
        }

        .legend-item {
            display: flex;
            align-items: center;
            gap: 6px;
            color: var(--text-body);
        }

        .legend-color {
            width: 10px;
            height: 10px;
            border-radius: 3px;
        }

        .chart-canvas-container {
            position: relative;
            height: 240px;
            width: 100%;
        }

        /* ==========================================================================
           BOTTOM SECTION: RECENT ACTIVITIES TABLE (Recent Activities from Pin)
           ========================================================================== */
        .card-table {
            background: var(--bg-card);
            border-radius: var(--radius-card);
            padding: 24px;
            border: 1px solid var(--border);
            box-shadow: var(--shadow-subtle);
        }

        .table-header-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 16px;
            flex-wrap: wrap;
            gap: 12px;
        }

        .table-title-group h2 {
            font-size: 1.15rem;
            font-weight: 700;
            color: var(--text-heading);
            letter-spacing: -0.3px;
        }

        .table-title-group p {
            font-size: 0.8rem;
            color: var(--text-muted);
            margin-top: 2px;
        }

        .table-controls {
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .table-counter-badge {
            font-size: 0.78rem;
            font-weight: 600;
            color: var(--text-body);
            background: #f3f4f6;
            padding: 5px 12px;
            border-radius: var(--radius-pill);
        }

        .table-responsive {
            overflow-x: auto;
            -webkit-overflow-scrolling: touch;
            scrollbar-width: thin;
        }

        .activities-table {
            width: 100%;
            border-collapse: separate;
            border-spacing: 0;
            font-size: 0.85rem;
        }

        .activities-table th {
            text-align: left;
            padding: 12px 14px;
            color: var(--text-muted);
            font-weight: 600;
            font-size: 0.72rem;
            text-transform: uppercase;
            letter-spacing: 0.6px;
            border-bottom: 1px solid var(--border);
            white-space: nowrap;
        }

        .activities-table td {
            padding: 14px;
            color: var(--text-body);
            border-bottom: 1px solid #f8fafc;
            vertical-align: middle;
            font-variant-numeric: tabular-nums;
            white-space: nowrap;
        }

        .activities-table tbody tr {
            transition: background 0.15s ease;
        }

        .activities-table tbody tr:hover {
            background: #f8fafc;
        }

        .activities-table td strong {
            color: var(--text-heading);
            font-weight: 600;
        }

        .pill-status-dot {
            display: inline-flex;
            align-items: center;
            gap: 6px;
            padding: 4px 10px;
            border-radius: var(--radius-pill);
            font-size: 0.75rem;
            font-weight: 600;
        }

        .pill-status-dot::before {
            content: '';
            width: 6px;
            height: 6px;
            border-radius: 50%;
            display: inline-block;
        }

        .pill-status-dot.ok {
            background: var(--badge-green-bg);
            color: var(--badge-green-text);
        }
        .pill-status-dot.ok::before {
            background: #10b981;
        }

        .pill-status-dot.warn {
            background: var(--badge-amber-bg);
            color: var(--badge-amber-text);
        }
        .pill-status-dot.warn::before {
            background: #f59e0b;
        }

        /* ==========================================================================
           MODAL QR CODE (Minimalist & Clean)
           ========================================================================== */
        .modal-backdrop {
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(17, 24, 39, 0.45);
            backdrop-filter: blur(8px);
            display: none;
            align-items: center;
            justify-content: center;
            z-index: 1000;
            padding: 20px;
        }

        .modal-card {
            background: #ffffff;
            border-radius: 24px;
            padding: 28px 24px;
            max-width: 380px;
            width: 100%;
            text-align: center;
            box-shadow: var(--shadow-modal);
            border: 1px solid var(--border);
        }

        .modal-card h3 {
            font-size: 1.25rem;
            font-weight: 700;
            color: var(--text-heading);
            letter-spacing: -0.3px;
        }

        .modal-card p {
            font-size: 0.85rem;
            color: var(--text-muted);
            margin: 6px 0 18px 0;
        }

        .qr-frame {
            background: #f9fafb;
            padding: 14px;
            border-radius: 18px;
            border: 1px solid var(--border);
            display: inline-block;
            margin-bottom: 16px;
        }

        .qr-frame img {
            width: 190px;
            height: 190px;
            display: block;
            border-radius: 8px;
        }

        .modal-url {
            font-family: monospace;
            font-size: 0.78rem;
            color: var(--primary);
            font-weight: 600;
            margin-bottom: 18px;
            word-break: break-all;
            background: #f8fafc;
            padding: 8px 12px;
            border-radius: 10px;
            border: 1px solid var(--border);
        }

        .modal-btn-group {
            display: flex;
            flex-direction: column;
            gap: 10px;
        }

        /* ==========================================================================
           RESPONSIVIDADE DEDICADA PARA DISPOSITIVOS MÓVEIS (iOS & Android)
           ========================================================================== */
        @media (max-width: 768px) {
            body {
                padding: 12px 10px calc(24px + env(safe-area-inset-bottom)) 10px;
            }

            .dashboard-wrapper {
                gap: 14px;
            }

            /* Top Navbar em telas móveis */
            .top-navbar {
                border-radius: 18px;
                padding: 10px 14px;
                gap: 10px;
            }

            .brand-badge {
                padding: 5px 12px;
            }

            .brand-badge img {
                height: 19px;
            }

            .brand-title {
                font-size: 0.98rem;
            }

            .brand-subtitle {
                display: none; /* Oculta etiqueta secundária para economizar espaço horizontal */
            }

            .nav-actions {
                gap: 8px;
                margin-left: auto;
            }

            .status-pill {
                padding: 5px 10px;
                font-size: 0.72rem;
            }

            .btn-pill {
                padding: 6px 12px;
                font-size: 0.78rem;
                min-height: 38px;
            }

            /* Navegação de pílulas com scroll horizontal suave */
            .nav-pills {
                width: 100%;
                overflow-x: auto;
                -webkit-overflow-scrolling: touch;
                scrollbar-width: none;
                justify-content: flex-start;
                padding: 3px;
                margin-top: 2px;
            }

            .nav-pills::-webkit-scrollbar {
                display: none;
            }

            .nav-pill {
                padding: 7px 14px;
                font-size: 0.78rem;
                flex-shrink: 0;
            }

            /* Cabeçalho da página */
            .page-header {
                padding: 2px 4px;
            }

            .sync-tag {
                font-size: 0.72rem;
                padding: 4px 10px;
            }

            /* Grade principal: uma coluna vertical no celular */
            .main-grid {
                grid-template-columns: 1fr;
                gap: 14px;
            }

            .main-grid > .col-chart {
                grid-column: span 1;
            }

            /* Cartão de Condição Geral */
            .card-balance {
                padding: 18px 16px;
                border-radius: 18px;
                gap: 16px;
            }

            .balance-status-tag {
                font-size: 0.72rem;
            }

            .btn-balance {
                min-height: 42px;
                font-size: 0.82rem;
            }

            .submetric-item {
                padding: 9px 12px;
            }

            /* Grid 2x2 dos Nutrientes */
            .metric-cards-2x2 {
                gap: 10px;
            }

            .card-stat {
                padding: 16px 14px;
                border-radius: 16px;
            }

            .card-stat .stat-body {
                margin: 8px 0 6px 0;
            }

            .stat-pill {
                font-size: 0.7rem;
                padding: 3px 8px;
            }

            /* Cartão do Gráfico */
            .card-chart {
                padding: 18px 16px;
                border-radius: 18px;
            }

            .chart-canvas-container {
                height: 210px;
            }

            .chart-legend {
                gap: 10px;
                font-size: 0.7rem;
            }

            /* Tabela de Histórico */
            .card-table {
                padding: 18px 14px;
                border-radius: 18px;
            }

            .activities-table th,
            .activities-table td {
                padding: 11px 10px;
                font-size: 0.78rem;
            }
        }

        @media (max-width: 400px) {
            .metric-cards-2x2 {
                grid-template-columns: 1fr; /* Coluna única para celulares muito compactos (ex: iPhone SE) */
            }
            .card-stat .stat-number {
                font-size: 1.7rem;
            }
        }
    </style>
</head>
<body>

    <div class="dashboard-wrapper">

        <!-- 1. TOP NAVBAR (Inspired by Finexy pill bar) -->
        <header class="top-navbar">
            <div class="nav-brand">
                <div class="brand-badge">
                    <img src="data:image/png;base64,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" alt="AgroSolo Logo" onerror="this.src='img/logo-white.png'">
                </div>
                <span class="brand-title">AgroSolo</span>
                <span class="brand-subtitle">Bettiu 2P</span>
            </div>

            <div class="nav-actions">
                <div class="status-pill">
                    <span id="pulse-dot" class="pulse-dot"></span>
                    <span id="status-text">RS-485 Online</span>
                </div>
                <button class="btn-pill" id="btn-open-qr">
                    Celular
                </button>
            </div>

            <!-- Abas com scroll horizontal para navegação tátil fluida -->
            <nav class="nav-pills">
                <button class="nav-pill active" onclick="filtrarVisao('geral', event)">Visao Geral</button>
                <button class="nav-pill" onclick="filtrarVisao('nutrientes', event)">Nutrientes NPK</button>
                <button class="nav-pill" onclick="filtrarVisao('grafico', event)">Graficos</button>
                <button class="nav-pill" onclick="filtrarVisao('historico', event)">Historico</button>
            </nav>
        </header>

        <!-- 2. PAGE HEADER -->
        <section class="page-header">
            <div>
                <h1>Monitoramento de Solo</h1>
                <p>Analise multivariada em tempo real com Sensor Industrial 7 em 1.</p>
            </div>
            <div class="sync-tag">
                Sincronizacao a cada 2s
            </div>
        </section>

        <!-- 3. MAIN DASHBOARD GRID (Balance Card, 2x2 Stats, Chart) -->
        <main class="main-grid">

            <!-- COL 1: BALANCE STYLE (Status Geral + Umidade + Temperatura) -->
            <section class="card-balance" id="secao-geral">
                <div>
                    <div class="card-label-subtle">Condicao Geral do Solo</div>
                    <div class="balance-value-row">
                        <div id="soil-general-status" class="balance-large">Equilibrado</div>
                        <div id="soil-status-badge" class="balance-status-tag">
                            Faixa Ideal de Cultivo
                        </div>
                    </div>
                </div>

                <div class="btn-group-balance">
                    <button class="btn-balance btn-balance-dark" onclick="fetchDados()">Atualizar</button>
                    <button class="btn-balance btn-balance-light" id="btn-balance-qr">QR Code</button>
                </div>

                <!-- Submetrics (Wallets Style from Pin) -->
                <div class="submetrics-list">
                    <div class="submetric-item">
                        <div class="submetric-meta">
                            <span class="submetric-name">Umidade do Solo</span>
                            <span id="sub-umidade-val" class="submetric-val">-- %</span>
                        </div>
                        <span id="sub-umidade-pill" class="stat-pill pill-success">Adequado</span>
                    </div>

                    <div class="submetric-item">
                        <div class="submetric-meta">
                            <span class="submetric-name">Temperatura do Solo</span>
                            <span id="sub-temp-val" class="submetric-val">-- °C</span>
                        </div>
                        <span id="sub-temp-pill" class="stat-pill pill-success">Adequado</span>
                    </div>

                    <div class="submetric-item">
                        <div class="submetric-meta">
                            <span class="submetric-name">pH do Solo</span>
                            <span id="sub-ph-val" class="submetric-val">--</span>
                        </div>
                        <span id="sub-ph-pill" class="stat-pill pill-success">Neutro</span>
                    </div>
                </div>

                <!-- Spending Limit Style Progress Bar -->
                <div class="moisture-limit-box">
                    <div class="moisture-header-row">
                        <span>Nivel Hidrico do Solo</span>
                        <span id="moisture-ratio-text">-- / 100%</span>
                    </div>
                    <div class="progress-track">
                        <div id="moisture-progress-bar" class="progress-fill"></div>
                    </div>
                </div>
            </section>

            <!-- COL 2: 2x2 STATS (Nitrogenio, Fosforo, Potassio, Condutividade EC) -->
            <section class="metric-cards-2x2" id="secao-nutrientes">

                <!-- Nitrogenio (N) - Featured Card (Deep Green style from orange pin) -->
                <article class="card-stat card-stat-featured">
                    <div class="stat-header">
                        <span class="card-label-subtle">Nitrogenio (N)</span>
                        <span class="stat-dot-indicator"></span>
                    </div>
                    <div class="stat-body">
                        <div class="stat-number">
                            <span id="val-n">--</span><span class="stat-unit">mg/kg</span>
                        </div>
                    </div>
                    <div class="stat-footer">
                        <span id="badge-n" class="stat-pill">Lendo...</span>
                        <span class="card-label-subtle" style="font-size: 0.72rem;">Vegetativo</span>
                    </div>
                </article>

                <!-- Fosforo (P) -->
                <article class="card-stat">
                    <div class="stat-header">
                        <span class="card-label-subtle">Fosforo (P)</span>
                        <span class="stat-dot-indicator"></span>
                    </div>
                    <div class="stat-body">
                        <div class="stat-number">
                            <span id="val-p">--</span><span class="stat-unit">mg/kg</span>
                        </div>
                    </div>
                    <div class="stat-footer">
                        <span id="badge-p" class="stat-pill pill-info">Lendo...</span>
                        <span class="card-label-subtle" style="font-size: 0.72rem;">Radicular</span>
                    </div>
                </article>

                <!-- Potassio (K) -->
                <article class="card-stat">
                    <div class="stat-header">
                        <span class="card-label-subtle">Potassio (K)</span>
                        <span class="stat-dot-indicator"></span>
                    </div>
                    <div class="stat-body">
                        <div class="stat-number">
                            <span id="val-k">--</span><span class="stat-unit">mg/kg</span>
                        </div>
                    </div>
                    <div class="stat-footer">
                        <span id="badge-k" class="stat-pill pill-info">Lendo...</span>
                        <span class="card-label-subtle" style="font-size: 0.72rem;">Resistencia</span>
                    </div>
                </article>

                <!-- Condutividade (EC) -->
                <article class="card-stat">
                    <div class="stat-header">
                        <span class="card-label-subtle">Condutividade (EC)</span>
                        <span class="stat-dot-indicator"></span>
                    </div>
                    <div class="stat-body">
                        <div class="stat-number">
                            <span id="val-ec">--</span><span class="stat-unit">uS/cm</span>
                        </div>
                    </div>
                    <div class="stat-footer">
                        <span id="badge-ec" class="stat-pill pill-success">Lendo...</span>
                        <span class="card-label-subtle" style="font-size: 0.72rem;">Salinidade</span>
                    </div>
                </article>
            </section>

            <!-- COL 3: CHART CARD (Total Income style from Pin) -->
            <section class="card-chart col-chart" id="secao-grafico">
                <div class="chart-header">
                    <div class="chart-title-group">
                        <h2>Variacao Temporal</h2>
                        <p>Tendencia continua dos ultimos minutos</p>
                    </div>
                    <div class="chart-legend">
                        <div class="legend-item">
                            <span class="legend-color" style="background: #10b981;"></span>
                            <span>Umidade</span>
                        </div>
                        <div class="legend-item">
                            <span class="legend-color" style="background: #1e293b;"></span>
                            <span>Temperatura</span>
                        </div>
                        <div class="legend-item">
                            <span class="legend-color" style="background: #3b82f6;"></span>
                            <span>pH</span>
                        </div>
                    </div>
                </div>

                <div class="chart-canvas-container">
                    <canvas id="soilMainChart"></canvas>
                </div>
            </section>
        </main>

        <!-- 4. BOTTOM SECTION: RECENT ACTIVITIES TABLE (Table Style from Pin) -->
        <section class="card-table" id="secao-historico">
            <div class="table-header-row">
                <div class="table-title-group">
                    <h2>Historico Recente</h2>
                    <p>Registros cronologicos decodificados do barramento RS-485</p>
                </div>
                <div class="table-controls">
                    <span id="table-counter" class="table-counter-badge">Aguardando dados...</span>
                </div>
            </div>

            <div class="table-responsive">
                <table class="activities-table">
                    <thead>
                        <tr>
                            <th>Horario</th>
                            <th>Nitrogenio (N)</th>
                            <th>Fosforo (P)</th>
                            <th>Potassio (K)</th>
                            <th>pH Solo</th>
                            <th>Condutividade (EC)</th>
                            <th>Umidade</th>
                            <th>Temperatura</th>
                            <th>Status</th>
                        </tr>
                    </thead>
                    <tbody id="history-tbody">
                        <tr>
                            <td colspan="9" style="text-align: center; color: var(--text-muted); padding: 24px;">
                                Conectando ao ESP32 e aguardando telemetria...
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </section>

    </div>

    <!-- MODAL QR CODE / COMPARTILHAMENTO -->
    <div id="qr-modal" class="modal-backdrop">
        <div class="modal-card">
            <h3>Acesso no Celular</h3>
            <p>Escaneie o QR Code ou copie o endereco IP local:</p>
            <div class="qr-frame">
                <img id="qr-img" src="" alt="QR Code AgroSolo">
            </div>
            <div id="qr-url-text" class="modal-url"></div>
            <div class="modal-btn-group">
                <button class="btn-pill" id="btn-copy-url" style="width: 100%; justify-content: center;">
                    Copiar Endereco
                </button>
                <button class="btn-pill btn-pill-dark" id="btn-close-qr" style="width: 100%; justify-content: center;">
                    Fechar
                </button>
            </div>
        </div>
    </div>

    <!-- SCRIPT DE TELEMETRIA E RENDERIZACAO CLIENT-SIDE -->
    <script>
        const elStatusGeneral = document.getElementById('soil-general-status');
        const elStatusBadge = document.getElementById('soil-status-badge');
        const elSubUmidVal = document.getElementById('sub-umidade-val');
        const elSubUmidPill = document.getElementById('sub-umidade-pill');
        const elSubTempVal = document.getElementById('sub-temp-val');
        const elSubTempPill = document.getElementById('sub-temp-pill');
        const elSubPhVal = document.getElementById('sub-ph-val');
        const elSubPhPill = document.getElementById('sub-ph-pill');

        const elMoistureRatio = document.getElementById('moisture-ratio-text');
        const elMoistureBar = document.getElementById('moisture-progress-bar');

        const elValN = document.getElementById('val-n');
        const elBadgeN = document.getElementById('badge-n');
        const elValP = document.getElementById('val-p');
        const elBadgeP = document.getElementById('badge-p');
        const elValK = document.getElementById('val-k');
        const elBadgeK = document.getElementById('badge-k');
        const elValEc = document.getElementById('val-ec');
        const elBadgeEc = document.getElementById('badge-ec');

        const elPulseDot = document.getElementById('pulse-dot');
        const elStatusText = document.getElementById('status-text');
        const elHistoryTbody = document.getElementById('history-tbody');
        const elTableCounter = document.getElementById('table-counter');

        const elBtnOpenQr = document.getElementById('btn-open-qr');
        const elBtnBalanceQr = document.getElementById('btn-balance-qr');
        const elBtnCloseQr = document.getElementById('btn-close-qr');
        const elBtnCopyUrl = document.getElementById('btn-copy-url');
        const elQrModal = document.getElementById('qr-modal');
        const elQrImg = document.getElementById('qr-img');
        const elQrUrlText = document.getElementById('qr-url-text');

        let soilChartInstance = null;
        let historico = [];
        const MAX_HISTORICO = 8;
        const MAX_PONTOS_GRAFICO = 20;

        // Modal QR Code
        function abrirQrModal() {
            const url = window.location.origin;
            elQrImg.src = "https://api.qrserver.com/v1/create-qr-code/?size=220x220&data=" + encodeURIComponent(url);
            elQrUrlText.textContent = url;
            elQrModal.style.display = 'flex';
        }

        elBtnOpenQr.addEventListener('click', abrirQrModal);
        elBtnBalanceQr.addEventListener('click', abrirQrModal);
        elBtnCloseQr.addEventListener('click', () => {
            elQrModal.style.display = 'none';
        });
        elQrModal.addEventListener('click', (e) => {
            if (e.target === elQrModal) elQrModal.style.display = 'none';
        });

        elBtnCopyUrl.addEventListener('click', async () => {
            try {
                await navigator.clipboard.writeText(window.location.origin);
                elBtnCopyUrl.textContent = 'Copiado!';
                setTimeout(() => {
                    elBtnCopyUrl.textContent = 'Copiar Endereco';
                }, 2000);
            } catch (err) {
                elBtnCopyUrl.textContent = 'Erro ao copiar';
            }
        });

        function filtrarVisao(secao, event) {
            const navPills = document.querySelectorAll('.nav-pill');
            navPills.forEach(p => p.classList.remove('active'));
            if (event && event.target) {
                event.target.classList.add('active');
            }

            if (secao === 'historico') {
                document.getElementById('secao-historico').scrollIntoView({ behavior: 'smooth' });
            } else if (secao === 'grafico') {
                document.getElementById('secao-grafico').scrollIntoView({ behavior: 'smooth' });
            } else if (secao === 'nutrientes') {
                document.getElementById('secao-nutrientes').scrollIntoView({ behavior: 'smooth' });
            } else {
                window.scrollTo({ top: 0, behavior: 'smooth' });
            }
        }

        // Inicializacao do Grafico Chart.js
        function inicializarGrafico() {
            const ctx = document.getElementById('soilMainChart').getContext('2d');
            soilChartInstance = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [
                        {
                            label: 'Umidade (%)',
                            borderColor: '#10b981',
                            backgroundColor: 'rgba(16, 185, 129, 0.08)',
                            fill: true,
                            borderWidth: 2.2,
                            tension: 0.35,
                            pointRadius: 0,
                            pointHoverRadius: 4,
                            yAxisID: 'y'
                        },
                        {
                            label: 'Temperatura (°C)',
                            borderColor: '#1e293b',
                            backgroundColor: 'transparent',
                            borderWidth: 2,
                            tension: 0.35,
                            pointRadius: 0,
                            pointHoverRadius: 4,
                            yAxisID: 'y'
                        },
                        {
                            label: 'pH',
                            borderColor: '#3b82f6',
                            backgroundColor: 'transparent',
                            borderWidth: 1.8,
                            borderDash: [4, 4],
                            tension: 0.35,
                            pointRadius: 0,
                            pointHoverRadius: 4,
                            yAxisID: 'yPh'
                        }
                    ]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { display: false },
                        tooltip: {
                            backgroundColor: '#111827',
                            padding: 10,
                            cornerRadius: 8,
                            titleFont: { size: 11, family: 'Inter' },
                            bodyFont: { size: 11, family: 'Inter' }
                        }
                    },
                    scales: {
                        x: {
                            grid: { display: false },
                            ticks: { font: { size: 10, family: 'Inter' }, color: '#9ca3af' }
                        },
                        y: {
                            grid: { color: '#f1f5f9', borderDash: [3, 3] },
                            ticks: { font: { size: 10, family: 'Inter' }, color: '#9ca3af' }
                        },
                        yPh: {
                            position: 'right',
                            min: 0,
                            max: 14,
                            grid: { display: false },
                            ticks: { display: false }
                        }
                    }
                }
            });
        }

        function atualizarGrafico(hora, hum, temp, ph) {
            if (!soilChartInstance) return;
            soilChartInstance.data.labels.push(hora);
            soilChartInstance.data.datasets[0].data.push(hum);
            soilChartInstance.data.datasets[1].data.push(temp);
            soilChartInstance.data.datasets[2].data.push(ph);

            if (soilChartInstance.data.labels.length > MAX_PONTOS_GRAFICO) {
                soilChartInstance.data.labels.shift();
                soilChartInstance.data.datasets[0].data.shift();
                soilChartInstance.data.datasets[1].data.shift();
                soilChartInstance.data.datasets[2].data.shift();
            }
            soilChartInstance.update('none');
        }

        function renderizar(dados) {
            const agora = new Date().toLocaleTimeString('pt-BR', { hour: '2-digit', minute: '2-digit', second: '2-digit' });

            const hum = dados.umidade !== undefined ? Math.max(0, Math.min(100, dados.umidade)) : 0;
            const temp = dados.temperatura !== undefined ? dados.temperatura : 0;
            const ph = dados.ph !== undefined ? dados.ph : 7.0;
            const ec = dados.ec !== undefined ? dados.ec : 0;
            const n = dados.n !== undefined ? dados.n : 0;
            const p = dados.p !== undefined ? dados.p : 0;
            const k = dados.k !== undefined ? dados.k : 0;

            // 1. Visao Geral (Balance Column)
            elSubUmidVal.textContent = hum.toFixed(1) + ' %';
            elSubTempVal.textContent = temp.toFixed(1) + ' °C';
            elSubPhVal.textContent = ph.toFixed(1);

            // Progress Bar de Umidade
            elMoistureRatio.textContent = `${hum.toFixed(1)}% / 100%`;
            elMoistureBar.style.width = `${hum}%`;

            // Status Global
            if (hum < 25) {
                elStatusGeneral.textContent = 'Solo Seco';
                elStatusBadge.textContent = 'Necessita Irrigacao';
                elStatusBadge.className = 'balance-status-tag';
                elStatusBadge.style.background = 'var(--badge-red-bg)';
                elStatusBadge.style.color = 'var(--badge-red-text)';
                elSubUmidPill.textContent = 'Critico';
                elSubUmidPill.className = 'stat-pill pill-danger';
            } else if (hum > 80) {
                elStatusGeneral.textContent = 'Encharcado';
                elStatusBadge.textContent = 'Suspender Irrigacao';
                elStatusBadge.className = 'balance-status-tag';
                elStatusBadge.style.background = 'var(--badge-amber-bg)';
                elStatusBadge.style.color = 'var(--badge-amber-text)';
                elSubUmidPill.textContent = 'Elevado';
                elSubUmidPill.className = 'stat-pill pill-warning';
            } else {
                elStatusGeneral.textContent = 'Equilibrado';
                elStatusBadge.textContent = 'Faixa Ideal de Cultivo';
                elStatusBadge.className = 'balance-status-tag';
                elStatusBadge.style.background = 'var(--badge-green-bg)';
                elStatusBadge.style.color = 'var(--badge-green-text)';
                elSubUmidPill.textContent = 'Adequado';
                elSubUmidPill.className = 'stat-pill pill-success';
            }

            if (temp < 15) {
                elSubTempPill.textContent = 'Frio';
                elSubTempPill.className = 'stat-pill pill-info';
            } else if (temp > 35) {
                elSubTempPill.textContent = 'Quente';
                elSubTempPill.className = 'stat-pill pill-warning';
            } else {
                elSubTempPill.textContent = 'Ideal';
                elSubTempPill.className = 'stat-pill pill-success';
            }

            if (ph < 5.8) {
                elSubPhPill.textContent = 'Acido';
                elSubPhPill.className = 'stat-pill pill-warning';
            } else if (ph > 7.4) {
                elSubPhPill.textContent = 'Alcalino';
                elSubPhPill.className = 'stat-pill pill-info';
            } else {
                elSubPhPill.textContent = 'Neutro';
                elSubPhPill.className = 'stat-pill pill-success';
            }

            // 2. 2x2 Stats Cards
            elValN.textContent = n;
            elBadgeN.textContent = n < 20 ? 'Baixo' : (n <= 60 ? 'Adequado' : 'Elevado');

            elValP.textContent = p;
            elBadgeP.textContent = p < 15 ? 'Baixo' : (p <= 45 ? 'Adequado' : 'Elevado');

            elValK.textContent = k;
            elBadgeK.textContent = k < 30 ? 'Baixo' : (k <= 80 ? 'Adequado' : 'Elevado');

            elValEc.textContent = ec;
            elBadgeEc.textContent = ec < 500 ? 'Nao Salino' : (ec <= 1500 ? 'Moderado' : 'Salino');

            // 3. Grafico
            atualizarGrafico(agora, hum, temp, ph);

            // 4. Tabela de Historico
            adicionarHistorico({
                hora: agora,
                n: n,
                p: p,
                k: k,
                ph: ph.toFixed(1),
                ec: ec,
                umidade: `${hum.toFixed(1)} %`,
                temp: `${temp.toFixed(1)} °C`,
                status: (hum >= 25 && hum <= 80 && ph >= 5.5 && ph <= 7.8) ? 'Normal' : 'Atencao'
            });
        }

        function adicionarHistorico(item) {
            historico.unshift(item);
            if (historico.length > MAX_HISTORICO) historico.pop();
            elTableCounter.textContent = `${historico.length} leituras recentes`;

            let html = '';
            historico.forEach(r => {
                const statusClass = r.status === 'Normal' ? 'ok' : 'warn';
                html += `
                    <tr>
                        <td><strong>${r.hora}</strong></td>
                        <td>${r.n} mg/kg</td>
                        <td>${r.p} mg/kg</td>
                        <td>${r.k} mg/kg</td>
                        <td>${r.ph}</td>
                        <td>${r.ec} uS/cm</td>
                        <td>${r.umidade}</td>
                        <td>${r.temp}</td>
                        <td><span class="pill-status-dot ${statusClass}">${r.status}</span></td>
                    </tr>
                `;
            });
            elHistoryTbody.innerHTML = html;
        }

        async function fetchDados() {
            try {
                const res = await fetch('/api/data');
                if (!res.ok) throw new Error('HTTP ' + res.status);
                const dados = await res.json();
                elPulseDot.style.backgroundColor = '#10b981';
                elPulseDot.style.boxShadow = '0 0 0 2px rgba(16, 185, 129, 0.2)';
                elStatusText.textContent = 'RS-485 Online';
                renderizar(dados);
            } catch (err) {
                elPulseDot.style.backgroundColor = '#ef4444';
                elPulseDot.style.boxShadow = '0 0 0 2px rgba(239, 68, 68, 0.2)';
                elStatusText.textContent = 'Offline';
            }
        }

        window.addEventListener('DOMContentLoaded', () => {
            inicializarGrafico();
            fetchDados();
            setInterval(fetchDados, 2000);
        });
    </script>
</body>
</html>)rawliteral";
