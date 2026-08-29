#include "app.h"

static const char *ascii_logo[] = {
    "    ______  ______ ____   ____ ",
    "   /___  / / ____// __ \\ / __ \\",
    "     / /  / __/  / /_/ // / / /",
    "   / /__ / /___ / _, _// /_/ / ",
    " /_____//_____//_/ |_| \\____/  ",
    "   _____ ______ ___    __    ______",
    "  / ___// ____//   |  / /   / ____/",
    "  \\__ \\/ /    / /| | / /   / __/   ",
    " ___/ / /___ / ___ |/ /___/ /___   ",
    "/____/\\____//_/  |_/_____/_____/"
};

void show_splash(const char *status_msg, int duration_ms, uint32_t color) {
    tb_clear();
    int width = tb_width();
    int height = tb_height();

    int logo_rows = 10;
    int logo_block_w = 35;
    int total_block_h = logo_rows + 4;

    int start_y = (height - total_block_h) / 2;
    if (start_y < 1) start_y = 1;

    int block_start_x = (width - logo_block_w) / 2;
    if (block_start_x < 1) block_start_x = 1;

    for (int i = 0; i < logo_rows; i++) {
        tb_printf(block_start_x, start_y + i, color, 0, "%s", ascii_logo[i]);
    }

    // Version
    char ver_buf[32];
    snprintf(ver_buf, sizeof(ver_buf), "v%s", g_app.config.version);
    int ver_x = (width - (int)strlen(ver_buf)) / 2;
    if (ver_x < 1) ver_x = 1;
    tb_printf(ver_x, start_y + logo_rows + 1, TB_WHITE, 0, "%s", ver_buf);

    // Centered Status Message Badge
    if (status_msg && strlen(status_msg) > 0) {
        char badge_buf[64];
        snprintf(badge_buf, sizeof(badge_buf), "[ %s ]", status_msg);
        int badge_x = (width - (int)strlen(badge_buf)) / 2;
        if (badge_x < 1) badge_x = 1;
        tb_printf(badge_x, start_y + logo_rows + 3, TB_GREEN | TB_BOLD, 0, "%s", badge_buf);
    }

    tb_present();
    if (duration_ms > 0) {
        usleep(duration_ms * 1000);
    }
}

// Helper to draw clean unicode bordered modal boxes
static void draw_modal_box(int x, int y, int w, int h, const char *title, uint32_t border_fg, uint32_t bg) {
    for (int r = y; r < y + h; r++) {
        for (int c = x; c < x + w; c++) {
            tb_set_cell(c, r, ' ', TB_DEFAULT, bg);
        }
    }

    // Top border
    tb_set_cell(x, y, 0x250C /* ┌ */, border_fg, bg);
    for (int c = x + 1; c < x + w - 1; c++) {
        tb_set_cell(c, y, 0x2500 /* ─ */, border_fg, bg);
    }
    tb_set_cell(x + w - 1, y, 0x2510 /* ┐ */, border_fg, bg);

    // Left & Right side borders
    for (int r = y + 1; r < y + h - 1; r++) {
        tb_set_cell(x, r, 0x2502 /* │ */, border_fg, bg);
        tb_set_cell(x + w - 1, r, 0x2502 /* │ */, border_fg, bg);
    }

    // Bottom border
    tb_set_cell(x, y + h - 1, 0x2514 /* └ */, border_fg, bg);
    for (int c = x + 1; c < x + w - 1; c++) {
        tb_set_cell(c, y + h - 1, 0x2500 /* ─ */, border_fg, bg);
    }
    tb_set_cell(x + w - 1, y + h - 1, 0x2518 /* ┘ */, border_fg, bg);

    // Title on top border
    if (title && strlen(title) > 0) {
        tb_printf(x + 2, y, border_fg | TB_BOLD, bg, " %s ", title);
    }
}

// Helper to draw a safely bounded label-value row inside a modal without overflowing
static void draw_modal_row(int start_x, int y, int box_w, const char *label, const char *val, uint32_t val_fg, uint32_t bg) {
    int max_val_len = box_w - 22;
    if (max_val_len < 10) max_val_len = 10;

    tb_printf(start_x + 3, y, TB_WHITE, bg, "%-15s: ", label);
    tb_printf(start_x + 20, y, val_fg, bg, "%.*s", max_val_len, val);
    tb_set_cell(start_x + box_w - 1, y, 0x2502 /* │ */, TB_CYAN | TB_BOLD, bg);
}

