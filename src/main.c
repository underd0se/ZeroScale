#define TB_IMPL
#include "app.h"

AppState g_app;

static void handle_sigint(int sig) {
    (void)sig;
    g_app.running = 0;
}

void detect_terminal(void) {
    // Default fallback
    snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Shift+Drag");
    snprintf(g_app.term_name, sizeof(g_app.term_name), "Terminal");

    const char *tp = getenv("TERM_PROGRAM");
    const char *term = getenv("TERM");

    if (tp) {
        if (strcasestr(tp, "ghostty")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Shift+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "Ghostty");
            return;
        } else if (strcasestr(tp, "kitty")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Shift+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "Kitty");
            return;
        } else if (strcasestr(tp, "wezterm")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Shift+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "WezTerm");
            return;
        } else if (strcasestr(tp, "alacritty")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Shift+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "Alacritty");
            return;
        } else if (strcasestr(tp, "Apple_Terminal")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Fn+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "Terminal.app");
            return;
        } else if (strcasestr(tp, "iTerm")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Fn+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "iTerm2");
            return;
        }
    }

    if (term) {
        if (strcasestr(term, "ghostty")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Shift+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "Ghostty");
            return;
        } else if (strcasestr(term, "kitty")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Shift+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "Kitty");
            return;
        } else if (strcasestr(term, "alacritty")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Shift+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "Alacritty");
            return;
        } else if (strcasestr(term, "foot")) {
            snprintf(g_app.copy_hint, sizeof(g_app.copy_hint), "Shift+Drag");
            snprintf(g_app.term_name, sizeof(g_app.term_name), "Foot");
            return;
        }
    }
}

static void sanitize_legacy_symlinks(void) {
    char target[256];
    ssize_t len = readlink("/opt/bin/tailmon", target, sizeof(target) - 1);
    if (len > 0) {
        target[len] = '\0';
        if (strstr(target, "zeroscale") || strstr(target, "tailmon-zero")) {
            unlink("/opt/bin/tailmon");
            if (access("/jffs/scripts/tailmon.sh", F_OK) == 0) {
                symlink("/jffs/scripts/tailmon.sh", "/opt/bin/tailmon");
            } else if (access("/jffs/scripts/tailmon", F_OK) == 0) {
                symlink("/jffs/scripts/tailmon", "/opt/bin/tailmon");
            }
        }
    }
    unlink("/opt/bin/tailmon-zero");
    unlink("/jffs/scripts/tailmon-zero");
}

void app_init(void) {
    memset(&g_app, 0, sizeof(g_app));
    g_app.running = 1;
    g_app.mode = VIEW_DASHBOARD;
    g_app.selected_peer = -1;

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    detect_terminal();
    sanitize_legacy_symlinks();

    tb_init();
    tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);

    load_config();

    // Centered startup splash sequence
    show_splash("INITIALIZING...", 350, TB_HI_BLACK);
    show_splash("INITIALIZING ... DONE", 300, TB_YELLOW | TB_BOLD);
    show_splash("STARTING ZEROSCALE", 300, TB_GREEN | TB_BOLD);

    refresh_tailscale_status();
    g_app.countdown = g_app.config.timerloop > 0 ? g_app.config.timerloop : 60;
    g_app.last_tick = time(NULL);
    g_app.last_status_refresh = time(NULL);

    log_event("INFO", "ZeroScale v%s session started.", g_app.config.version);
}

void app_cleanup(void) {
    // Centered exit splash sequence
    show_splash("SHUTTING DOWN...", 350, TB_YELLOW | TB_BOLD);
    show_splash("GOODBYE...", 500, TB_HI_BLACK);
    log_event("INFO", "ZeroScale session ended.");
    tb_shutdown();
}

void request_confirm(const char *prompt, const char *action_label, const char *cmd) {
    snprintf(g_app.confirm_prompt, sizeof(g_app.confirm_prompt), "%s", prompt);
    snprintf(g_app.confirm_action_label, sizeof(g_app.confirm_action_label), "%s", action_label);
    snprintf(g_app.confirm_cmd, sizeof(g_app.confirm_cmd), "%s", cmd);
    g_app.confirm_selected_btn = 0;
    g_app.prev_mode = g_app.mode;
    g_app.mode = VIEW_CONFIRM;
}

void request_input(InputTarget target, const char *title, const char *prompt, const char *initial) {
    g_app.input_target = target;
    snprintf(g_app.input_title, sizeof(g_app.input_title), "%s", title);
    snprintf(g_app.input_prompt, sizeof(g_app.input_prompt), "%s", prompt);
    snprintf(g_app.input_buf, sizeof(g_app.input_buf), "%s", initial ? initial : "");
    g_app.input_cursor = (int)strlen(g_app.input_buf);
    g_app.input_selected_btn = 0;
    g_app.prev_mode = g_app.mode;
    g_app.mode = VIEW_INPUT;
}

