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

// UI Overhaul: Dashboard header action button
// - Unfocused: Text button without solid background fill
// - Focused: High-emphasis filled elevated container (Green for positive/nav, Red for stop/down/quit)
static void draw_header_btn(int x, int y, int idx, const char *prefix, const char *key, const char *suffix, int width_chars) {
    int is_focused = (g_app.dash_focus == FOCUS_HEADER_MENU && g_app.header_selected_idx == idx);
    int is_destructive = (idx == 1 /* Down */ || idx == 4 /* Stop */ || idx == 7 /* Quit */);
    
    uint32_t bg = 0;
    uint32_t fg_text = TB_WHITE;
    uint32_t fg_key = is_destructive ? (TB_RED | TB_BOLD | TB_UNDERLINE) : (TB_GREEN | TB_BOLD | TB_UNDERLINE);

    if (is_focused) {
        bg = is_destructive ? (TB_RED | TB_BOLD) : (TB_GREEN | TB_BOLD);
        fg_text = TB_BLACK | TB_BOLD;
        fg_key = TB_BLACK | TB_BOLD | TB_UNDERLINE;
    }

    for (int i = 0; i < width_chars; i++) {
        tb_set_cell(x + i, y, ' ', fg_text, bg);
    }

    int cur_x = x + 1;
    if (prefix && strlen(prefix) > 0) {
        tb_printf(cur_x, y, fg_text, bg, "%s", prefix);
        cur_x += (int)strlen(prefix);
    }
    if (key && strlen(key) > 0) {
        tb_printf(cur_x, y, fg_key, bg, "%s", key);
        cur_x += (int)strlen(key);
    }
    if (suffix && strlen(suffix) > 0) {
        tb_printf(cur_x, y, fg_text, bg, "%s", suffix);
    }
}