static void draw_dashboard(void) {
    int width = tb_width();
    int height = tb_height();
    AppConfig *cfg = &g_app.config;

    // Top Header Banner
    tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale v%s ── Live Monitor", cfg->version);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%a %b %d, %Y %H:%M:%S", tm_info);
    tb_printf(width - 28, 1, TB_WHITE, 0, "%s", time_buf);

    // Row 2: Service & Tailnet
    tb_printf(2, 2, TB_WHITE, 0, "Service: [ ");
    if (cfg->daemon_running) tb_printf(13, 2, TB_GREEN | TB_BOLD, 0, "Started");
    else tb_printf(13, 2, TB_RED | TB_BOLD, 0, "Stopped");
    tb_printf(21, 2, TB_WHITE, 0, "] (v%s)", cfg->tsver);

    tb_printf(35, 2, TB_HI_BLACK, 0, "│");
    tb_printf(37, 2, TB_WHITE, 0, "Tailnet: [ ");
    if (cfg->tailnet_connected) tb_printf(48, 2, TB_GREEN | TB_BOLD, 0, "Connected");
    else tb_printf(48, 2, TB_RED | TB_BOLD, 0, "Disconnected");
    tb_printf(58, 2, TB_WHITE, 0, "] (%s Mode)", cfg->opmode);

    // Row 3: Watchdog, Exit Node, Routes
    tb_printf(2, 3, TB_WHITE, 0, "Watchdog: [ ");
    if (cfg->keepalive) tb_printf(14, 3, TB_GREEN | TB_BOLD, 0, "Active");
    else tb_printf(14, 3, TB_HI_BLACK, 0, "Off   ");
    tb_printf(21, 3, TB_WHITE, 0, "] (%ds loop)", cfg->timerloop);

    tb_printf(35, 3, TB_HI_BLACK, 0, "│");
    tb_printf(37, 3, TB_WHITE, 0, "Exit Node: [ ");
    if (cfg->exitnode) tb_printf(50, 3, TB_GREEN | TB_BOLD, 0, "Yes");
    else tb_printf(50, 3, TB_HI_BLACK, 0, "No ");
    tb_printf(54, 3, TB_WHITE, 0, "]  │ Routes: [ ");
    if (cfg->advroutes) {
        tb_printf(69, 3, TB_GREEN | TB_BOLD, 0, "%s", cfg->routes);
        tb_printf(69 + (int)strlen(cfg->routes), 3, TB_WHITE, 0, " ]");
    } else {
        tb_printf(69, 3, TB_HI_BLACK, 0, "No");
        tb_printf(72, 3, TB_WHITE, 0, " ]");
    }

    // Dividers
    for (int x = 1; x < width - 1; x++) {
        tb_printf(x, 4, TB_HI_BLACK, 0, "─");
        tb_printf(x, 6, TB_HI_BLACK, 0, "─");
    }

    // Row 5: Action Bar
    tb_printf(2, 5, TB_GREEN | TB_BOLD, 0, "(U)p");
    tb_printf(6, 5, TB_WHITE, 0, "/");
    tb_printf(7, 5, TB_GREEN | TB_BOLD, 0, "(D)own");
    tb_printf(14, 5, TB_HI_BLACK, 0, "│");

    tb_printf(17, 5, TB_GREEN | TB_BOLD, 0, "(R)estart");
    tb_printf(26, 5, TB_WHITE, 0, "/");
    tb_printf(27, 5, TB_GREEN | TB_BOLD, 0, "(S)tart");
    tb_printf(34, 5, TB_WHITE, 0, "/");
    tb_printf(35, 5, TB_GREEN | TB_BOLD, 0, "S(t)op");
    tb_printf(43, 5, TB_HI_BLACK, 0, "│");

    tb_printf(46, 5, TB_GREEN | TB_BOLD, 0, "(L)ogs");
    tb_printf(53, 5, TB_HI_BLACK, 0, "│");

    tb_printf(56, 5, TB_GREEN | TB_BOLD, 0, "(C)onfiguration");
    tb_printf(73, 5, TB_HI_BLACK, 0, "│");

    tb_printf(76, 5, TB_GREEN | TB_BOLD, 0, "(Q)uit");

    // Peer Table Header
    tb_printf(2, 8, TB_WHITE | TB_BOLD, 0, "Tailscale Peer & Network Status (%d peers):", g_app.peer_count);

    // Render Peers with distinctive color coding
    int start_y = 10;
    int max_rows = height - start_y - 3;
    if (max_rows < 1) max_rows = 1;

    for (int i = 0; i < max_rows; i++) {
        int idx = i + g_app.peer_scroll;
        if (idx >= g_app.peer_count) break;
        PeerInfo *p = &g_app.peers[idx];

        uint32_t fg = TB_WHITE;
        if (!p->is_online) {
            fg = TB_HI_BLACK;
        } else if (p->is_self) {
            fg = TB_MAGENTA | TB_BOLD;
        } else if (p->is_exit) {
            fg = TB_YELLOW | TB_BOLD;
        } else if (p->is_active) {
            fg = TB_GREEN | TB_BOLD;
        } else if (p->is_idle) {
            fg = TB_CYAN;
        }

        // Selection highlight
        uint32_t bg = 0;
        if (idx == g_app.selected_peer) {
            bg = TB_HI_BLACK;
        }

        tb_printf(2, start_y + i, fg, bg, "%-16s %-19s %-15s %-8s %s", p->ip, p->name, p->user, p->os, p->status);
    }

    // Scroll Indicator
    if (g_app.peer_count > max_rows) {
        tb_printf(width - 14, 8, TB_YELLOW, 0, "[▼ %d more]", g_app.peer_count - (g_app.peer_scroll + max_rows));
    }

    // Toast Notification Bar
    if (g_app.toast_expiry > time(NULL) && strlen(g_app.toast_msg) > 0) {
        tb_printf(2, height - 3, TB_YELLOW | TB_BOLD, 0, "⚡ %s", g_app.toast_msg);
    }

    // Bottom Status & Timer
    int pct = cfg->timerloop > 0 ? ((cfg->timerloop - g_app.countdown) * 100 / cfg->timerloop) : 0;
    tb_printf(2, height - 2, TB_WHITE, 0, "[ %02ds / %03d%% ]  [Click buttons or peers | Enter: Inspect Peer | Option+Drag: Copy | q: Quit]", g_app.countdown, pct);
}