void execute_action(const char *action, const char *cmd) {
    show_toast("%s", action);
    log_event("INFO", "%s", action);
    char buf[512];
    snprintf(buf, sizeof(buf), "( %s ) >/dev/null 2>&1 &", cmd);
    system(buf);
    tb_invalidate();
    refresh_tailscale_status();
}

void install_zeroscale(void) {
    printf("\n=== Installing / Reinstalling Entware Tailscale ===\n\n");
    system("/opt/etc/init.d/S06tailscaled stop 2>/dev/null || true");
    system("opkg update && opkg install --force-reinstall tailscale");
    system("/opt/etc/init.d/S06tailscaled start");
    printf("\n[+] Entware Tailscale Installation Complete.\n\n");
}

void uninstall_zeroscale(void) {
    show_splash("UNINSTALLING ZEROSCALE...", 600, TB_RED | TB_BOLD);
    system("/opt/etc/init.d/S06tailscaled stop 2>/dev/null; "
           "sed -i -e '/zeroscale/d' /jffs/scripts/post-mount 2>/dev/null; "
           "cru d zeroscale_autoupdate 2>/dev/null; "
           "rm -rf /jffs/addons/zeroscale.d /jffs/scripts/zeroscale /opt/bin/zeroscale");
    show_splash("UNINSTALL COMPLETE", 800, TB_HI_BLACK);
    tb_shutdown();
    printf("\n[ZeroScale Successfully Uninstalled from Router]\n");
    if (access("/jffs/scripts/tailmon.sh", F_OK) == 0 || access("/jffs/scripts/tailmon", F_OK) == 0) {
        printf("[*] TAILMON was detected on your system. To re-activate TAILMON, simply run: tailmon\n");
    }
    printf("\n");
    exit(0);
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Dashboard Handlers

static void trigger_header_action(int idx) {
    g_app.dash_focus = FOCUS_HEADER_MENU;
    g_app.header_selected_idx = idx;
    switch (idx) {
        case 0: // Up
            execute_action("Connecting Tailscale (tailscale up)...", "tailscale up");
            break;
        case 1: // Down
            request_confirm("Disconnect from Tailnet?", "Disconnect", "tailscale down");
            break;
        case 2: // Restart
            request_confirm("Restart Tailscale Daemon?", "Restart", "/opt/etc/init.d/S06tailscaled restart");
            break;
        case 3: // Start
            execute_action("Starting Tailscale Daemon...", "/opt/etc/init.d/S06tailscaled start");
            break;
        case 4: // Stop
            request_confirm("Stop Tailscale Daemon?", "Stop", "/opt/etc/init.d/S06tailscaled stop");
            break;
        case 5: // Logs
            load_logs();
            g_app.mode = VIEW_LOGS;
            break;
        case 6: // Configuration
            g_app.mode = VIEW_CONFIG;
            break;
        case 7: // Quit
            g_app.running = 0;
            break;
        default: break;
    }
}

static void handle_dashboard_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC || ev->ch == 'q' || ev->ch == 'Q') {
        g_app.running = 0;
    } else if (ev->ch == 'u' || ev->ch == 'U') {
        trigger_header_action(0);
    } else if (ev->ch == 'd' || ev->ch == 'D') {
        trigger_header_action(1);
    } else if (ev->ch == 'r' || ev->ch == 'R') {
        trigger_header_action(2);
    } else if (ev->ch == 's' || ev->ch == 'S') {
        trigger_header_action(3);
    } else if (ev->ch == 't' || ev->ch == 'T') {
        trigger_header_action(4);
    } else if (ev->ch == 'l' || ev->ch == 'L') {
        trigger_header_action(5);
    } else if (ev->ch == 'c' || ev->ch == 'C') {
        trigger_header_action(6);
    } else if (ev->key == TB_KEY_ARROW_RIGHT) {
        if (g_app.dash_focus == FOCUS_NONE) {
            g_app.dash_focus = FOCUS_HEADER_MENU;
            g_app.header_selected_idx = 0;
        } else if (g_app.dash_focus == FOCUS_HEADER_MENU) {
            if (g_app.header_selected_idx < 7) g_app.header_selected_idx++;
        }
    } else if (ev->key == TB_KEY_ARROW_LEFT) {
        if (g_app.dash_focus == FOCUS_HEADER_MENU) {
            if (g_app.header_selected_idx > 0) g_app.header_selected_idx--;
        }
    } else if (ev->key == TB_KEY_ARROW_DOWN) {
        if (g_app.dash_focus == FOCUS_NONE) {
            g_app.dash_focus = FOCUS_HEADER_MENU;
            g_app.header_selected_idx = 0;
        } else if (g_app.dash_focus == FOCUS_HEADER_MENU) {
            g_app.dash_focus = FOCUS_PEERS;
            if (g_app.selected_peer < 0) g_app.selected_peer = 0;
        } else if (g_app.dash_focus == FOCUS_PEERS) {
            if (g_app.selected_peer < g_app.peer_count - 1) g_app.selected_peer++;
            int max_rows = tb_height() - 13;
            if (max_rows < 1) max_rows = 1;
            if (g_app.selected_peer >= g_app.peer_scroll + max_rows) g_app.peer_scroll++;
        }
    } else if (ev->key == TB_KEY_ARROW_UP) {
        if (g_app.dash_focus == FOCUS_NONE) {
            g_app.dash_focus = FOCUS_HEADER_MENU;
            g_app.header_selected_idx = 0;
        } else if (g_app.dash_focus == FOCUS_PEERS) {
            if (g_app.selected_peer > 0) {
                g_app.selected_peer--;
                if (g_app.selected_peer < g_app.peer_scroll) g_app.peer_scroll = g_app.selected_peer;
            } else {
                g_app.dash_focus = FOCUS_HEADER_MENU;
            }
        }
    } else if (ev->key == TB_KEY_ENTER) {
        if (g_app.dash_focus == FOCUS_HEADER_MENU) {
            trigger_header_action(g_app.header_selected_idx);
        } else if (g_app.dash_focus == FOCUS_PEERS || g_app.selected_peer >= 0) {
            if (g_app.selected_peer >= 0 && g_app.selected_peer < g_app.peer_count) {
                g_app.peer_detail_selected_btn = 0;
                g_app.mode = VIEW_PEER_DETAIL;
            }
        }
    }
}