// UI Overhaul: Modal dialog button
// - Unfocused: Text button / transparent on modal surface (no background fill)
//   - Primary Positive: Green bold text
//   - Primary Destructive: Red bold text
//   - Secondary / Neutral: White text
// - Focused (Selected): Elevated Filled Container
//   - Primary Positive: Bold Green fill, bold black text
//   - Primary Destructive: Bold Red fill, bold black text
//   - Secondary / Neutral: Bold White fill, bold black text
static void draw_modal_btn(int x, int y, const char *label, int is_focused, int is_primary, int is_destructive) {
    int len = (int)strlen(label);
    int width_chars = len + 2;
    uint32_t bg = TB_HI_BLACK;
    uint32_t fg_text = TB_WHITE;
    uint32_t fg_key = TB_WHITE | TB_BOLD | TB_UNDERLINE;

    if (is_focused) {
        // Material Filled Button (High-emphasis focus container)
        if (is_destructive) {
            bg = TB_RED | TB_BOLD;
        } else if (is_primary) {
            bg = TB_GREEN | TB_BOLD;
        } else {
            bg = TB_WHITE | TB_BOLD;
        }
        fg_text = TB_BLACK | TB_BOLD;
        fg_key = TB_BLACK | TB_BOLD | TB_UNDERLINE;
    } else {
        // Material Text/Tonal Button (Low-emphasis, transparent background)
        bg = TB_HI_BLACK;
        if (is_destructive) {
            fg_text = TB_RED | TB_BOLD;
            fg_key = TB_RED | TB_BOLD | TB_UNDERLINE;
        } else if (is_primary) {
            fg_text = TB_GREEN | TB_BOLD;
            fg_key = TB_GREEN | TB_BOLD | TB_UNDERLINE;
        } else {
            fg_text = TB_WHITE;
            fg_key = TB_WHITE | TB_BOLD | TB_UNDERLINE;
        }
    }

    for (int i = 0; i < width_chars; i++) {
        tb_set_cell(x + i, y, ' ', fg_text, bg);
    }

    if (len > 0) {
        tb_printf(x + 1, y, fg_key, bg, "%c", label[0]);
        if (len > 1) {
            tb_printf(x + 2, y, fg_text, bg, "%s", label + 1);
        }
    }
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

    // Row 5: Action Bar with htop-style background buttons & underlined shortcut keys
    int is_wide = (width >= 96);
    int x0 = 1;

    // Group 1: Tailnet (Up / Down)
    tb_printf(x0, 5, TB_HI_BLACK, 0, "Tailnet:");
    int btn0_x = x0 + 9;
    draw_header_btn(btn0_x, 5, 0, "", "U", "p", 4);
    int btn1_x = btn0_x + 5;
    draw_header_btn(btn1_x, 5, 1, "", "D", "own", 6);

    int div1_x = btn1_x + 7;
    tb_printf(div1_x, 5, TB_HI_BLACK, 0, "│");

    // Group 2: Tailscale (Restart / Start / Stop)
    int ts_lbl_x = div1_x + 2;
    if (is_wide) {
        tb_printf(ts_lbl_x, 5, TB_HI_BLACK, 0, "Tailscale:");
        ts_lbl_x += 11;
    } else {
        tb_printf(ts_lbl_x, 5, TB_HI_BLACK, 0, "TS:");
        ts_lbl_x += 4;
    }
    int btn2_x = ts_lbl_x;
    draw_header_btn(btn2_x, 5, 2, "", "R", "estart", 9);
    int btn3_x = btn2_x + 10;
    draw_header_btn(btn3_x, 5, 3, "", "S", "tart", 7);
    int btn4_x = btn3_x + 8;
    draw_header_btn(btn4_x, 5, 4, "S", "t", "op", 6);

    int div2_x = btn4_x + 7;
    tb_printf(div2_x, 5, TB_HI_BLACK, 0, "│");

    // Group 3: Logs
    int btn5_x = div2_x + 2;
    draw_header_btn(btn5_x, 5, 5, "", "L", "ogs", 6);

    int div3_x = btn5_x + 7;
    tb_printf(div3_x, 5, TB_HI_BLACK, 0, "│");

    // Group 4: Configuration
    int btn6_x = div3_x + 2;
    if (is_wide) {
        draw_header_btn(btn6_x, 5, 6, "", "C", "onfiguration", 15);
        btn6_x += 16;
    } else {
        draw_header_btn(btn6_x, 5, 6, "", "C", "onfig", 8);
        btn6_x += 9;
    }

    int div4_x = btn6_x;
    tb_printf(div4_x, 5, TB_HI_BLACK, 0, "│");

    // Group 5: Quit
    int btn7_x = div4_x + 2;
    draw_header_btn(btn7_x, 5, 7, "", "Q", "uit", 6);

    // Peer Table Header
    tb_printf(2, 8, TB_WHITE | TB_BOLD, 0, "Tailscale Peer & Network Status (%d peers):", g_app.peer_count);

    // Render Peers with distinctive color coding & triangle cursor
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

        if (g_app.dash_focus == FOCUS_PEERS && idx == g_app.selected_peer) {
            tb_printf(1, start_y + i, TB_CYAN | TB_BOLD, bg, "▶");
        } else {
            tb_printf(1, start_y + i, TB_WHITE, bg, " ");
        }

        tb_printf(3, start_y + i, fg, bg, "%-16s %-19s %-15s %-8s %s", p->ip, p->name, p->user, p->os, p->status);
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
    int cur = g_app.config_selected_idx;

    tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale Configuration & Service Management");
    tb_printf(2, 2, TB_HI_BLACK, 0, "Manage watchdog, routing, operating modes, logs, alerts, and binary updates.");
    for (int x = 1; x < width - 1; x++) {
        tb_printf(x, 3, TB_HI_BLACK, 0, "─");
    }

    // Section 1: DAEMON & HEALTH MONITOR
    tb_printf(2, 4, TB_WHITE | TB_BOLD, 0, "DAEMON & HEALTH MONITOR");

    tb_printf(2, 5, (cur == 0) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 0) ? "▶" : " ");
    tb_printf(4, 5, TB_GREEN | TB_BOLD, 0, "(1)");
    tb_printf(7, 5, TB_WHITE, 0, " Keepalive Watchdog Check       : ");
    if (cfg->keepalive) tb_printf(41, 5, TB_GREEN | TB_BOLD, 0, "Enabled");
    else tb_printf(41, 5, TB_HI_BLACK, 0, "Disabled");

    tb_printf(2, 6, (cur == 1) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 1) ? "▶" : " ");
    tb_printf(4, 6, TB_GREEN | TB_BOLD, 0, "(2)");
    tb_printf(7, 6, TB_WHITE, 0, " Keep Settings Persistent       : ");
    if (cfg->persistentsettings) tb_printf(41, 6, TB_GREEN | TB_BOLD, 0, "Enabled");
    else tb_printf(41, 6, TB_HI_BLACK, 0, "Disabled");

    tb_printf(2, 7, (cur == 2) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 2) ? "▶" : " ");
    tb_printf(4, 7, TB_GREEN | TB_BOLD, 0, "(3)");
    tb_printf(7, 7, TB_WHITE, 0, " Autostart on Router Boot       : ");
    if (cfg->autostart) tb_printf(41, 7, TB_GREEN | TB_BOLD, 0, "Enabled");
    else tb_printf(41, 7, TB_HI_BLACK, 0, "Disabled");

    // Section 2: TAILSCALE ROUTING & SERVICE
    tb_printf(2, 9, TB_WHITE | TB_BOLD, 0, "TAILSCALE ROUTING & OPERATING MODE");

    tb_printf(2, 10, (cur == 3) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 3) ? "▶" : " ");
    tb_printf(4, 10, TB_GREEN | TB_BOLD, 0, "(4)");
    tb_printf(7, 10, TB_WHITE, 0, " Tailscale Operating Mode       : ");
    tb_printf(41, 10, TB_CYAN | TB_BOLD, 0, "%s Mode", cfg->opmode);

    tb_printf(2, 11, (cur == 4) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 4) ? "▶" : " ");
    tb_printf(4, 11, TB_GREEN | TB_BOLD, 0, "(5)");
    tb_printf(7, 11, TB_WHITE, 0, " Advertise as Exit Node         : ");
    if (cfg->exitnode) tb_printf(41, 11, TB_GREEN | TB_BOLD, 0, "Enabled");
    else tb_printf(41, 11, TB_HI_BLACK, 0, "Disabled");

    tb_printf(2, 12, (cur == 5) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 5) ? "▶" : " ");
    tb_printf(4, 12, TB_GREEN | TB_BOLD, 0, "(6)");
    tb_printf(7, 12, TB_WHITE, 0, " Advertise Subnet Routes        : ");
    if (cfg->advroutes) {
        if (strlen(cfg->routes) > 0) tb_printf(41, 12, TB_GREEN | TB_BOLD, 0, "Enabled (%s)", cfg->routes);
        else tb_printf(41, 12, TB_GREEN | TB_BOLD, 0, "Enabled");
    } else {
        tb_printf(41, 12, TB_HI_BLACK, 0, "Disabled");
    }

    tb_printf(2, 13, (cur == 6) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 6) ? "▶" : " ");
    tb_printf(4, 13, TB_GREEN | TB_BOLD, 0, "(7)");
    tb_printf(7, 13, TB_WHITE, 0, " Edit Subnet Route CIDR         : ");
    if (strlen(cfg->routes) > 0) tb_printf(41, 13, TB_YELLOW | TB_BOLD, 0, "%s", cfg->routes);
    else tb_printf(41, 13, TB_HI_BLACK, 0, "None");

    // Section 3: INTERFACE & LOGGING
    tb_printf(2, 15, TB_WHITE | TB_BOLD, 0, "INTERFACE & LOGGING");

    tb_printf(2, 16, (cur == 7) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 7) ? "▶" : " ");
    tb_printf(4, 16, TB_GREEN | TB_BOLD, 0, "(8)");
    tb_printf(7, 16, TB_WHITE, 0, " Status Check Interval          : ");
    tb_printf(41, 16, TB_CYAN | TB_BOLD, 0, "%ds", cfg->timerloop);

    tb_printf(2, 17, (cur == 8) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 8) ? "▶" : " ");
    tb_printf(4, 17, TB_GREEN | TB_BOLD, 0, "(9)");
    tb_printf(7, 17, TB_WHITE, 0, " Event Log Retention Max        : ");
    if (cfg->logsize == 0) tb_printf(41, 17, TB_HI_BLACK, 0, "Disabled");
    else tb_printf(41, 17, TB_CYAN | TB_BOLD, 0, "%d rows", cfg->logsize);

    // Section 4: NOTIFICATIONS & AUTOMATION
    tb_printf(2, 19, TB_WHITE | TB_BOLD, 0, "NOTIFICATIONS & AUTOMATION");

    tb_printf(2, 20, (cur == 9) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 9) ? "▶" : " ");
    tb_printf(4, 20, TB_GREEN | TB_BOLD, 0, "(10)");
    tb_printf(8, 20, TB_WHITE, 0, " AMTM Email Notifications     : ");
    if (!cfg->amtmemailsuccess && !cfg->amtmemailfailure) tb_printf(41, 20, TB_HI_BLACK, 0, "Disabled");
    else if (!cfg->amtmemailsuccess && cfg->amtmemailfailure) tb_printf(41, 20, TB_GREEN | TB_BOLD, 0, "Failures only (RL: %d/h)", cfg->ratelimit);
    else if (cfg->amtmemailsuccess && !cfg->amtmemailfailure) tb_printf(41, 20, TB_GREEN | TB_BOLD, 0, "Success only (RL: %d/h)", cfg->ratelimit);
    else tb_printf(41, 20, TB_GREEN | TB_BOLD, 0, "Success & Failures (RL: %d/h)", cfg->ratelimit);

    tb_printf(2, 21, (cur == 10) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 10) ? "▶" : " ");
    tb_printf(4, 21, TB_GREEN | TB_BOLD, 0, "(11)");
    tb_printf(8, 21, TB_WHITE, 0, " Scheduled Autoupdate Track   : ");
    if (cfg->schedule) tb_printf(41, 21, TB_GREEN | TB_BOLD, 0, "Enabled @ %02d:%02d (%s)", cfg->schedulehrs, cfg->schedulemin, cfg->track ? "Beta" : "Stable");
    else tb_printf(41, 21, TB_HI_BLACK, 0, "Disabled");

    // Section 5: BINARY, MAINTENANCE & INSTALLATION
    tb_printf(2, 23, TB_WHITE | TB_BOLD, 0, "BINARY MAINTENANCE & INSTALLATION");

    tb_printf(2, 24, (cur == 11) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 11) ? "▶" : " ");
    tb_printf(4, 24, TB_GREEN | TB_BOLD, 0, "(12)");
    tb_printf(8, 24, TB_WHITE, 0, " Update Tailscale Binary      : ");
    tb_printf(41, 24, TB_WHITE, 0, "Check & Update to Latest (v%s)", cfg->tsver);

    tb_printf(2, 25, (cur == 12) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 12) ? "▶" : " ");
    tb_printf(4, 25, TB_GREEN | TB_BOLD, 0, "(13)");
    tb_printf(8, 25, TB_WHITE, 0, " Reset Daemon State / Re-login: ");
    tb_printf(41, 25, TB_YELLOW | TB_BOLD, 0, "Clear State & Re-authenticate");

    tb_printf(2, 26, (cur == 13) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 13) ? "▶" : " ");
    tb_printf(4, 26, TB_GREEN | TB_BOLD, 0, "(14)");
    tb_printf(8, 26, TB_WHITE, 0, " Reinstall Entware Tailscale  : ");
    tb_printf(41, 26, TB_CYAN | TB_BOLD, 0, "Run Entware Installer");

    tb_printf(2, 27, (cur == 14) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 14) ? "▶" : " ");
    tb_printf(4, 27, TB_GREEN | TB_BOLD, 0, "(15)");
    tb_printf(8, 27, TB_WHITE, 0, " Uninstall ZeroScale          : ");
    tb_printf(41, 27, TB_RED | TB_BOLD, 0, "Complete Removal & Cleanup");

    for (int x = 1; x < width - 1; x++) {
        tb_printf(x, height - 3, TB_HI_BLACK, 0, "─");
    }

    if (g_app.toast_expiry > time(NULL) && strlen(g_app.toast_msg) > 0) {
        tb_printf(2, height - 4, TB_YELLOW | TB_BOLD, 0, "⚡ %s", g_app.toast_msg);
    }

    tb_printf(2, height - 2, TB_WHITE, 0, "[ ↑/↓: Navigate | Enter: Toggle/Edit | Click row or press number | Esc/q: Back to Monitor ]");
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

    tb_printf(start_x + 3, start_y + 9, TB_WHITE | TB_DIM, TB_HI_BLACK, "Use ←/→ or Tab to select, Enter to execute.");

    for (int x = start_x + 1; x < start_x + box_w - 1; x++) {
        tb_printf(x, start_y + 10, TB_WHITE, TB_HI_BLACK, "─");
    }

    draw_modal_btn(start_x + 3, start_y + 11, "Ping", (g_app.peer_detail_selected_btn == 0), 1, 0);
    draw_modal_btn(start_x + 13, start_y + 11, "Tailscale Ping", (g_app.peer_detail_selected_btn == 1), 1, 0);
    draw_modal_btn(start_x + 34, start_y + 11, "Close", (g_app.peer_detail_selected_btn == 2), 0, 0);
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
    if (g_app.prev_mode == VIEW_CONFIG || g_app.mode == VIEW_INPUT) {
        draw_unified_config_view();
    } else {
        draw_dashboard();
    }

    int width = tb_width();
    int height = tb_height();

    int box_w = (width >= 74) ? 68 : (width - 4);
    int box_h = 8;
    int start_x = (width - box_w) / 2;
    int start_y = (height - box_h) / 2;

    draw_modal_box(start_x, start_y, box_w, box_h, g_app.input_title, TB_YELLOW | TB_BOLD, TB_HI_BLACK);

    tb_printf(start_x + 3, start_y + 2, TB_WHITE, TB_HI_BLACK, "%.*s:", box_w - 8, g_app.input_prompt);

    int text_focused = (g_app.input_selected_btn == 0);
    tb_printf(start_x + 3, start_y + 4, text_focused ? (TB_CYAN | TB_BOLD | TB_UNDERLINE) : (TB_WHITE | TB_BOLD), TB_HI_BLACK, "%.*s%s", box_w - 8, g_app.input_buf, text_focused ? "_" : " ");

    draw_modal_btn(start_x + 4, start_y + 6, "Save", (g_app.input_selected_btn == 1), 1, 0);
    draw_modal_btn(start_x + 14, start_y + 6, "Cancel", (g_app.input_selected_btn == 2), 0, 0);
    tb_set_cell(start_x + box_w - 1, start_y + 6, 0x2502 /* │ */, TB_YELLOW | TB_BOLD, TB_HI_BLACK);
}