static void draw_unified_config_view(void) {
    int width = tb_width();
    int height = tb_height();
    AppConfig *cfg = &g_app.config;

    tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale Configuration & Service Management");

    tb_printf(2, 2, TB_HI_BLACK, 0, "Manage watchdog, routing, operating modes, logs, alerts, and binary updates.");
    for (int x = 1; x < width - 1; x++) {
        tb_printf(x, 3, TB_HI_BLACK, 0, "─");
    }

    // Section 1: DAEMON & HEALTH MONITOR
    tb_printf(2, 4, TB_WHITE | TB_BOLD, 0, "DAEMON & HEALTH MONITOR");

    tb_printf(4, 5, TB_GREEN | TB_BOLD, 0, "(1) Keepalive Watchdog Check       : ");
    if (cfg->keepalive) tb_printf(41, 5, TB_GREEN | TB_BOLD, 0, "[ Enabled ]");
    else tb_printf(41, 5, TB_HI_BLACK, 0, "[ Disabled ]");

    tb_printf(4, 6, TB_GREEN | TB_BOLD, 0, "(2) Keep Settings Persistent       : ");
    if (cfg->persistentsettings) tb_printf(41, 6, TB_GREEN | TB_BOLD, 0, "[ Enabled ]");
    else tb_printf(41, 6, TB_HI_BLACK, 0, "[ Disabled ]");

    tb_printf(4, 7, TB_GREEN | TB_BOLD, 0, "(3) Autostart on Router Boot       : ");
    if (cfg->autostart) tb_printf(41, 7, TB_GREEN | TB_BOLD, 0, "[ Enabled ]");
    else tb_printf(41, 7, TB_HI_BLACK, 0, "[ Disabled ]");

    // Section 2: TAILSCALE ROUTING & SERVICE
    tb_printf(2, 9, TB_WHITE | TB_BOLD, 0, "TAILSCALE ROUTING & OPERATING MODE");

    tb_printf(4, 10, TB_GREEN | TB_BOLD, 0, "(4) Tailscale Operating Mode       : ");
    tb_printf(41, 10, TB_CYAN | TB_BOLD, 0, "[ %s Mode ] (Click to toggle)", cfg->opmode);

    tb_printf(4, 11, TB_GREEN | TB_BOLD, 0, "(5) Advertise as Exit Node         : ");
    if (cfg->exitnode) tb_printf(41, 11, TB_GREEN | TB_BOLD, 0, "[ Enabled ]");
    else tb_printf(41, 11, TB_HI_BLACK, 0, "[ Disabled ]");

    tb_printf(4, 12, TB_GREEN | TB_BOLD, 0, "(6) Advertise Subnet Routes        : ");
    if (cfg->advroutes) tb_printf(41, 12, TB_GREEN | TB_BOLD, 0, "[ Enabled ] (%s)", cfg->routes);
    else tb_printf(41, 12, TB_HI_BLACK, 0, "[ Disabled ]");

    tb_printf(4, 13, TB_GREEN | TB_BOLD, 0, "(7) Edit Subnet Route CIDR         : ");
    tb_printf(41, 13, TB_YELLOW | TB_BOLD, 0, "[ %s ] (Click to edit)", cfg->routes);

    // Section 3: INTERFACE & LOGGING
    tb_printf(2, 15, TB_WHITE | TB_BOLD, 0, "INTERFACE & LOGGING");

    tb_printf(4, 16, TB_GREEN | TB_BOLD, 0, "(8) Status Check Interval          : ");
    tb_printf(41, 16, TB_CYAN | TB_BOLD, 0, "[ %ds ] (Click to cycle)", cfg->timerloop);

    tb_printf(4, 17, TB_GREEN | TB_BOLD, 0, "(9) Event Log Retention Max        : ");
    if (cfg->logsize == 0) tb_printf(41, 17, TB_HI_BLACK, 0, "[ Disabled ]");
    else tb_printf(41, 17, TB_CYAN | TB_BOLD, 0, "[ %d rows ] (Click to edit)", cfg->logsize);

    // Section 4: NOTIFICATIONS & AUTOMATION
    tb_printf(2, 19, TB_WHITE | TB_BOLD, 0, "NOTIFICATIONS & AUTOMATION");

    tb_printf(4, 20, TB_GREEN | TB_BOLD, 0, "(10) AMTM Email Notifications      : ");
    if (!cfg->amtmemailsuccess && !cfg->amtmemailfailure) tb_printf(41, 20, TB_HI_BLACK, 0, "[ Disabled ]");
    else if (!cfg->amtmemailsuccess && cfg->amtmemailfailure) tb_printf(41, 20, TB_GREEN | TB_BOLD, 0, "[ Failures only ] (RL: %d/h)", cfg->ratelimit);
    else if (cfg->amtmemailsuccess && !cfg->amtmemailfailure) tb_printf(41, 20, TB_GREEN | TB_BOLD, 0, "[ Success only ] (RL: %d/h)", cfg->ratelimit);
    else tb_printf(41, 20, TB_GREEN | TB_BOLD, 0, "[ Success & Failures ] (RL: %d/h)", cfg->ratelimit);

    tb_printf(4, 21, TB_GREEN | TB_BOLD, 0, "(11) Scheduled Autoupdate Track    : ");
    if (cfg->schedule) tb_printf(41, 21, TB_GREEN | TB_BOLD, 0, "[ Enabled @ %02d:%02d (%s) ]", cfg->schedulehrs, cfg->schedulemin, cfg->track ? "Beta" : "Stable");
    else tb_printf(41, 21, TB_HI_BLACK, 0, "[ Disabled ]");

    // Section 5: BINARY, MAINTENANCE & INSTALLATION
    tb_printf(2, 23, TB_WHITE | TB_BOLD, 0, "BINARY MAINTENANCE & INSTALLATION");

    tb_printf(4, 24, TB_GREEN | TB_BOLD, 0, "(12) Update Tailscale Binary       : ");
    tb_printf(41, 24, TB_WHITE, 0, "[ Check & Update to Latest (v%s) ]", cfg->tsver);

    tb_printf(4, 25, TB_GREEN | TB_BOLD, 0, "(13) Reset Daemon State / Re-login : ");
    tb_printf(41, 25, TB_YELLOW | TB_BOLD, 0, "[ Clear State & Re-authenticate ]");

    tb_printf(4, 26, TB_GREEN | TB_BOLD, 0, "(14) Reinstall Entware Tailscale   : ");
    tb_printf(41, 26, TB_CYAN | TB_BOLD, 0, "[ Run Entware Installer ]");

    tb_printf(4, 27, TB_GREEN | TB_BOLD, 0, "(15) Uninstall ZeroScale           : ");
    tb_printf(41, 27, TB_RED | TB_BOLD, 0, "[ Complete Removal & Cleanup ]");

    for (int x = 1; x < width - 1; x++) {
        tb_printf(x, height - 3, TB_HI_BLACK, 0, "─");
    }

    if (g_app.toast_expiry > time(NULL) && strlen(g_app.toast_msg) > 0) {
        tb_printf(2, height - 4, TB_YELLOW | TB_BOLD, 0, "⚡ %s", g_app.toast_msg);
    }

    tb_printf(2, height - 2, TB_WHITE, 0, "[ Click row or press number to toggle/edit | Esc/q: Back to Monitor ]");
}