static void handle_dashboard_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_LEFT) {
        int width = tb_width();
        int is_wide = (width >= 96);
        // Row 5 Action Bar Click
        if (ev->y == 5) {
            int x0 = 1;
            int b0 = x0 + 9;
            int b1 = b0 + 5;
            int div1 = b1 + 7;
            int ts_lbl = div1 + 2 + (is_wide ? 11 : 4);
            int b2 = ts_lbl;
            int b3 = b2 + 10;
            int b4 = b3 + 8;
            int div2 = b4 + 7;
            int b5 = div2 + 2;
            int div3 = b5 + 7;
            int b6 = div3 + 2;
            int div4 = b6 + (is_wide ? 16 : 9);
            int b7 = div4 + 2;

            if (ev->x >= b0 && ev->x < b0 + 4) {
                trigger_header_action(0);
            } else if (ev->x >= b1 && ev->x < b1 + 6) {
                trigger_header_action(1);
            } else if (ev->x >= b2 && ev->x < b2 + 9) {
                trigger_header_action(2);
            } else if (ev->x >= b3 && ev->x < b3 + 7) {
                trigger_header_action(3);
            } else if (ev->x >= b4 && ev->x < b4 + 6) {
                trigger_header_action(4);
            } else if (ev->x >= b5 && ev->x < b5 + 6) {
                trigger_header_action(5);
            } else if (ev->x >= b6 && ev->x < b6 + (is_wide ? 15 : 8)) {
                trigger_header_action(6);
            } else if (ev->x >= b7 && ev->x < b7 + 6) {
                trigger_header_action(7);
            }
        } else if (ev->y >= 10 && ev->y < 10 + g_app.peer_count) {
            int clicked_idx = (ev->y - 10) + g_app.peer_scroll;
            if (clicked_idx < g_app.peer_count) {
                g_app.dash_focus = FOCUS_PEERS;
                if (g_app.selected_peer == clicked_idx) {
                    g_app.peer_detail_selected_btn = 0;
                    g_app.mode = VIEW_PEER_DETAIL;
                } else {
                    g_app.selected_peer = clicked_idx;
                    show_toast("Selected: %s (%s) — Press Enter or click again for details", 
                               g_app.peers[clicked_idx].name, g_app.peers[clicked_idx].ip);
                }
            }
        }
    } else if (ev->key == TB_KEY_MOUSE_WHEEL_UP) {
        if (g_app.peer_scroll > 0) g_app.peer_scroll--;
    } else if (ev->key == TB_KEY_MOUSE_WHEEL_DOWN) {
        if (g_app.peer_scroll < g_app.peer_count - 5) g_app.peer_scroll++;
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Unified Config Menu Handlers

static void trigger_config_action(int idx) {
    g_app.config_selected_idx = idx;
    switch (idx) {
        case 0: toggle_keepalive(); break;
        case 1: toggle_persistentsettings(); break;
        case 2: toggle_autostart(); break;
        case 3: cycle_opmode(); break;
        case 4: toggle_exitnode(); break;
        case 5: toggle_advroutes(); break;
        case 6: request_input(INPUT_ROUTES, "Subnet Routes CIDR", "Enter subnet CIDRs, comma-separated (e.g. 192.168.1.0/24,10.0.0.0/24)", g_app.config.routes); break;
        case 7: cycle_timerloop(); break;
        case 8: {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", g_app.config.logsize);
            request_input(INPUT_LOGSIZE, "Event Log Retention", "Enter max log rows (100-9999, 0=Disable)", buf);
            break;
        }
        case 9: cycle_amtm_email(); break;
        case 10: cycle_schedule(); break;
        case 11:
            request_confirm("Update Tailscale binary to latest version?",
                            "Update",
                            "/opt/bin/opkg update && /opt/bin/opkg upgrade tailscale || tailscale update --yes; /opt/etc/init.d/S06tailscaled restart");
            break;
        case 12:
            request_confirm("Reset daemon state and re-authenticate?",
                            "Reset State",
                            "/opt/etc/init.d/S06tailscaled stop; rm -f /opt/var/tailscaled.state; /opt/etc/init.d/S06tailscaled start");
            break;
        case 13:
            request_confirm("Reinstall Entware Tailscale package?",
                            "Reinstall",
                            "/opt/etc/init.d/S06tailscaled stop 2>/dev/null; /opt/bin/opkg update; /opt/bin/opkg install --force-reinstall tailscale; /opt/etc/init.d/S06tailscaled start");
            break;
        case 14:
            request_confirm("Completely uninstall ZeroScale from router?",
                            "Uninstall",
                            "killall -9 zeroscale 2>/dev/null; /opt/etc/init.d/S06tailscaled stop; sed -i -e '/zeroscale/d' /jffs/scripts/post-mount 2>/dev/null; cru d zeroscale_autoupdate 2>/dev/null; rm -rf /jffs/addons/zeroscale.d /jffs/scripts/zeroscale /opt/bin/zeroscale");
            break;
        default: break;
    }
}

static int s_config_pending_digit = 0;
static time_t s_config_digit_time = 0;

static void handle_config_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC || ev->ch == 'q' || ev->ch == 'Q' || ev->ch == 'e' || ev->ch == 'E') {
        s_config_pending_digit = 0;
        g_app.mode = VIEW_DASHBOARD;
        return;
    }

    if (s_config_pending_digit == 1) {
        s_config_pending_digit = 0;
        if (ev->ch >= '0' && ev->ch <= '5') {
            int num = 10 + (ev->ch - '0');
            if (num >= 10 && num <= 15) {
                trigger_config_action(num - 1);
                return;
            }
        } else if (ev->key == TB_KEY_ENTER || ev->ch == ' ') {
            trigger_config_action(0);
            return;
        } else {
            trigger_config_action(0);
        }
    }

    if (ev->key == TB_KEY_ARROW_DOWN) {
        if (g_app.config_selected_idx < 14) g_app.config_selected_idx++;
    } else if (ev->key == TB_KEY_ARROW_UP) {
        if (g_app.config_selected_idx > 0) g_app.config_selected_idx--;
    } else if (ev->key == TB_KEY_ENTER) {
        if (g_app.config_selected_idx == 3 && strcasecmp(g_app.config.opmode, "Custom") == 0) {
            request_input(INPUT_CUSTOMPARAMS, "Custom Tailscale Flags", "Enter custom tailscale up flags (e.g. --accept-routes)", g_app.config.customparams);
        } else {
            trigger_config_action(g_app.config_selected_idx);
        }
    } else if (ev->ch == ' ') {
        trigger_config_action(g_app.config_selected_idx);
    } else if (ev->ch == '1') {
        s_config_pending_digit = 1;
        s_config_digit_time = time(NULL);
        g_app.config_selected_idx = 0;
        show_toast("Option (1)... [Press 0-5 for (10)-(15), or Enter for (1)]");
    } else if (ev->ch == '4') {
        if (strcasecmp(g_app.config.opmode, "Custom") == 0) {
            request_input(INPUT_CUSTOMPARAMS, "Custom Tailscale Flags", "Enter custom tailscale up flags (e.g. --accept-routes)", g_app.config.customparams);
        } else {
            trigger_config_action(3);
        }
    } else if (ev->ch >= '2' && ev->ch <= '9') {
        trigger_config_action(ev->ch - '1');
    } else if (ev->ch == 'u' || ev->ch == 'U') {
        trigger_config_action(11);
    } else if (ev->ch == 'x' || ev->ch == 'X') {
        trigger_config_action(12);
    } else if (ev->ch == 'i' || ev->ch == 'I') {
        trigger_config_action(13);
    }
}