static void draw_confirm_dialog(void) {
    if (g_app.prev_mode == VIEW_CONFIG) {
        draw_unified_config_view();
    } else {
        draw_dashboard();
    }

    int width = tb_width();
    int height = tb_height();

    int box_w = (width >= 70) ? 66 : (width - 4);
    int box_h = 8;
    int start_x = (width - box_w) / 2;
    int start_y = (height - box_h) / 2;

    const char *action_lbl = (strlen(g_app.confirm_action_label) > 0) ? g_app.confirm_action_label : "Confirm";
    int is_destructive = (strstr(action_lbl, "Uninstall") || strstr(action_lbl, "Stop") || strstr(action_lbl, "Reset") || strstr(action_lbl, "Disconnect")) ? 1 : 0;

    draw_modal_box(start_x, start_y, box_w, box_h, "CONFIRMATION REQUIRED", is_destructive ? (TB_RED | TB_BOLD) : (TB_YELLOW | TB_BOLD), TB_HI_BLACK);

    tb_printf(start_x + 3, start_y + 2, TB_WHITE | TB_BOLD, TB_HI_BLACK, "%.*s", box_w - 8, g_app.confirm_prompt);
    tb_printf(start_x + 3, start_y + 3, TB_WHITE | TB_DIM, TB_HI_BLACK, "Use ←/→ or Tab to select, Enter to execute.");

    // Action button & Cancel button
    int act_len = (int)strlen(action_lbl) + 2;
    int btn0_x = start_x + 4;
    int btn1_x = btn0_x + act_len + 3;

    draw_modal_btn(btn0_x, start_y + 5, action_lbl, (g_app.confirm_selected_btn == 0), 1, is_destructive);
    draw_modal_btn(btn1_x, start_y + 5, "Cancel", (g_app.confirm_selected_btn == 1), 0, 0);

    tb_set_cell(start_x + box_w - 1, start_y + 5, 0x2502 /* │ */, is_destructive ? (TB_RED | TB_BOLD) : (TB_YELLOW | TB_BOLD), TB_HI_BLACK);
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