static void draw_peer_detail_modal(void) {
    draw_dashboard();

    int width = tb_width();
    int height = tb_height();

    if (g_app.selected_peer < 0 || g_app.selected_peer >= g_app.peer_count) {
        g_app.mode = VIEW_DASHBOARD;
        return;
    }

    PeerInfo *p = &g_app.peers[g_app.selected_peer];

    int box_w = (width >= 86) ? 80 : (width - 4);
    if (box_w < 60) box_w = 60;
    int box_h = 13;
    int start_x = (width - box_w) / 2;
    int start_y = (height - box_h) / 2;

    draw_modal_box(start_x, start_y, box_w, box_h, "PEER NODE INSPECTOR", TB_CYAN | TB_BOLD, TB_HI_BLACK);

    draw_modal_row(start_x, start_y + 2, box_w, "Device Name", p->name, TB_GREEN | TB_BOLD, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 3, box_w, "Tailscale IP", p->ip, TB_CYAN | TB_BOLD, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 4, box_w, "OS / Platform", p->os, TB_WHITE | TB_BOLD, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 5, box_w, "Owner / User", p->user, TB_WHITE, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 6, box_w, "Connection", p->relay_info, p->is_direct ? (TB_GREEN | TB_BOLD) : TB_YELLOW, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 7, box_w, "Tailnet Status", p->status, p->is_online ? (TB_GREEN | TB_BOLD) : TB_HI_BLACK, TB_HI_BLACK);

    for (int x = start_x + 1; x < start_x + box_w - 1; x++) {
        tb_printf(x, start_y + 9, TB_WHITE, TB_HI_BLACK, "─");
    }

    tb_printf(start_x + 3, start_y + 11, TB_GREEN | TB_BOLD, TB_HI_BLACK, "[p] Ping Node");
    tb_printf(start_x + 22, start_y + 11, TB_CYAN | TB_BOLD, TB_HI_BLACK, "[t] Tailscale Ping");
    tb_printf(start_x + 48, start_y + 11, TB_WHITE, TB_HI_BLACK, "[c / Esc] Close");
    tb_set_cell(start_x + box_w - 1, start_y + 11, 0x2502 /* │ */, TB_CYAN | TB_BOLD, TB_HI_BLACK);
}