static void handle_config_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_LEFT) {
        switch (ev->y) {
            case 5: trigger_config_action(0); break;
            case 6: trigger_config_action(1); break;
            case 7: trigger_config_action(2); break;
            case 10: trigger_config_action(3); break;
            case 11: trigger_config_action(4); break;
            case 12: trigger_config_action(5); break;
            case 13: trigger_config_action(6); break;
            case 16: trigger_config_action(7); break;
            case 17: trigger_config_action(8); break;
            case 20: trigger_config_action(9); break;
            case 21: trigger_config_action(10); break;
            case 24: trigger_config_action(11); break;
            case 25: trigger_config_action(12); break;
            case 26: trigger_config_action(13); break;
            case 27: trigger_config_action(14); break;
            default: break;
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Peer Detail Modal Handlers

static void do_peer_ping(void) {
    if (g_app.selected_peer >= 0 && g_app.selected_peer < g_app.peer_count) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "ping -c 3 %s >/dev/null 2>&1", g_app.peers[g_app.selected_peer].ip);
        show_toast("Pinging %s (%s)...", g_app.peers[g_app.selected_peer].name, g_app.peers[g_app.selected_peer].ip);
        int res = system(cmd);
        if (res == 0) show_toast("Ping to %s: SUCCESS (Host Online)", g_app.peers[g_app.selected_peer].name);
        else show_toast("Ping to %s: FAILED / TIMEOUT", g_app.peers[g_app.selected_peer].name);
    }
}

