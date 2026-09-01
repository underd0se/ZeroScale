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

// Helper to get a concise status keyword on small/compact screens
static const char *get_compact_peer_status(const PeerInfo *p) {
    if (p->is_self) return "Self";
    if (!p->is_online) return "offline";
    if (p->is_exit) return "exit node";
    if (p->is_active) return "active";
    if (p->is_idle) return "idle";
    return "online";
}

// Helper to draw a safely bounded label-value row inside a modal without overflowing
static void draw_modal_row(int start_x, int y, int box_w, const char *label, const char *val, uint32_t val_fg, uint32_t bg) {
    int lbl_w = (box_w < 55) ? 11 : 15;
    int val_x = start_x + lbl_w + 5;
    int max_val_len = box_w - lbl_w - 7;
    if (max_val_len < 6) max_val_len = 6;

    tb_printf(start_x + 3, y, TB_WHITE, bg, "%-*s: ", lbl_w, label);
    tb_printf(val_x, y, val_fg, bg, "%.*s", max_val_len, val);
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
    int is_compact = (width < 80);
    int is_wide = (width >= 96);

    // Row 1: Top Header Banner
    if (g_app.mock_mode) {
        if (is_compact) {
            tb_printf(2, 1, TB_MAGENTA | TB_BOLD, 0, "ZeroScale v%s [MOCK]", cfg->version);
        } else {
            tb_printf(2, 1, TB_MAGENTA | TB_BOLD, 0, "ZeroScale v%s ── %s (%s) [MOCK SIMULATOR]", cfg->version, cfg->router_model, cfg->router_firmware);
        }
    } else {
        if (strlen(cfg->router_model) > 0 && strcmp(cfg->router_model, "Asus Router") != 0) {
            if (is_compact) {
                tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale v%s ── %s", cfg->version, cfg->router_model);
            } else if (strlen(cfg->router_firmware) > 0) {
                tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale v%s ── %s (%s)", cfg->version, cfg->router_model, cfg->router_firmware);
            } else {
                tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale v%s ── %s", cfg->version, cfg->router_model);
            }
        } else {
            tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale v%s ── Live Monitor", cfg->version);
        }
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[64];
    if (is_compact) {
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
        tb_printf(width - 10, 1, TB_WHITE, 0, "%s", time_buf);
    } else {
        strftime(time_buf, sizeof(time_buf), "%a %b %d, %Y %H:%M:%S", tm_info);
        tb_printf(width - 28, 1, TB_WHITE, 0, "%s", time_buf);
    }

    // Row 2: Service & Tailnet
    if (is_compact) {
        tb_printf(2, 2, TB_WHITE, 0, "Svc: [");
        if (cfg->daemon_running) tb_printf(8, 2, TB_GREEN | TB_BOLD, 0, "Started");
        else tb_printf(8, 2, TB_RED | TB_BOLD, 0, "Stopped");
        tb_printf(15, 2, TB_WHITE, 0, "] v%s", cfg->tsver);

        tb_printf(26, 2, TB_HI_BLACK, 0, "│");
        tb_printf(28, 2, TB_WHITE, 0, "Tailnet: [");
        if (cfg->tailnet_connected) tb_printf(38, 2, TB_GREEN | TB_BOLD, 0, "OK");
        else tb_printf(38, 2, TB_RED | TB_BOLD, 0, "Off");
        tb_printf(41, 2, TB_WHITE, 0, "] (%s)", cfg->opmode);
    } else {
        tb_printf(2, 2, TB_WHITE, 0, "Service: [ ");
        if (cfg->daemon_running) tb_printf(13, 2, TB_GREEN | TB_BOLD, 0, "Started");
        else tb_printf(13, 2, TB_RED | TB_BOLD, 0, "Stopped");
        tb_printf(21, 2, TB_WHITE, 0, "] (v%s)", cfg->tsver);

        tb_printf(35, 2, TB_HI_BLACK, 0, "│");
        tb_printf(37, 2, TB_WHITE, 0, "Tailnet: [ ");
        if (cfg->tailnet_connected) tb_printf(48, 2, TB_GREEN | TB_BOLD, 0, "Connected");
        else tb_printf(48, 2, TB_RED | TB_BOLD, 0, "Disconnected");
        tb_printf(58, 2, TB_WHITE, 0, "] (%s Mode)", cfg->opmode);
    }

    // Row 3: Watchdog, Exit Node, Routes
    if (is_compact) {
        tb_printf(2, 3, TB_WHITE, 0, "Dog: [");
        if (cfg->keepalive) tb_printf(8, 3, TB_GREEN | TB_BOLD, 0, "On ");
        else tb_printf(8, 3, TB_HI_BLACK, 0, "Off");
        tb_printf(11, 3, TB_WHITE, 0, "] (%ds)", cfg->timerloop);

        tb_printf(20, 3, TB_HI_BLACK, 0, "│");
        tb_printf(22, 3, TB_WHITE, 0, "Exit: [");
        if (cfg->exitnode) tb_printf(29, 3, TB_GREEN | TB_BOLD, 0, "Yes");
        else tb_printf(29, 3, TB_HI_BLACK, 0, "No ");
        tb_printf(32, 3, TB_WHITE, 0, "] │ Rts: [");
        if (cfg->advroutes) {
            int max_r_len = width - 46;
            if (max_r_len < 4) max_r_len = 4;
            tb_printf(42, 3, TB_GREEN | TB_BOLD, 0, "%.*s", max_r_len, cfg->routes);
            tb_printf(42 + (int)strlen(cfg->routes) < width - 2 ? 42 + (int)strlen(cfg->routes) : width - 3, 3, TB_WHITE, 0, "]");
        } else {
            tb_printf(42, 3, TB_HI_BLACK, 0, "No]");
        }
    } else {
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
    }

    // Dividers
    for (int x = 1; x < width - 1; x++) {
        tb_printf(x, 4, TB_HI_BLACK, 0, "─");
        tb_printf(x, 6, TB_HI_BLACK, 0, "─");
    }

    // Row 5: Action Bar with htop-style background buttons
    if (is_compact) {
        int x0 = 1;
        draw_header_btn(x0, 5, 0, "", "U", "p", 4);
        draw_header_btn(x0 + 5, 5, 1, "", "D", "own", 6);
        tb_printf(x0 + 12, 5, TB_HI_BLACK, 0, "│");

        draw_header_btn(x0 + 14, 5, 2, "", "R", "est", 6);
        draw_header_btn(x0 + 21, 5, 3, "", "S", "tart", 7);
        draw_header_btn(x0 + 29, 5, 4, "S", "t", "op", 6);
        tb_printf(x0 + 36, 5, TB_HI_BLACK, 0, "│");

        draw_header_btn(x0 + 38, 5, 5, "", "L", "og", 5);
        tb_printf(x0 + 44, 5, TB_HI_BLACK, 0, "│");

        draw_header_btn(x0 + 46, 5, 6, "", "C", "fg", 5);
        tb_printf(x0 + 52, 5, TB_HI_BLACK, 0, "│");

        draw_header_btn(x0 + 54, 5, 7, "", "Q", "uit", 6);
    } else {
        int x0 = 1;
        tb_printf(x0, 5, TB_HI_BLACK, 0, "Tailnet:");
        int btn0_x = x0 + 9;
        draw_header_btn(btn0_x, 5, 0, "", "U", "p", 4);
        int btn1_x = btn0_x + 5;
        draw_header_btn(btn1_x, 5, 1, "", "D", "own", 6);

        int div1_x = btn1_x + 7;
        tb_printf(div1_x, 5, TB_HI_BLACK, 0, "│");

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

        int btn5_x = div2_x + 2;
        draw_header_btn(btn5_x, 5, 5, "", "L", "ogs", 6);

        int div3_x = btn5_x + 7;
        tb_printf(div3_x, 5, TB_HI_BLACK, 0, "│");

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

        int btn7_x = div4_x + 2;
        draw_header_btn(btn7_x, 5, 7, "", "Q", "uit", 6);
    }

    // Peer Table Header
    if (strlen(g_app.peer_filter) > 0) {
        tb_printf(2, 8, TB_WHITE | TB_BOLD, 0, "Tailscale Peer & Network Status (%d/%d peers):", g_app.filtered_count, g_app.peer_count);
    } else {
        tb_printf(2, 8, TB_WHITE | TB_BOLD, 0, "Tailscale Peer & Network Status (%d peers):", g_app.peer_count);
    }

    // Render Peers with distinctive color coding & triangle cursor
    int start_y = 10;
    int max_rows = height - start_y - 3;
    if (max_rows < 1) max_rows = 1;

    for (int i = 0; i < max_rows; i++) {
        int row_idx = i + g_app.peer_scroll;
        if (row_idx >= g_app.filtered_count) break;
        int peer_idx = g_app.filtered_indices[row_idx];
        PeerInfo *p = &g_app.peers[peer_idx];

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
        if (row_idx == g_app.selected_peer) {
            bg = TB_HI_BLACK;
        }

        if (g_app.dash_focus == FOCUS_PEERS && row_idx == g_app.selected_peer) {
            tb_printf(1, start_y + i, TB_CYAN | TB_BOLD, bg, "▶");
        } else {
            tb_printf(1, start_y + i, TB_WHITE, bg, " ");
        }

        if (width >= 86) {
            tb_printf(3, start_y + i, fg, bg, "%-16s %-19s %-15s %-8s %s", p->ip, p->name, p->user, p->os, p->status);
        } else if (width >= 70) {
            const char *st = get_compact_peer_status(p);
            tb_printf(3, start_y + i, fg, bg, "%-15s %-18s %-8s %s", p->ip, p->name, p->os, st);
        } else {
            const char *st = get_compact_peer_status(p);
            tb_printf(3, start_y + i, fg, bg, "%-15s %-14s %s", p->ip, p->name, st);
        }
    }

    // Scroll Indicator
    if (g_app.filtered_count > max_rows) {
        tb_printf(width - 14, 8, TB_YELLOW, 0, "[▼ %d more]", g_app.filtered_count - (g_app.peer_scroll + max_rows));
    }

    // Toast Notification Bar / Active Filter Search Bar
    if (g_app.peer_filter_active) {
        tb_printf(2, height - 3, TB_BLACK, TB_CYAN, " Search Peers: [ %s█ ]  (Press Enter to confirm, Esc to cancel) ", g_app.peer_filter);
    } else if (g_app.toast_expiry > time(NULL) && strlen(g_app.toast_msg) > 0) {
        tb_printf(2, height - 3, TB_YELLOW | TB_BOLD, 0, "⚡ %s", g_app.toast_msg);
    } else if (strlen(g_app.peer_filter) > 0) {
        tb_printf(2, height - 3, TB_CYAN, 0, "Filtered by \"%s\" (%d matches) ── Press / to edit, Esc to clear", g_app.peer_filter, g_app.filtered_count);
    }

    // Bottom Status & Timer
    int pct = cfg->timerloop > 0 ? ((cfg->timerloop - g_app.countdown) * 100 / cfg->timerloop) : 0;
    if (is_compact) {
        tb_printf(2, height - 2, TB_WHITE, 0, "[ %02ds / %02d%% ] [Enter: View | /: Search | +/-: Timer | q: Quit]", g_app.countdown, pct);
    } else {
        tb_printf(2, height - 2, TB_WHITE, 0, "[ %02ds / %03d%% ]  [Enter: Inspect | /: Search | o: Sort | +/-: Timer (%ds) | %s: Copy | q: Quit]", g_app.countdown, pct, cfg->timerloop, g_app.copy_hint);
    }
}

static void draw_unified_config_view(void) {
    int width = tb_width();
    int height = tb_height();
    AppConfig *cfg = &g_app.config;
    int cur = g_app.config_selected_idx;
    int is_2col = (width >= 86);
    int is_compact = (width < 80);

    // Header
    if (is_compact) {
        tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale Configuration");
        tb_printf(2, 2, TB_HI_BLACK, 0, "Manage services, routing, logs, and settings.");
    } else {
        tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale Configuration & Service Management");
        tb_printf(2, 2, TB_HI_BLACK, 0, "Manage watchdog, routing, operating modes, logs, alerts, and binary updates.");
    }
    for (int x = 1; x < width - 1; x++) {
        tb_printf(x, 3, TB_HI_BLACK, 0, "─");
    }

    if (is_2col) {
        // =========================================================================
        // 2-COLUMN CARD LAYOUT (Fits all 16 items in only 18 rows!)
        // =========================================================================
        int left_vcol = 26;
        int center_x = 47;
        int right_x = 49;
        int right_vcol = 72;
        int max_left_vlen = center_x - left_vcol - 1;
        int max_right_vlen = width - right_vcol - 2;
        if (max_right_vlen < 6) max_right_vlen = 6;

        // Vertical Center Divider
        for (int y = 4; y <= 17; y++) {
            tb_printf(center_x, y, TB_HI_BLACK, 0, "│");
        }

        // --- LEFT COLUMN ---
        // Section 1: DAEMON & HEALTH
        tb_printf(2, 4, TB_WHITE | TB_BOLD, 0, "DAEMON & HEALTH");

        tb_printf(2, 5, (cur == 0) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 0) ? "▶" : " ");
        tb_printf(4, 5, TB_GREEN | TB_BOLD, 0, "(1)");
        tb_printf(7, 5, TB_WHITE, 0, " Watchdog Check : ");
        if (cfg->keepalive) tb_printf(left_vcol, 5, TB_GREEN | TB_BOLD, 0, "Enabled");
        else tb_printf(left_vcol, 5, TB_HI_BLACK, 0, "Disabled");

        tb_printf(2, 6, (cur == 1) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 1) ? "▶" : " ");
        tb_printf(4, 6, TB_GREEN | TB_BOLD, 0, "(2)");
        tb_printf(7, 6, TB_WHITE, 0, " Persistent Conf: ");
        if (cfg->persistentsettings) tb_printf(left_vcol, 6, TB_GREEN | TB_BOLD, 0, "Enabled");
        else tb_printf(left_vcol, 6, TB_HI_BLACK, 0, "Disabled");

        tb_printf(2, 7, (cur == 2) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 2) ? "▶" : " ");
        tb_printf(4, 7, TB_GREEN | TB_BOLD, 0, "(3)");
        tb_printf(7, 7, TB_WHITE, 0, " Autostart Boot : ");
        if (cfg->autostart) tb_printf(left_vcol, 7, TB_GREEN | TB_BOLD, 0, "Enabled");
        else tb_printf(left_vcol, 7, TB_HI_BLACK, 0, "Disabled");

        // Section 2: ROUTING & OPERATING MODE
        tb_printf(2, 9, TB_WHITE | TB_BOLD, 0, "ROUTING & OPERATING MODE");

        tb_printf(2, 10, (cur == 3) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 3) ? "▶" : " ");
        tb_printf(4, 10, TB_GREEN | TB_BOLD, 0, "(4)");
        tb_printf(7, 10, TB_WHITE, 0, " Operating Mode : ");
        if (strcasecmp(cfg->opmode, "Custom") == 0) {
            if (strlen(cfg->customparams) > 0) {
                tb_printf(left_vcol, 10, TB_MAGENTA | TB_BOLD, 0, "Custom (%.*s)", max_left_vlen - 9 > 4 ? max_left_vlen - 9 : 4, cfg->customparams);
            } else {
                tb_printf(left_vcol, 10, TB_MAGENTA | TB_BOLD, 0, "Custom");
            }
        } else if (strcasecmp(cfg->opmode, "Kernel") == 0) {
            tb_printf(left_vcol, 10, TB_CYAN | TB_BOLD, 0, "Kernel (TUN)");
        } else {
            tb_printf(left_vcol, 10, TB_CYAN | TB_BOLD, 0, "Userspace");
        }

        tb_printf(2, 11, (cur == 4) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 4) ? "▶" : " ");
        tb_printf(4, 11, TB_GREEN | TB_BOLD, 0, "(5)");
        tb_printf(7, 11, TB_WHITE, 0, " Exit Node Mode : ");
        if (cfg->exitnode) tb_printf(left_vcol, 11, TB_GREEN | TB_BOLD, 0, "Enabled");
        else tb_printf(left_vcol, 11, TB_HI_BLACK, 0, "Disabled");

        tb_printf(2, 12, (cur == 5) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 5) ? "▶" : " ");
        tb_printf(4, 12, TB_GREEN | TB_BOLD, 0, "(6)");
        tb_printf(7, 12, TB_WHITE, 0, " Subnet Routing : ");
        if (cfg->advroutes) tb_printf(left_vcol, 12, TB_GREEN | TB_BOLD, 0, "Enabled");
        else tb_printf(left_vcol, 12, TB_HI_BLACK, 0, "Disabled");

        tb_printf(2, 13, (cur == 6) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 6) ? "▶" : " ");
        tb_printf(4, 13, TB_GREEN | TB_BOLD, 0, "(7)");
        tb_printf(7, 13, TB_WHITE, 0, " Subnet CIDR    : ");
        if (strlen(cfg->routes) > 0) tb_printf(left_vcol, 13, TB_YELLOW | TB_BOLD, 0, "%.*s", max_left_vlen, cfg->routes);
        else tb_printf(left_vcol, 13, TB_HI_BLACK, 0, "None");

        // Section 3: INTERFACE & LOGGING
        tb_printf(2, 15, TB_WHITE | TB_BOLD, 0, "INTERFACE & LOGGING");

        tb_printf(2, 16, (cur == 7) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 7) ? "▶" : " ");
        tb_printf(4, 16, TB_GREEN | TB_BOLD, 0, "(8)");
        tb_printf(7, 16, TB_WHITE, 0, " Status Interval: ");
        tb_printf(left_vcol, 16, TB_CYAN | TB_BOLD, 0, "%ds", cfg->timerloop);

        tb_printf(2, 17, (cur == 8) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 8) ? "▶" : " ");
        tb_printf(4, 17, TB_GREEN | TB_BOLD, 0, "(9)");
        tb_printf(7, 17, TB_WHITE, 0, " Log Retention  : ");
        if (cfg->logsize == 0) tb_printf(left_vcol, 17, TB_HI_BLACK, 0, "Disabled");
        else tb_printf(left_vcol, 17, TB_CYAN | TB_BOLD, 0, "%d rows", cfg->logsize);

        // --- RIGHT COLUMN ---
        // Section 4: AUTOMATION & ALERTS
        tb_printf(right_x, 4, TB_WHITE | TB_BOLD, 0, "AUTOMATION & ALERTS");

        tb_printf(right_x, 5, (cur == 9) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 9) ? "▶" : " ");
        tb_printf(right_x + 2, 5, TB_GREEN | TB_BOLD, 0, "(10)");
        tb_printf(right_x + 6, 5, TB_WHITE, 0, " AMTM Alerts    : ");
        if (!cfg->amtmemailsuccess && !cfg->amtmemailfailure) tb_printf(right_vcol, 5, TB_HI_BLACK, 0, "Disabled");
        else if (!cfg->amtmemailsuccess && cfg->amtmemailfailure) tb_printf(right_vcol, 5, TB_GREEN | TB_BOLD, 0, "Failures (%d/h)", cfg->ratelimit);
        else if (cfg->amtmemailsuccess && !cfg->amtmemailfailure) tb_printf(right_vcol, 5, TB_GREEN | TB_BOLD, 0, "Success (%d/h)", cfg->ratelimit);
        else tb_printf(right_vcol, 5, TB_GREEN | TB_BOLD, 0, "All (%d/h)", cfg->ratelimit);

        tb_printf(right_x, 6, (cur == 10) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 10) ? "▶" : " ");
        tb_printf(right_x + 2, 6, TB_GREEN | TB_BOLD, 0, "(11)");
        tb_printf(right_x + 6, 6, TB_WHITE, 0, " Auto-Update    : ");
        if (cfg->schedule) tb_printf(right_vcol, 6, TB_GREEN | TB_BOLD, 0, "@ %02d:%02d", cfg->schedulehrs, cfg->schedulemin);
        else tb_printf(right_vcol, 6, TB_HI_BLACK, 0, "Disabled");

        tb_printf(right_x, 7, (cur == 11) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 11) ? "▶" : " ");
        tb_printf(right_x + 2, 7, TB_GREEN | TB_BOLD, 0, "(12)");
        tb_printf(right_x + 6, 7, TB_WHITE, 0, " Release Track  : ");
        if (cfg->track) tb_printf(right_vcol, 7, TB_YELLOW | TB_BOLD, 0, "Beta (Development)");
        else tb_printf(right_vcol, 7, TB_CYAN | TB_BOLD, 0, "Stable (Official)");

        // Section 5: BINARY & MAINTENANCE
        tb_printf(right_x, 9, TB_WHITE | TB_BOLD, 0, "BINARY & MAINTENANCE");

        tb_printf(right_x, 10, (cur == 12) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 12) ? "▶" : " ");
        tb_printf(right_x + 2, 10, TB_GREEN | TB_BOLD, 0, "(13)");
        tb_printf(right_x + 6, 10, TB_WHITE, 0, " Update TS      : ");
        tb_printf(right_vcol, 10, TB_WHITE, 0, "Latest (v%s)", cfg->tsver);

        tb_printf(right_x, 11, (cur == 13) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 13) ? "▶" : " ");
        tb_printf(right_x + 2, 11, TB_GREEN | TB_BOLD, 0, "(14)");
        tb_printf(right_x + 6, 11, TB_WHITE, 0, " Reset State    : ");
        tb_printf(right_vcol, 11, TB_YELLOW | TB_BOLD, 0, "Clear State & Login");

        tb_printf(right_x, 12, (cur == 14) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 14) ? "▶" : " ");
        tb_printf(right_x + 2, 12, TB_GREEN | TB_BOLD, 0, "(15)");
        tb_printf(right_x + 6, 12, TB_WHITE, 0, " Reinstall TS   : ");
        tb_printf(right_vcol, 12, TB_CYAN | TB_BOLD, 0, "Run Installer");

        tb_printf(right_x, 13, (cur == 15) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == 15) ? "▶" : " ");
        tb_printf(right_x + 2, 13, TB_GREEN | TB_BOLD, 0, "(16)");
        tb_printf(right_x + 6, 13, TB_WHITE, 0, " Uninstall ZS   : ");
        tb_printf(right_vcol, 13, TB_RED | TB_BOLD, 0, "Remove All");

        // Section 6: QUICK TIPS & HELP
        tb_printf(right_x, 15, TB_WHITE | TB_BOLD, 0, "TIPS & SHORTCUTS");
        tb_printf(right_x + 2, 16, TB_HI_BLACK, 0, "• Apply Routes : Press 'u' on Monitor to push live");
        tb_printf(right_x + 2, 17, TB_HI_BLACK, 0, "• Quick Toggle : Click any row or press number");
    } else {
        // =========================================================================
        // SINGLE-COLUMN SCROLLING VIEWPORT (For narrow terminals < 86 columns)
        // =========================================================================
        int vcol = is_compact ? 27 : 34;
        int max_vlen = width - vcol - 2;
        if (max_vlen < 6) max_vlen = 6;

        int max_visible = height - 9;
        if (max_visible < 5) max_visible = 5;
        if (max_visible > 16) max_visible = 16;

        if (cur < g_app.config_scroll) g_app.config_scroll = cur;
        if (cur >= g_app.config_scroll + max_visible) g_app.config_scroll = cur - max_visible + 1;
        if (g_app.config_scroll > 16 - max_visible) g_app.config_scroll = 16 - max_visible;
        if (g_app.config_scroll < 0) g_app.config_scroll = 0;

        if (g_app.config_scroll > 0) {
            tb_printf(2, 4, TB_YELLOW, 0, "[▲ %d items above]", g_app.config_scroll);
        } else {
            tb_printf(2, 4, TB_HI_BLACK, 0, "CONFIGURATION ITEMS (1-16):");
        }

        for (int i = 0; i < max_visible; i++) {
            int idx = g_app.config_scroll + i;
            if (idx >= 16) break;
            int y = 5 + i;

            tb_printf(2, y, (cur == idx) ? (TB_CYAN | TB_BOLD) : TB_WHITE, 0, (cur == idx) ? "▶" : " ");
            tb_printf(4, y, TB_GREEN | TB_BOLD, 0, "(%d)", idx + 1);

            int label_x = (idx + 1 >= 10) ? 9 : 8;

            switch (idx) {
                case 0:
                    tb_printf(label_x, y, TB_WHITE, 0, "Watchdog Check   : ");
                    if (cfg->keepalive) tb_printf(vcol, y, TB_GREEN | TB_BOLD, 0, "Enabled");
                    else tb_printf(vcol, y, TB_HI_BLACK, 0, "Disabled");
                    break;
                case 1:
                    tb_printf(label_x, y, TB_WHITE, 0, "Keep Settings    : ");
                    if (cfg->persistentsettings) tb_printf(vcol, y, TB_GREEN | TB_BOLD, 0, "Enabled");
                    else tb_printf(vcol, y, TB_HI_BLACK, 0, "Disabled");
                    break;
                case 2:
                    tb_printf(label_x, y, TB_WHITE, 0, "Autostart Boot   : ");
                    if (cfg->autostart) tb_printf(vcol, y, TB_GREEN | TB_BOLD, 0, "Enabled");
                    else tb_printf(vcol, y, TB_HI_BLACK, 0, "Disabled");
                    break;
                case 3:
                    tb_printf(label_x, y, TB_WHITE, 0, "Operating Mode   : ");
                    if (strcasecmp(cfg->opmode, "Custom") == 0) {
                        tb_printf(vcol, y, TB_MAGENTA | TB_BOLD, 0, "Custom");
                    } else if (strcasecmp(cfg->opmode, "Kernel") == 0) {
                        tb_printf(vcol, y, TB_CYAN | TB_BOLD, 0, "Kernel");
                    } else {
                        tb_printf(vcol, y, TB_CYAN | TB_BOLD, 0, "Userspace");
                    }
                    break;
                case 4:
                    tb_printf(label_x, y, TB_WHITE, 0, "Exit Node Mode   : ");
                    if (cfg->exitnode) tb_printf(vcol, y, TB_GREEN | TB_BOLD, 0, "Enabled");
                    else tb_printf(vcol, y, TB_HI_BLACK, 0, "Disabled");
                    break;
                case 5:
                    tb_printf(label_x, y, TB_WHITE, 0, "Subnet Routing   : ");
                    if (cfg->advroutes) tb_printf(vcol, y, TB_GREEN | TB_BOLD, 0, "Enabled");
                    else tb_printf(vcol, y, TB_HI_BLACK, 0, "Disabled");
                    break;
                case 6:
                    tb_printf(label_x, y, TB_WHITE, 0, "Edit Subnet CIDR : ");
                    if (strlen(cfg->routes) > 0) tb_printf(vcol, y, TB_YELLOW | TB_BOLD, 0, "%.*s", max_vlen, cfg->routes);
                    else tb_printf(vcol, y, TB_HI_BLACK, 0, "None");
                    break;
                case 7:
                    tb_printf(label_x, y, TB_WHITE, 0, "Status Interval  : ");
                    tb_printf(vcol, y, TB_CYAN | TB_BOLD, 0, "%ds", cfg->timerloop);
                    break;
                case 8:
                    tb_printf(label_x, y, TB_WHITE, 0, "Log Retention    : ");
                    if (cfg->logsize == 0) tb_printf(vcol, y, TB_HI_BLACK, 0, "Disabled");
                    else tb_printf(vcol, y, TB_CYAN | TB_BOLD, 0, "%d rows", cfg->logsize);
                    break;
                case 9:
                    tb_printf(label_x, y, TB_WHITE, 0, "AMTM Alerts      : ");
                    if (!cfg->amtmemailsuccess && !cfg->amtmemailfailure) tb_printf(vcol, y, TB_HI_BLACK, 0, "Disabled");
                    else if (!cfg->amtmemailsuccess && cfg->amtmemailfailure) tb_printf(vcol, y, TB_GREEN | TB_BOLD, 0, "Failures");
                    else if (cfg->amtmemailsuccess && !cfg->amtmemailfailure) tb_printf(vcol, y, TB_GREEN | TB_BOLD, 0, "Success");
                    else tb_printf(vcol, y, TB_GREEN | TB_BOLD, 0, "All Alerts");
                    break;
                case 10:
                    tb_printf(label_x, y, TB_WHITE, 0, "Auto-Update      : ");
                    if (cfg->schedule) tb_printf(vcol, y, TB_GREEN | TB_BOLD, 0, "@ %02d:%02d", cfg->schedulehrs, cfg->schedulemin);
                    else tb_printf(vcol, y, TB_HI_BLACK, 0, "Disabled");
                    break;
                case 11:
                    tb_printf(label_x, y, TB_WHITE, 0, "Release Track    : ");
                    if (cfg->track) tb_printf(vcol, y, TB_YELLOW | TB_BOLD, 0, "Beta");
                    else tb_printf(vcol, y, TB_CYAN | TB_BOLD, 0, "Stable");
                    break;
                case 12:
                    tb_printf(label_x, y, TB_WHITE, 0, "Update TS Binary : ");
                    tb_printf(vcol, y, TB_WHITE, 0, "Latest");
                    break;
                case 13:
                    tb_printf(label_x, y, TB_WHITE, 0, "Reset State/Auth : ");
                    tb_printf(vcol, y, TB_YELLOW | TB_BOLD, 0, "Clear & Login");
                    break;
                case 14:
                    tb_printf(label_x, y, TB_WHITE, 0, "Reinstall TS     : ");
                    tb_printf(vcol, y, TB_CYAN | TB_BOLD, 0, "Reinstall");
                    break;
                case 15:
                    tb_printf(label_x, y, TB_WHITE, 0, "Uninstall ZScale : ");
                    tb_printf(vcol, y, TB_RED | TB_BOLD, 0, "Remove All");
                    break;
            }
        }

        if (g_app.config_scroll + max_visible < 16) {
            tb_printf(2, 5 + max_visible, TB_YELLOW, 0, "[▼ %d items below]", 16 - (g_app.config_scroll + max_visible));
        }
    }

    // Bottom Toast & Divider
    for (int x = 1; x < width - 1; x++) {
        tb_printf(x, height - 3, TB_HI_BLACK, 0, "─");
    }

    if (g_app.toast_expiry > time(NULL) && strlen(g_app.toast_msg) > 0) {
        tb_printf(2, height - 4, TB_YELLOW | TB_BOLD, 0, "⚡ %s", g_app.toast_msg);
    }

    if (is_compact) {
        tb_printf(2, height - 2, TB_WHITE, 0, "[ ↑/↓: Nav | Enter: Toggle/Edit | Esc: Back ]");
    } else {
        tb_printf(2, height - 2, TB_WHITE, 0, "[ ↑/↓/←/→: Navigate | Enter: Toggle/Edit | 1-16: Jump | Esc/q: Back to Monitor ]");
    }
}