static void draw_logs_view(void) {
    int width = tb_width();
    int height = tb_height();

    tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale Event Log Viewer (Total: %d lines)", g_app.log_count);

    for (int x = 1; x < width - 1; x++) {
        tb_printf(x, 2, TB_HI_BLACK, 0, "─");
        tb_printf(x, height - 3, TB_HI_BLACK, 0, "─");
    }

    int start_y = 3;
    int max_rows = height - 6;
    if (max_rows < 1) max_rows = 1;

    for (int i = 0; i < max_rows; i++) {
        int idx = i + g_app.log_scroll;
        if (idx >= g_app.log_count) break;

        char *line = g_app.log_lines[idx];
        uint32_t fg = TB_WHITE;
        if (strstr(line, "WARN") || strstr(line, "FAIL")) fg = TB_RED | TB_BOLD;
        else if (strstr(line, "INFO")) fg = TB_CYAN;
        else if (strstr(line, "SUCCESS") || strstr(line, "ONLINE")) fg = TB_GREEN;

        tb_printf(2, start_y + i, fg, 0, "%.*s", width - 4, line);
    }

    tb_printf(2, height - 2, TB_WHITE, 0, "Line %d-%d of %d  [↑/↓: Scroll | g/G: Top/Bottom | r: Refresh | Esc/q: Back]", 
              g_app.log_scroll + 1, 
              (g_app.log_scroll + max_rows > g_app.log_count ? g_app.log_count : g_app.log_scroll + max_rows),
              g_app.log_count);
}