static void do_peer_ts_ping(void) {
    if (g_app.selected_peer >= 0 && g_app.selected_peer < g_app.peer_count) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "tailscale ping -c 1 %s >/dev/null 2>&1", g_app.peers[g_app.selected_peer].ip);
        show_toast("Testing Tailscale WireGuard latency to %s...", g_app.peers[g_app.selected_peer].name);
        int res = system(cmd);
        if (res == 0) show_toast("Tailscale Ping to %s: Connected!", g_app.peers[g_app.selected_peer].name);
        else show_toast("Tailscale Ping to %s: Unreachable", g_app.peers[g_app.selected_peer].name);
    }
}

static void handle_peer_detail_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC || ev->ch == 'c' || ev->ch == 'C' || ev->ch == 'q' || ev->ch == 'Q' || ev->ch == 'e' || ev->ch == 'E') {
        g_app.mode = VIEW_DASHBOARD;
    } else if (ev->ch == 'p' || ev->ch == 'P') {
        g_app.peer_detail_selected_btn = 0;
        do_peer_ping();
    } else if (ev->ch == 't' || ev->ch == 'T') {
        g_app.peer_detail_selected_btn = 1;
        do_peer_ts_ping();
    } else if (ev->key == TB_KEY_ARROW_RIGHT || ev->key == TB_KEY_TAB) {
        g_app.peer_detail_selected_btn = (g_app.peer_detail_selected_btn + 1) % 3;
    } else if (ev->key == TB_KEY_ARROW_LEFT) {
        g_app.peer_detail_selected_btn = (g_app.peer_detail_selected_btn + 2) % 3;
    } else if (ev->key == TB_KEY_ENTER || ev->ch == ' ') {
        if (g_app.peer_detail_selected_btn == 0) {
            do_peer_ping();
        } else if (g_app.peer_detail_selected_btn == 1) {
            do_peer_ts_ping();
        } else if (g_app.peer_detail_selected_btn == 2) {
            g_app.mode = VIEW_DASHBOARD;
        }
    }
}

static void handle_peer_detail_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_LEFT) {
        int width = tb_width();
        int height = tb_height();
        int box_w = (width >= 86) ? 80 : (width - 4);
        if (box_w < 60) box_w = 60;
        int box_h = 13;
        int start_x = (width - box_w) / 2;
        int start_y = (height - box_h) / 2;

        if (ev->y == start_y + 11) {
            if (ev->x >= start_x + 3 && ev->x < start_x + 11) {
                g_app.peer_detail_selected_btn = 0;
                do_peer_ping();
            } else if (ev->x >= start_x + 13 && ev->x < start_x + 32) {
                g_app.peer_detail_selected_btn = 1;
                do_peer_ts_ping();
            } else if (ev->x >= start_x + 34 && ev->x < start_x + 43) {
                g_app.peer_detail_selected_btn = 2;
                g_app.mode = VIEW_DASHBOARD;
            }
        } else if (ev->x < start_x || ev->x >= start_x + box_w || ev->y < start_y || ev->y >= start_y + box_h) {
            g_app.mode = VIEW_DASHBOARD;
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Log Viewer Handlers

static void handle_logs_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC || ev->ch == 'q' || ev->ch == 'Q' || ev->ch == 'e' || ev->ch == 'E') {
        g_app.mode = VIEW_DASHBOARD;
    } else if (ev->ch == 'r' || ev->ch == 'R') {
        load_logs();
        show_toast("Logs reloaded (%d lines)", g_app.log_count);
    } else if (ev->key == TB_KEY_ARROW_UP || ev->ch == 'k') {
        if (g_app.log_scroll > 0) g_app.log_scroll--;
    } else if (ev->key == TB_KEY_ARROW_DOWN || ev->ch == 'j') {
        if (g_app.log_scroll < g_app.log_count - 5) g_app.log_scroll++;
    } else if (ev->key == TB_KEY_PGUP) {
        g_app.log_scroll -= 15;
        if (g_app.log_scroll < 0) g_app.log_scroll = 0;
    } else if (ev->key == TB_KEY_PGDN) {
        g_app.log_scroll += 15;
        if (g_app.log_scroll >= g_app.log_count - 5) g_app.log_scroll = g_app.log_count - 5;
        if (g_app.log_scroll < 0) g_app.log_scroll = 0;
    } else if (ev->ch == 'g') {
        g_app.log_scroll = 0;
    } else if (ev->ch == 'G') {
        int max_visible = tb_height() - 6;
        g_app.log_scroll = g_app.log_count - max_visible;
        if (g_app.log_scroll < 0) g_app.log_scroll = 0;
    }
}