static void draw_peer_detail_modal(void) {
    draw_dashboard();

    int width = tb_width();
    int height = tb_height();

    if (g_app.selected_peer < 0 || g_app.selected_peer >= g_app.filtered_count) {
        g_app.mode = VIEW_DASHBOARD;
        return;
    }

    int peer_idx = g_app.filtered_indices[g_app.selected_peer];
    PeerInfo *p = &g_app.peers[peer_idx];

    int box_w = (width >= 86) ? 76 : (width - 4);
    if (box_w < 38) box_w = width - 2;
    int box_h = 15;
    int start_x = (width - box_w) / 2;
    int start_y = (height - box_h) / 2;

    draw_modal_box(start_x, start_y, box_w, box_h, "PEER NODE INSPECTOR", TB_CYAN | TB_BOLD, TB_HI_BLACK);

    draw_modal_row(start_x, start_y + 2, box_w, "Device Name", p->name, TB_GREEN | TB_BOLD, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 3, box_w, "Tailscale IP", p->ip, TB_CYAN | TB_BOLD, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 4, box_w, "OS / Platform", p->os, TB_WHITE | TB_BOLD, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 5, box_w, "Owner / User", p->user, TB_WHITE, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 6, box_w, "Connection", p->relay_info, p->is_direct ? (TB_GREEN | TB_BOLD) : TB_YELLOW, TB_HI_BLACK);

    char traffic_buf[64];
    snprintf(traffic_buf, sizeof(traffic_buf), "▲ %s  │  ▼ %s", p->tx_str, p->rx_str);
    draw_modal_row(start_x, start_y + 7, box_w, "Traffic (Tx/Rx)", traffic_buf, TB_CYAN | TB_BOLD, TB_HI_BLACK);

    draw_modal_row(start_x, start_y + 8, box_w, "Last Activity", p->last_seen, p->is_online ? (TB_GREEN | TB_BOLD) : TB_HI_BLACK, TB_HI_BLACK);
    draw_modal_row(start_x, start_y + 9, box_w, "Tailnet Status", p->status, p->is_online ? TB_WHITE : TB_HI_BLACK, TB_HI_BLACK);

    if (box_w < 50) {
        tb_printf(start_x + 3, start_y + 11, TB_WHITE | TB_DIM, TB_HI_BLACK, "Tab: Select | Enter: Run");
    } else {
        tb_printf(start_x + 3, start_y + 11, TB_WHITE | TB_DIM, TB_HI_BLACK, "Use ←/→ or Tab to select, Enter to execute.");
    }

    for (int x = start_x + 1; x < start_x + box_w - 1; x++) {
        tb_printf(x, start_y + 12, TB_WHITE, TB_HI_BLACK, "─");
    }

    if (box_w >= 54) {
        draw_modal_btn(start_x + 3, start_y + 13, "Ping", (g_app.peer_detail_selected_btn == 0), 1, 0);
        draw_modal_btn(start_x + 13, start_y + 13, "Tailscale Ping", (g_app.peer_detail_selected_btn == 1), 1, 0);
        draw_modal_btn(start_x + 34, start_y + 13, "Close", (g_app.peer_detail_selected_btn == 2), 0, 0);
    } else {
        draw_modal_btn(start_x + 2, start_y + 13, "Ping", (g_app.peer_detail_selected_btn == 0), 1, 0);
        draw_modal_btn(start_x + 11, start_y + 13, "TS Ping", (g_app.peer_detail_selected_btn == 1), 1, 0);
        draw_modal_btn(start_x + 23, start_y + 13, "Close", (g_app.peer_detail_selected_btn == 2), 0, 0);
    }
    tb_set_cell(start_x + box_w - 1, start_y + 13, 0x2502 /* │ */, TB_CYAN | TB_BOLD, TB_HI_BLACK);
}

