#define TB_IMPL
#include "app.h"

AppState g_app;

static void handle_sigint(int sig) {
    (void)sig;
    g_app.running = 0;
}

void app_init(void) {
    memset(&g_app, 0, sizeof(g_app));
    g_app.running = 1;
    g_app.mode = VIEW_DASHBOARD;
    g_app.selected_peer = -1;

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

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
}

void app_cleanup(void) {
    // Centered exit splash sequence
    show_splash("SHUTTING DOWN...", 350, TB_YELLOW | TB_BOLD);
    show_splash("GOODBYE...", 500, TB_HI_BLACK);
    tb_shutdown();
}

void request_confirm(const char *prompt, const char *cmd) {
    snprintf(g_app.confirm_prompt, sizeof(g_app.confirm_prompt), "%s", prompt);
    snprintf(g_app.confirm_cmd, sizeof(g_app.confirm_cmd), "%s", cmd);
    g_app.mode = VIEW_CONFIRM;
}

void request_input(InputTarget target, const char *title, const char *prompt, const char *initial) {
    g_app.input_target = target;
    snprintf(g_app.input_title, sizeof(g_app.input_title), "%s", title);
    snprintf(g_app.input_prompt, sizeof(g_app.input_prompt), "%s", prompt);
    snprintf(g_app.input_buf, sizeof(g_app.input_buf), "%s", initial);
    g_app.input_cursor = (int)strlen(g_app.input_buf);
    g_app.mode = VIEW_INPUT;
}

void execute_action(const char *action, const char *cmd) {
    show_toast("%s", action);
    char buf[512];
    snprintf(buf, sizeof(buf), "%s >/dev/null 2>&1 &", cmd);
    system(buf);
    refresh_tailscale_status();
}

void install_zeroscale(void) {
    tb_shutdown();
    printf("\n=== Running Entware Tailscale Installer ===\n\n");
    system("opkg update && opkg install tailscale && /opt/etc/init.d/S06tailscaled start");
    printf("\nPress Enter to return to ZeroScale TUI...");
    getchar();
    tb_init();
    tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
    show_splash("INSTALLATION COMPLETE", 600, TB_GREEN | TB_BOLD);
    load_config();
    refresh_tailscale_status();
}

void uninstall_zeroscale(void) {
    show_splash("UNINSTALLING ZEROSCALE...", 600, TB_RED | TB_BOLD);
    system("/opt/etc/init.d/S06tailscaled stop 2>/dev/null; "
           "sed -i -e '/zeroscale/d' -e '/tailmon/d' /jffs/scripts/post-mount 2>/dev/null; "
           "cru d zeroscale_autoupdate 2>/dev/null");
    show_splash("UNINSTALL COMPLETE", 800, TB_HI_BLACK);
    tb_shutdown();
    printf("\n[ZeroScale Successfully Uninstalled from Router]\n\n");
    exit(0);
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Dashboard Handlers

static void handle_dashboard_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC || ev->ch == 'q' || ev->ch == 'Q') {
        g_app.running = 0;
    } else if (ev->ch == 'u' || ev->ch == 'U') {
        execute_action("Connecting Tailscale (tailscale up)...", "tailscale up");
    } else if (ev->ch == 'd' || ev->ch == 'D') {
        request_confirm("Disconnect Tailscale (tailscale down)?", "tailscale down");
    } else if (ev->ch == 'r' || ev->ch == 'R') {
        request_confirm("Restart Tailscale Daemon?", "/opt/etc/init.d/S06tailscaled restart");
    } else if (ev->ch == 's' || ev->ch == 'S') {
        execute_action("Starting Tailscale Daemon...", "/opt/etc/init.d/S06tailscaled start");
    } else if (ev->ch == 't' || ev->ch == 'T') {
        request_confirm("Stop Tailscale Daemon?", "/opt/etc/init.d/S06tailscaled stop");
    } else if (ev->ch == 'l' || ev->ch == 'L') {
        load_logs();
        g_app.mode = VIEW_LOGS;
    } else if (ev->ch == 'c' || ev->ch == 'C') {
        g_app.mode = VIEW_CONFIG;
    } else if (ev->key == TB_KEY_ENTER) {
        if (g_app.selected_peer >= 0 && g_app.selected_peer < g_app.peer_count) {
            g_app.mode = VIEW_PEER_DETAIL;
        }
    } else if (ev->key == TB_KEY_ARROW_UP) {
        if (g_app.selected_peer > 0) g_app.selected_peer--;
        else if (g_app.selected_peer == -1 && g_app.peer_count > 0) g_app.selected_peer = 0;
        if (g_app.selected_peer < g_app.peer_scroll) g_app.peer_scroll = g_app.selected_peer;
    } else if (ev->key == TB_KEY_ARROW_DOWN) {
        if (g_app.selected_peer < g_app.peer_count - 1) g_app.selected_peer++;
        int max_rows = tb_height() - 13;
        if (g_app.selected_peer >= g_app.peer_scroll + max_rows) g_app.peer_scroll++;
    }
}