static void handle_logs_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_WHEEL_UP) {
        if (g_app.log_scroll > 0) g_app.log_scroll -= 3;
        if (g_app.log_scroll < 0) g_app.log_scroll = 0;
    } else if (ev->key == TB_KEY_MOUSE_WHEEL_DOWN) {
        if (g_app.log_scroll < g_app.log_count - 5) g_app.log_scroll += 3;
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Input Modal Handlers

static void save_input_action(void) {
    if (g_app.input_target == INPUT_ROUTES) {
        snprintf(g_app.config.routes, sizeof(g_app.config.routes), "%s", g_app.input_buf);
        save_config();
        log_event("INFO", "Subnet Routes updated to: %s", g_app.config.routes);
        show_toast("Subnet Routes updated to: %s", g_app.config.routes);
    } else if (g_app.input_target == INPUT_LOGSIZE) {
        int size = atoi(g_app.input_buf);
        g_app.config.logsize = size;
        save_config();
        log_event("INFO", "Log retention updated to: %d rows.", g_app.config.logsize);
        show_toast("Log Retention updated to: %d rows", g_app.config.logsize);
    } else if (g_app.input_target == INPUT_CUSTOMPARAMS) {
        snprintf(g_app.config.customparams, sizeof(g_app.config.customparams), "%s", g_app.input_buf);
        save_config();
        log_event("INFO", "Custom Tailscale flags updated to: %s", g_app.config.customparams);
        show_toast("Custom Flags updated: %s", g_app.config.customparams);
    }
    g_app.mode = (g_app.prev_mode == VIEW_CONFIG) ? VIEW_CONFIG : VIEW_DASHBOARD;
}

static void cancel_input_action(void) {
    show_toast("Edit cancelled.");
    g_app.mode = (g_app.prev_mode == VIEW_CONFIG) ? VIEW_CONFIG : VIEW_DASHBOARD;
}

static void handle_input_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC) {
        cancel_input_action();
        return;
    }

    if (g_app.input_selected_btn == 0) {
        // Focus is on text input box
        int len = (int)strlen(g_app.input_buf);

        if (ev->key == TB_KEY_ENTER) {
            save_input_action();
        } else if (ev->key == TB_KEY_TAB || ev->key == TB_KEY_ARROW_DOWN) {
            g_app.input_selected_btn = 1; // Move to Save button
        } else if (ev->key == TB_KEY_ARROW_LEFT) {
            if (g_app.input_cursor > 0) g_app.input_cursor--;
        } else if (ev->key == TB_KEY_ARROW_RIGHT) {
            if (g_app.input_cursor < len) g_app.input_cursor++;
        } else if (ev->key == TB_KEY_HOME || (ev->key == TB_KEY_CTRL_A)) {
            g_app.input_cursor = 0;
        } else if (ev->key == TB_KEY_END || (ev->key == TB_KEY_CTRL_E)) {
            g_app.input_cursor = len;
        } else if (ev->key == TB_KEY_DELETE || (ev->key == TB_KEY_CTRL_D)) {
            if (g_app.input_cursor < len) {
                memmove(&g_app.input_buf[g_app.input_cursor], &g_app.input_buf[g_app.input_cursor + 1], len - g_app.input_cursor);
            }
        } else if (ev->key == TB_KEY_BACKSPACE || ev->key == TB_KEY_BACKSPACE2) {
            if (g_app.input_cursor > 0 && len > 0) {
                memmove(&g_app.input_buf[g_app.input_cursor - 1], &g_app.input_buf[g_app.input_cursor], len - g_app.input_cursor + 1);
                g_app.input_cursor--;
            }
        } else if (ev->key == TB_KEY_CTRL_U || ev->key == TB_KEY_CTRL_K) {
            g_app.input_buf[0] = '\0';
            g_app.input_cursor = 0;
        } else if (ev->ch >= 32 && ev->ch <= 126) {
            if (len < (int)sizeof(g_app.input_buf) - 2) {
                memmove(&g_app.input_buf[g_app.input_cursor + 1], &g_app.input_buf[g_app.input_cursor], len - g_app.input_cursor + 1);
                g_app.input_buf[g_app.input_cursor++] = (char)ev->ch;
            }
        }
    } else if (g_app.input_selected_btn == 1) {
        // Focus is on Save button
        if (ev->key == TB_KEY_ENTER || ev->ch == ' ' || ev->ch == 's' || ev->ch == 'S' || ev->ch == 'y' || ev->ch == 'Y') {
            save_input_action();
        } else if (ev->key == TB_KEY_ARROW_RIGHT || ev->key == TB_KEY_TAB) {
            g_app.input_selected_btn = 2; // Move to Cancel button
        } else if (ev->key == TB_KEY_ARROW_UP || ev->key == TB_KEY_ARROW_LEFT) {
            g_app.input_selected_btn = 0; // Return to text field
        } else if (ev->ch == 'c' || ev->ch == 'C' || ev->ch == 'q' || ev->ch == 'Q' || ev->ch == 'e' || ev->ch == 'E') {
            cancel_input_action();
        }
    } else if (g_app.input_selected_btn == 2) {
        // Focus is on Cancel button
        if (ev->key == TB_KEY_ENTER || ev->ch == ' ' || ev->ch == 'c' || ev->ch == 'C' || ev->ch == 'q' || ev->ch == 'Q' || ev->ch == 'e' || ev->ch == 'E') {
            cancel_input_action();
        } else if (ev->key == TB_KEY_ARROW_LEFT) {
            g_app.input_selected_btn = 1; // Move to Save button
        } else if (ev->key == TB_KEY_ARROW_UP) {
            g_app.input_selected_btn = 0; // Return to text field
        } else if (ev->key == TB_KEY_TAB || ev->key == TB_KEY_ARROW_RIGHT) {
            g_app.input_selected_btn = 0; // Wrap back to text field
        }
    }
}