static void draw_logs_view(void) {
    int width = tb_width();
    int height = tb_height();
    int is_compact = (width < 80);

    if (is_compact) {
        tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale Logs (%d lines)", g_app.log_count);
    } else {
        tb_printf(2, 1, TB_GREEN | TB_BOLD, 0, "ZeroScale Event Log Viewer (Total: %d lines)", g_app.log_count);
    }

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

    if (is_compact) {
        tb_printf(2, height - 2, TB_WHITE, 0, "L%d-%d/%d [↑/↓: Scroll | r: Refresh | Esc: Back]", 
                  g_app.log_scroll + 1, 
                  (g_app.log_scroll + max_rows > g_app.log_count ? g_app.log_count : g_app.log_scroll + max_rows),
                  g_app.log_count);
    } else {
        tb_printf(2, height - 2, TB_WHITE, 0, "Line %d-%d of %d  [↑/↓: Scroll | g/G: Top/Bottom | r: Refresh | Esc/q: Back]", 
                  g_app.log_scroll + 1, 
                  (g_app.log_scroll + max_rows > g_app.log_count ? g_app.log_count : g_app.log_scroll + max_rows),
                  g_app.log_count);
    }
}

static void draw_input_modal(void) {
    if (g_app.prev_mode == VIEW_CONFIG || g_app.mode == VIEW_INPUT) {
        draw_unified_config_view();
    } else {
        draw_dashboard();
    }

    int width = tb_width();
    int height = tb_height();

    int box_w = (width >= 82) ? 76 : (width - 4);
    if (box_w < 38) box_w = width - 2;
    int box_h = 9;
    int start_x = (width - box_w) / 2;
    int start_y = (height - box_h) / 2;

    draw_modal_box(start_x, start_y, box_w, box_h, g_app.input_title, TB_YELLOW | TB_BOLD, TB_HI_BLACK);

    tb_printf(start_x + 3, start_y + 2, TB_WHITE | TB_BOLD, TB_HI_BLACK, "%.*s", box_w - 6, g_app.input_prompt);
    if (box_w < 50) {
        tb_printf(start_x + 3, start_y + 3, TB_WHITE | TB_DIM, TB_HI_BLACK, "Enter: Save | Esc: Cancel");
    } else {
        tb_printf(start_x + 3, start_y + 3, TB_WHITE | TB_DIM, TB_HI_BLACK, "Use ←/→ to move cursor, Enter to save, Tab for buttons.");
    }

    int text_focused = (g_app.input_selected_btn == 0);
    int input_field_y = start_y + 5;
    int max_input_display = box_w - 6;
    int buf_len = (int)strlen(g_app.input_buf);

    for (int x = 0; x < max_input_display; x++) {
        tb_set_cell(start_x + 3 + x, input_field_y, ' ', TB_WHITE, TB_HI_BLACK);
    }

    for (int i = 0; i < buf_len && i < max_input_display; i++) {
        char ch = g_app.input_buf[i];
        if (text_focused && i == g_app.input_cursor) {
            tb_set_cell(start_x + 3 + i, input_field_y, ch, TB_BLACK, TB_CYAN | TB_BOLD);
        } else {
            tb_set_cell(start_x + 3 + i, input_field_y, ch, TB_CYAN | TB_BOLD, TB_HI_BLACK);
        }
    }
    if (text_focused && g_app.input_cursor == buf_len && buf_len < max_input_display) {
        tb_set_cell(start_x + 3 + buf_len, input_field_y, ' ', TB_BLACK, TB_CYAN | TB_BOLD);
    }

    draw_modal_btn(start_x + 3, start_y + 7, "Save", (g_app.input_selected_btn == 1), 1, 0);
    draw_modal_btn(start_x + 13, start_y + 7, "Cancel", (g_app.input_selected_btn == 2), 0, 0);
    tb_set_cell(start_x + box_w - 1, start_y + 7, 0x2502 /* │ */, TB_YELLOW | TB_BOLD, TB_HI_BLACK);
}