static void draw_input_modal(void) {
    draw_dashboard();

    int width = tb_width();
    int height = tb_height();

    int box_w = (width >= 74) ? 68 : (width - 4);
    int box_h = 7;
    int start_x = (width - box_w) / 2;
    int start_y = (height - box_h) / 2;

    draw_modal_box(start_x, start_y, box_w, box_h, g_app.input_title, TB_YELLOW | TB_BOLD, TB_HI_BLACK);

    tb_printf(start_x + 3, start_y + 2, TB_WHITE, TB_HI_BLACK, "%.*s:", box_w - 8, g_app.input_prompt);
    tb_printf(start_x + 3, start_y + 4, TB_CYAN | TB_BOLD | TB_UNDERLINE, TB_HI_BLACK, "%.*s_", box_w - 8, g_app.input_buf);

    tb_printf(start_x + 3, start_y + 5, TB_GREEN | TB_BOLD, TB_HI_BLACK, "[Enter] Save");
    tb_printf(start_x + 28, start_y + 5, TB_WHITE, TB_HI_BLACK, "[Esc] Cancel");
    tb_set_cell(start_x + box_w - 1, start_y + 5, 0x2502 /* │ */, TB_YELLOW | TB_BOLD, TB_HI_BLACK);
}

static void draw_confirm_dialog(void) {
    draw_dashboard();

    int width = tb_width();
    int height = tb_height();

    int box_w = (width >= 70) ? 66 : (width - 4);
    int box_h = 7;
    int start_x = (width - box_w) / 2;
    int start_y = (height - box_h) / 2;

    draw_modal_box(start_x, start_y, box_w, box_h, "CONFIRMATION REQUIRED", TB_RED | TB_BOLD, TB_HI_BLACK);

    tb_printf(start_x + 3, start_y + 2, TB_YELLOW | TB_BOLD, TB_HI_BLACK, "%.*s", box_w - 8, g_app.confirm_prompt);
    tb_printf(start_x + 3, start_y + 5, TB_GREEN | TB_BOLD, TB_HI_BLACK, "[y] Confirm Action");
    tb_printf(start_x + 30, start_y + 5, TB_WHITE, TB_HI_BLACK, "[n / Esc] Cancel");
    tb_set_cell(start_x + box_w - 1, start_y + 5, 0x2502 /* │ */, TB_RED | TB_BOLD, TB_HI_BLACK);
}

void ui_draw(void) {
    tb_clear();

    switch (g_app.mode) {
        case VIEW_DASHBOARD:
            draw_dashboard();
            break;
        case VIEW_CONFIG:
            draw_unified_config_view();
            break;
        case VIEW_LOGS:
            draw_logs_view();
            break;
        case VIEW_PEER_DETAIL:
            draw_peer_detail_modal();
            break;
        case VIEW_INPUT:
            draw_input_modal();
            break;
        case VIEW_CONFIRM:
            draw_confirm_dialog();
            break;
        default:
            draw_dashboard();
            break;
    }

    tb_present();
}