static void handle_input_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_LEFT) {
        int width = tb_width();
        int height = tb_height();
        int box_w = (width >= 82) ? 78 : (width - 4);
        if (box_w < 50) box_w = 50;
        int box_h = 9;
        int start_x = (width - box_w) / 2;
        int start_y = (height - box_h) / 2;

        if (ev->y == start_y + 5) {
            g_app.input_selected_btn = 0; // Click text field
            int clicked_pos = ev->x - (start_x + 3);
            int len = (int)strlen(g_app.input_buf);
            if (clicked_pos < 0) clicked_pos = 0;
            if (clicked_pos > len) clicked_pos = len;
            g_app.input_cursor = clicked_pos;
        } else if (ev->y == start_y + 7) {
            if (ev->x >= start_x + 4 && ev->x < start_x + 12) {
                g_app.input_selected_btn = 1;
                save_input_action();
            } else if (ev->x >= start_x + 14 && ev->x < start_x + 22) {
                g_app.input_selected_btn = 2;
                cancel_input_action();
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Confirm Dialog Handlers

static void execute_confirm_action(void) {
    char buf[512];
    snprintf(buf, sizeof(buf), "( %s ) >/dev/null 2>&1 &", g_app.confirm_cmd);
    system(buf);
    log_event("INFO", "Executed action: %s (%s)", g_app.confirm_action_label, g_app.confirm_cmd);
    show_toast("Action executed.");
    g_app.mode = (g_app.prev_mode == VIEW_CONFIG) ? VIEW_CONFIG : VIEW_DASHBOARD;
    tb_invalidate();
    refresh_tailscale_status();
}

static void cancel_confirm_action(void) {
    show_toast("Action cancelled.");
    g_app.mode = (g_app.prev_mode == VIEW_CONFIG) ? VIEW_CONFIG : VIEW_DASHBOARD;
}

static void handle_confirm_key(struct tb_event *ev) {
    char act_char = (strlen(g_app.confirm_action_label) > 0) ? (char)tolower((unsigned char)g_app.confirm_action_label[0]) : 'y';

    if (ev->key == TB_KEY_ARROW_LEFT || ev->key == TB_KEY_ARROW_RIGHT || ev->key == TB_KEY_TAB) {
        g_app.confirm_selected_btn = 1 - g_app.confirm_selected_btn;
    } else if (ev->key == TB_KEY_ENTER || ev->ch == ' ') {
        if (g_app.confirm_selected_btn == 0) {
            execute_confirm_action();
        } else {
            cancel_confirm_action();
        }
    } else if (ev->ch == 'y' || ev->ch == 'Y' || ev->ch == act_char || ev->ch == (char)toupper((unsigned char)act_char)) {
        execute_confirm_action();
    } else if (ev->ch == 'n' || ev->ch == 'N' || ev->ch == 'c' || ev->ch == 'C' || ev->key == TB_KEY_ESC || ev->ch == 'q' || ev->ch == 'Q' || ev->ch == 'e' || ev->ch == 'E') {
        cancel_confirm_action();
    }
}

static void handle_confirm_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_LEFT) {
        int width = tb_width();
        int height = tb_height();
        int box_w = (width >= 70) ? 66 : (width - 4);
        int box_h = 8;
        int start_x = (width - box_w) / 2;
        int start_y = (height - box_h) / 2;

        if (ev->y == start_y + 5) {
            const char *action_lbl = (strlen(g_app.confirm_action_label) > 0) ? g_app.confirm_action_label : "Confirm";
            int act_len = (int)strlen(action_lbl) + 2;
            int btn0_x = start_x + 4;
            int btn1_x = btn0_x + act_len + 3;

            if (ev->x >= btn0_x && ev->x < btn0_x + act_len) {
                g_app.confirm_selected_btn = 0;
                execute_confirm_action();
            } else if (ev->x >= btn1_x && ev->x < btn1_x + 8) {
                g_app.confirm_selected_btn = 1;
                cancel_confirm_action();
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// Global Event Dispatcher

void handle_event(struct tb_event *ev) {
    if (ev->type == TB_EVENT_KEY) {
        if (ev->key == TB_KEY_CTRL_L || ev->key == TB_KEY_CTRL_R) {
            tb_invalidate();
            ui_draw();
            return;
        }
        switch (g_app.mode) {
            case VIEW_DASHBOARD: handle_dashboard_key(ev); break;
            case VIEW_CONFIG: handle_config_key(ev); break;
            case VIEW_LOGS: handle_logs_key(ev); break;
            case VIEW_PEER_DETAIL: handle_peer_detail_key(ev); break;
            case VIEW_INPUT: handle_input_key(ev); break;
            case VIEW_CONFIRM: handle_confirm_key(ev); break;
            default: break;
        }
    } else if (ev->type == TB_EVENT_MOUSE) {
        switch (g_app.mode) {
            case VIEW_DASHBOARD: handle_dashboard_mouse(ev); break;
            case VIEW_CONFIG: handle_config_mouse(ev); break;
            case VIEW_LOGS: handle_logs_mouse(ev); break;
            case VIEW_PEER_DETAIL: handle_peer_detail_mouse(ev); break;
            case VIEW_CONFIRM: handle_confirm_mouse(ev); break;
            case VIEW_INPUT: handle_input_mouse(ev); break;
            default: break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--install") == 0 || strcmp(argv[1], "-i") == 0) {
            install_zeroscale();
            return 0;
        } else if (strcmp(argv[1], "--uninstall") == 0 || strcmp(argv[1], "-u") == 0) {
            uninstall_zeroscale();
            return 0;
        } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("ZeroScale v0.2.4\n");
            printf("Usage: zeroscale [options]\n\n");
            printf("Options:\n");
            printf("  -i, --install      Install Entware Tailscale and ZeroScale services\n");
            printf("  -u, --uninstall    Uninstall ZeroScale and cleanup crontab/init entries\n");
            printf("  -h, --help         Show this help message\n\n");
            return 0;
        }
    }

    app_init();

    struct tb_event ev;

    while (g_app.running) {
        ui_draw();

        int res = tb_peek_event(&ev, 100);
        if (res == TB_OK) {
            handle_event(&ev);
        }

        // Timer Tick (1 second)
        time_t now = time(NULL);
        if (now - g_app.last_tick >= 1) {
            g_app.last_tick = now;
            if (--g_app.countdown <= 0) {
                g_app.countdown = g_app.config.timerloop > 0 ? g_app.config.timerloop : 60;
                refresh_tailscale_status();
            }
        }

        // Background Periodic Refresh (5 seconds)
        if (now - g_app.last_status_refresh >= 5) {
            g_app.last_status_refresh = now;
            refresh_tailscale_status();
        }

        // Auto-resolve pending digit 1 in config mode after 1.5 seconds
        if (g_app.mode == VIEW_CONFIG && s_config_pending_digit == 1) {
            if (now - s_config_digit_time >= 2) {
                s_config_pending_digit = 0;
                trigger_config_action(0);
            }
        }
    }

    app_cleanup();
    return 0;
}