static void draw_confirm_dialog(void) {
    if (g_app.prev_mode == VIEW_CONFIG) {
        draw_unified_config_view();
    } else {
        draw_dashboard();
    }

    int width = tb_width();
    int height = tb_height();

    int box_w = (width >= 70) ? 62 : (width - 4);
    if (box_w < 38) box_w = width - 2;
    int box_h = 8;
    int start_x = (width - box_w) / 2;
    int start_y = (height - box_h) / 2;

    const char *action_lbl = (strlen(g_app.confirm_action_label) > 0) ? g_app.confirm_action_label : "Confirm";
    int is_destructive = (strstr(action_lbl, "Uninstall") || strstr(action_lbl, "Stop") || strstr(action_lbl, "Reset") || strstr(action_lbl, "Disconnect")) ? 1 : 0;

    draw_modal_box(start_x, start_y, box_w, box_h, "CONFIRMATION REQUIRED", is_destructive ? (TB_RED | TB_BOLD) : (TB_YELLOW | TB_BOLD), TB_HI_BLACK);

    tb_printf(start_x + 3, start_y + 2, TB_WHITE | TB_BOLD, TB_HI_BLACK, "%.*s", box_w - 6, g_app.confirm_prompt);
    if (box_w < 50) {
        tb_printf(start_x + 3, start_y + 3, TB_WHITE | TB_DIM, TB_HI_BLACK, "Tab: Select | Enter: Run");
    } else {
        tb_printf(start_x + 3, start_y + 3, TB_WHITE | TB_DIM, TB_HI_BLACK, "Use ←/→ or Tab to select, Enter to execute.");
    }

    int act_len = (int)strlen(action_lbl) + 2;
    int btn0_x = start_x + 3;
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