static void handle_dashboard_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_LEFT) {
        // Row 5 Action Bar Click
        if (ev->y == 5) {
            if (ev->x >= 2 && ev->x <= 6) {
                execute_action("Connecting Tailscale (tailscale up)...", "tailscale up");
            } else if (ev->x >= 7 && ev->x <= 13) {
                request_confirm("Disconnect Tailscale (tailscale down)?", "tailscale down");
            } else if (ev->x >= 17 && ev->x <= 25) {
                request_confirm("Restart Tailscale Daemon?", "/opt/etc/init.d/S06tailscaled restart");
            } else if (ev->x >= 27 && ev->x <= 33) {
                execute_action("Starting Tailscale Daemon...", "/opt/etc/init.d/S06tailscaled start");
            } else if (ev->x >= 35 && ev->x <= 41) {
                request_confirm("Stop Tailscale Daemon?", "/opt/etc/init.d/S06tailscaled stop");
            } else if (ev->x >= 46 && ev->x <= 52) {
                load_logs();
                g_app.mode = VIEW_LOGS;
            } else if (ev->x >= 56 && ev->x <= 72) {
                g_app.mode = VIEW_CONFIG;
            } else if (ev->x >= 76 && ev->x <= 84) {
                g_app.running = 0;
            }
        } else if (ev->y >= 10 && ev->y < 10 + g_app.peer_count) {
            int clicked_idx = (ev->y - 10) + g_app.peer_scroll;
            if (clicked_idx < g_app.peer_count) {
                if (g_app.selected_peer == clicked_idx) {
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

static void handle_config_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC || ev->ch == 'q' || ev->ch == 'Q') {
        g_app.mode = VIEW_DASHBOARD;
    } else if (ev->ch == '1') {
        toggle_keepalive();
    } else if (ev->ch == '2') {
        toggle_persistentsettings();
    } else if (ev->ch == '3') {
        toggle_autostart();
    } else if (ev->ch == '4') {
        cycle_opmode();
    } else if (ev->ch == '5') {
        toggle_exitnode();
    } else if (ev->ch == '6') {
        toggle_advroutes();
    } else if (ev->ch == '7') {
        request_input(INPUT_ROUTES, "Subnet Routes CIDR", "Enter subnet CIDR to advertise", g_app.config.routes);
    } else if (ev->ch == '8') {
        cycle_timerloop();
    } else if (ev->ch == '9') {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", g_app.config.logsize);
        request_input(INPUT_LOGSIZE, "Event Log Retention", "Enter max log rows (100-9999, 0=Disable)", buf);
    } else if (ev->ch == '0' || ev->ch == 'a' || ev->ch == 'A') {
        cycle_amtm_email();
    } else if (ev->ch == 's' || ev->ch == 'S') {
        cycle_schedule();
    } else if (ev->ch == 'u' || ev->ch == 'U') {
        tb_shutdown();
        system("sh /jffs/scripts/zeroscale.sh -update");
        tb_init();
        tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
        refresh_tailscale_status();
    } else if (ev->ch == 'x' || ev->ch == 'X') {
        request_confirm("Reset Tailscale Daemon State?", "/opt/etc/init.d/S06tailscaled stop; rm -f /opt/var/tailscaled.state; /opt/etc/init.d/S06tailscaled start");
    } else if (ev->ch == 'i' || ev->ch == 'I') {
        install_zeroscale();
    }
}

static void handle_config_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_LEFT) {
        switch (ev->y) {
            case 5: toggle_keepalive(); break;
            case 6: toggle_persistentsettings(); break;
            case 7: toggle_autostart(); break;
            case 10: cycle_opmode(); break;
            case 11: toggle_exitnode(); break;
            case 12: toggle_advroutes(); break;
            case 13: request_input(INPUT_ROUTES, "Subnet Routes CIDR", "Enter subnet CIDR to advertise", g_app.config.routes); break;
            case 16: cycle_timerloop(); break;
            case 17: {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", g_app.config.logsize);
                request_input(INPUT_LOGSIZE, "Event Log Retention", "Enter max log rows (100-9999, 0=Disable)", buf);
                break;
            }
            case 20: cycle_amtm_email(); break;
            case 21: cycle_schedule(); break;
            case 24:
                tb_shutdown();
                system("sh /jffs/scripts/zeroscale.sh -update");
                tb_init();
                tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
                refresh_tailscale_status();
                break;
            case 25:
                request_confirm("Reset Tailscale Daemon State?", "/opt/etc/init.d/S06tailscaled stop; rm -f /opt/var/tailscaled.state; /opt/etc/init.d/S06tailscaled start");
                break;
            case 26:
                install_zeroscale();
                break;
            case 27:
                request_confirm("Uninstall ZeroScale completely?", "killall -9 zeroscale-tui 2>/dev/null; /opt/etc/init.d/S06tailscaled stop; sed -i -e '/zeroscale/d' -e '/tailmon/d' /jffs/scripts/post-mount 2>/dev/null; cru d zeroscale_autoupdate 2>/dev/null; rm -rf /jffs/addons/zeroscale.d /jffs/scripts/zeroscale-tui /opt/bin/zeroscale-tui");
                break;
            default: break;
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Peer Detail Modal Handlers

static void handle_peer_detail_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC || ev->ch == 'c' || ev->ch == 'C' || ev->ch == 'q' || ev->ch == 'Q') {
        g_app.mode = VIEW_DASHBOARD;
    } else if (ev->ch == 'p' || ev->ch == 'P') {
        if (g_app.selected_peer >= 0 && g_app.selected_peer < g_app.peer_count) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "ping -c 3 %s >/dev/null 2>&1", g_app.peers[g_app.selected_peer].ip);
            show_toast("Pinging %s (%s)...", g_app.peers[g_app.selected_peer].name, g_app.peers[g_app.selected_peer].ip);
            int res = system(cmd);
            if (res == 0) show_toast("Ping to %s: SUCCESS (Host Online)", g_app.peers[g_app.selected_peer].name);
            else show_toast("Ping to %s: FAILED / TIMEOUT", g_app.peers[g_app.selected_peer].name);
        }
    } else if (ev->ch == 't' || ev->ch == 'T') {
        if (g_app.selected_peer >= 0 && g_app.selected_peer < g_app.peer_count) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "tailscale ping -c 1 %s >/dev/null 2>&1", g_app.peers[g_app.selected_peer].ip);
            show_toast("Testing Tailscale wireguard latency to %s...", g_app.peers[g_app.selected_peer].name);
            int res = system(cmd);
            if (res == 0) show_toast("Tailscale Ping to %s: Connected!", g_app.peers[g_app.selected_peer].name);
            else show_toast("Tailscale Ping to %s: Unreachable", g_app.peers[g_app.selected_peer].name);
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Log Viewer Handlers

static void handle_logs_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC || ev->ch == 'q' || ev->ch == 'Q') {
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

static void handle_input_key(struct tb_event *ev) {
    if (ev->key == TB_KEY_ESC) {
        g_app.mode = VIEW_CONFIG;
    } else if (ev->key == TB_KEY_ENTER) {
        if (g_app.input_target == INPUT_ROUTES) {
            snprintf(g_app.config.routes, sizeof(g_app.config.routes), "%s", g_app.input_buf);
            save_config();
            show_toast("Subnet Routes updated to: %s", g_app.config.routes);
        } else if (g_app.input_target == INPUT_LOGSIZE) {
            int size = atoi(g_app.input_buf);
            g_app.config.logsize = size;
            save_config();
            show_toast("Log Retention updated to: %d rows", g_app.config.logsize);
        }
        g_app.mode = VIEW_CONFIG;
    } else if (ev->key == TB_KEY_BACKSPACE || ev->key == TB_KEY_BACKSPACE2) {
        if (g_app.input_cursor > 0) {
            g_app.input_buf[--g_app.input_cursor] = '\0';
        }
    } else if (ev->ch >= 32 && ev->ch <= 126) {
        if (g_app.input_cursor < (int)sizeof(g_app.input_buf) - 2) {
            g_app.input_buf[g_app.input_cursor++] = (char)ev->ch;
            g_app.input_buf[g_app.input_cursor] = '\0';
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Confirm Dialog Handlers

static void handle_confirm_key(struct tb_event *ev) {
    if (ev->ch == 'y' || ev->ch == 'Y') {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s >/dev/null 2>&1 &", g_app.confirm_cmd);
        system(buf);
        show_toast("Action executed.");
        g_app.mode = VIEW_DASHBOARD;
        refresh_tailscale_status();
    } else if (ev->ch == 'n' || ev->ch == 'N' || ev->key == TB_KEY_ESC) {
        show_toast("Action cancelled.");
        g_app.mode = VIEW_DASHBOARD;
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// Global Event Dispatcher

void handle_event(struct tb_event *ev) {
    if (ev->type == TB_EVENT_KEY) {
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
            printf("ZeroScale C-TUI v0.1.0\n");
            printf("Usage: zeroscale-tui [options]\n\n");
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
    }

    app_cleanup();
    return 0;
}
