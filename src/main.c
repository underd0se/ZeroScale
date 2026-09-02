#define TB_IMPL
#include "app.h"

AppState g_app;

static void handle_sigint(int sig) {
    (void)sig;
    g_app.running = 0;
}

static void handle_fatal_signal(int sig) {
    tb_shutdown();
    const char *signame = "Unknown";
    switch (sig) {
        case SIGSEGV: signame = "SIGSEGV (Segmentation Fault)"; break;
        case SIGBUS:  signame = "SIGBUS (Bus Error)"; break;
        case SIGILL:  signame = "SIGILL (Illegal Instruction)"; break;
        case SIGFPE:  signame = "SIGFPE (Floating Point Exception)"; break;
        case SIGABRT: signame = "SIGABRT (Aborted)"; break;
        case SIGHUP:  signame = "SIGHUP (Hangup / Terminal Closed)"; break;
        default: break;
    }
    fprintf(stderr, "\n[ZeroScale Error] Caught fatal signal %d: %s. Terminal state cleanly restored.\n\n", sig, signame);
    exit(128 + sig);
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
    int saved_mock = g_app.mock_mode;
    memset(&g_app, 0, sizeof(g_app));
    g_app.mock_mode = saved_mock;
    g_app.running = 1;
    g_app.mode = VIEW_DASHBOARD;
    g_app.selected_peer = -1;

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    signal(SIGHUP, handle_fatal_signal);
    signal(SIGSEGV, handle_fatal_signal);
    signal(SIGBUS, handle_fatal_signal);
    signal(SIGILL, handle_fatal_signal);
    signal(SIGFPE, handle_fatal_signal);
    signal(SIGABRT, handle_fatal_signal);

    detect_terminal();
    detect_router_info();
    if (!g_app.mock_mode) {
        sanitize_legacy_symlinks();
    }

    tb_init();
    tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);

    if (g_app.mock_mode) {
        load_mock_data();
    } else {
        load_config();
    }

    // Centered startup splash sequence
    show_splash("INITIALIZING...", 350, TB_HI_BLACK);
    show_splash("INITIALIZING ... DONE", 300, TB_YELLOW | TB_BOLD);
    show_splash(g_app.mock_mode ? "STARTING ZEROSCALE [MOCK]" : "STARTING ZEROSCALE", 300, TB_GREEN | TB_BOLD);

    if (!g_app.mock_mode) {
        refresh_tailscale_status();
    }
    g_app.countdown = g_app.config.timerloop > 0 ? g_app.config.timerloop : 60;
    g_app.last_tick = time(NULL);
    g_app.last_status_refresh = time(NULL);

    log_event("INFO", "ZeroScale v%s session started%s.", g_app.config.version, g_app.mock_mode ? " (Mock Simulator)" : "");
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
    if (!g_app.mock_mode) {
        char buf[512];
        snprintf(buf, sizeof(buf), "( %s ) >/dev/null 2>&1 &", cmd);
        system(buf);
    }
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

void check_and_prompt_zeroscale_update(void) {
    AppConfig *cfg = &g_app.config;
    const char *branch = cfg->track ? "beta" : "main";
    const char *track_name = cfg->track ? "Beta (Development)" : "Stable (Official)";

    char remote_ver[32] = {0};
    if (g_app.mock_mode) {
        snprintf(remote_ver, sizeof(remote_ver), "v0.3.5");
    } else {
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
                 "curl -fsSL --connect-timeout 4 https://raw.githubusercontent.com/underd0se/ZeroScale/%s/install.sh 2>/dev/null | grep -E '^VERSION=' | head -n 1 | cut -d'\"' -f2",
                 branch);
        FILE *f = popen(cmd, "r");
        if (f) {
            if (fgets(remote_ver, sizeof(remote_ver), f)) {
                remote_ver[strcspn(remote_ver, "\r\n")] = 0;
            }
            pclose(f);
        }
    }

    const char *remote_cmp = remote_ver;
    if (*remote_cmp == 'v' || *remote_cmp == 'V') remote_cmp++;

    const char *local_cmp = cfg->version;
    if (*local_cmp == 'v' || *local_cmp == 'V') local_cmp++;

    char prompt[512];
    if (strlen(remote_ver) == 0) {
        snprintf(prompt, sizeof(prompt),
                 "Current Version : v%s (Track: %s)\n"
                 "Remote Version  : [Failed to check network]\n\n"
                 "Do you want to attempt updating anyway?",
                 cfg->version, track_name);
        request_confirm(prompt, "Force Update", "INTERNAL_UPDATE_ZEROSCALE");
    } else if (strcmp(remote_cmp, local_cmp) == 0) {
        snprintf(prompt, sizeof(prompt),
                 "Current Version : v%s (%s)\n"
                 "Remote Version  : %s (Track: %s)\n\n"
                 "You are already on the latest version.\n"
                 "Do you want to force reinstall / update?",
                 cfg->version, track_name, remote_ver, branch);
        request_confirm(prompt, "Reinstall", "INTERNAL_UPDATE_ZEROSCALE");
    } else {
        snprintf(prompt, sizeof(prompt),
                 "Current Version : v%s\n"
                 "Remote Version  : %s\n"
                 "Release Track   : %s\n\n"
                 "A newer version is available on GitHub!\n"
                 "Do you want to download and install this update now?",
                 cfg->version, remote_ver, track_name);
        request_confirm(prompt, "Update Now", "INTERNAL_UPDATE_ZEROSCALE");
    }
}

void update_zeroscale(void) {
    show_splash("UPDATING ZEROSCALE...", 500, TB_CYAN | TB_BOLD);
    const char *branch = g_app.config.track ? "beta" : "main";
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "curl -fsSL https://raw.githubusercontent.com/underd0se/ZeroScale/%s/install.sh | sh -s -- --silent >/dev/null 2>&1",
             branch);
    log_event("INFO", "Manual ZeroScale update triggered (Track: %s).", g_app.config.track ? "Beta" : "Stable");
    int res = system(cmd);
    if (res == 0) {
        show_splash("UPDATE COMPLETE - RESTARTING...", 500, TB_GREEN | TB_BOLD);
        tb_shutdown();
        printf("\n[+] ZeroScale successfully updated to latest release on '%s' track!\n", branch);
        printf("[*] Relaunching ZeroScale...\n\n");
        fflush(stdout);
        execl("/jffs/scripts/zeroscale", "zeroscale", (char *)NULL);
        exit(0);
    } else {
        show_toast("Update failed! Check internet connection.");
        log_event("FAIL", "ZeroScale update failed (curl error code %d).", res);
        g_app.mode = VIEW_CONFIG;
        tb_invalidate();
    }
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
        case 0: { // Up
            char cmd[512];
            build_tailscale_up_cmd(cmd, sizeof(cmd));
            execute_action("Connecting Tailscale (tailscale up)...", cmd);
            break;
        }
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
    if (g_app.peer_filter_active) {
        if (ev->key == TB_KEY_ESC) {
            g_app.peer_filter_active = 0;
            g_app.peer_filter[0] = '\0';
            apply_peer_filter_and_sort();
            show_toast("Search cleared");
            return;
        }
        if (ev->key == TB_KEY_ENTER) {
            g_app.peer_filter_active = 0;
            g_app.dash_focus = FOCUS_PEERS;
            if (g_app.selected_peer < 0 && g_app.filtered_count > 0) g_app.selected_peer = 0;
            return;
        }
        if (ev->key == TB_KEY_BACKSPACE || ev->key == TB_KEY_BACKSPACE2) {
            size_t len = strlen(g_app.peer_filter);
            if (len > 0) {
                g_app.peer_filter[len - 1] = '\0';
                apply_peer_filter_and_sort();
            }
            return;
        }
        if (ev->key == TB_KEY_CTRL_U || ev->key == TB_KEY_CTRL_K) {
            g_app.peer_filter[0] = '\0';
            apply_peer_filter_and_sort();
            return;
        }
        if (ev->ch >= 32 && ev->ch <= 126) {
            size_t len = strlen(g_app.peer_filter);
            if (len < sizeof(g_app.peer_filter) - 2) {
                g_app.peer_filter[len] = (char)ev->ch;
                g_app.peer_filter[len + 1] = '\0';
                apply_peer_filter_and_sort();
            }
            return;
        }
        return;
    }

    if (ev->ch == '/' || ev->ch == 'f' || ev->ch == 'F') {
        g_app.peer_filter_active = 1;
        return;
    } else if (ev->ch == 'o' || ev->ch == 'O') {
        cycle_peer_sort();
        return;
    } else if (ev->ch == '+' || ev->ch == '=' || ev->ch == ']') {
        step_timerloop(1);
        return;
    } else if (ev->ch == '-' || ev->ch == '_' || ev->ch == '[') {
        step_timerloop(-1);
        return;
    } else if (ev->key == TB_KEY_ESC) {
        if (strlen(g_app.peer_filter) > 0) {
            g_app.peer_filter[0] = '\0';
            apply_peer_filter_and_sort();
            show_toast("Filter cleared");
        } else {
            g_app.running = 0;
        }
        return;
    } else if (ev->ch == 'q' || ev->ch == 'Q') {
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
            if (g_app.selected_peer < 0 && g_app.filtered_count > 0) g_app.selected_peer = 0;
        } else if (g_app.dash_focus == FOCUS_PEERS) {
            if (g_app.selected_peer < g_app.filtered_count - 1) g_app.selected_peer++;
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
            if (g_app.selected_peer >= 0 && g_app.selected_peer < g_app.filtered_count) {
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
            if (width < 80) {
                int x0 = 1;
                int b0 = x0;
                int b1 = x0 + 5;
                int b2 = x0 + 14;
                int b3 = x0 + 21;
                int b4 = x0 + 29;
                int b5 = x0 + 38;
                int b6 = x0 + 46;
                int b7 = x0 + 54;

                if (ev->x >= b0 && ev->x < b0 + 4) {
                    trigger_header_action(0);
                } else if (ev->x >= b1 && ev->x < b1 + 6) {
                    trigger_header_action(1);
                } else if (ev->x >= b2 && ev->x < b2 + 6) {
                    trigger_header_action(2);
                } else if (ev->x >= b3 && ev->x < b3 + 7) {
                    trigger_header_action(3);
                } else if (ev->x >= b4 && ev->x < b4 + 6) {
                    trigger_header_action(4);
                } else if (ev->x >= b5 && ev->x < b5 + 5) {
                    trigger_header_action(5);
                } else if (ev->x >= b6 && ev->x < b6 + 5) {
                    trigger_header_action(6);
                } else if (ev->x >= b7 && ev->x < b7 + 6) {
                    trigger_header_action(7);
                }
            } else {
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
            }
        } else if (ev->y >= 10 && ev->y < 10 + g_app.filtered_count) {
            int clicked_idx = (ev->y - 10) + g_app.peer_scroll;
            if (clicked_idx < g_app.filtered_count) {
                g_app.dash_focus = FOCUS_PEERS;
                if (g_app.selected_peer == clicked_idx) {
                    g_app.peer_detail_selected_btn = 0;
                    g_app.mode = VIEW_PEER_DETAIL;
                } else {
                    g_app.selected_peer = clicked_idx;
                    int peer_idx = g_app.filtered_indices[clicked_idx];
                    show_toast("Selected: %s (%s) — Press Enter or click again for details", 
                               g_app.peers[peer_idx].name, g_app.peers[peer_idx].ip);
                }
            }
        }
    } else if (ev->key == TB_KEY_MOUSE_WHEEL_UP) {
        if (g_app.peer_scroll > 0) g_app.peer_scroll--;
    } else if (ev->key == TB_KEY_MOUSE_WHEEL_DOWN) {
        if (g_app.peer_scroll < g_app.filtered_count - 5) g_app.peer_scroll++;
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
        case 9: {
            if (!is_amtm_email_configured()) {
                cycle_amtm_email();
            } else if (!g_app.config.amtmemailsuccess && !g_app.config.amtmemailfailure) {
                cycle_amtm_email();
            } else {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", g_app.config.ratelimit > 0 ? g_app.config.ratelimit : 5);
                request_input(INPUT_RATELIMIT, "amtm Email Rate Limit", "Enter max alert emails per hour (1-24):", buf);
            }
            break;
        }
        case 10: {
            if (!g_app.config.schedule) {
                cycle_schedule();
            } else {
                char buf[16];
                snprintf(buf, sizeof(buf), "%02d:%02d", g_app.config.schedulehrs, g_app.config.schedulemin);
                request_input(INPUT_SCHEDULE, "Auto-Update Time (24h)", "Enter run time (HH:MM or 0-23) or 'off' to disable:", buf);
            }
            break;
        }
        case 11: switch_track(); break;
        case 12:
            check_and_prompt_zeroscale_update();
            break;
        case 13:
            request_confirm("Update Tailscale binary to latest version?",
                            "Update TS",
                            "/opt/bin/opkg update && /opt/bin/opkg upgrade tailscale || tailscale update --yes; /opt/etc/init.d/S06tailscaled restart");
            break;
        case 14:
            request_confirm("Reset daemon state and re-authenticate?",
                            "Reset State",
                            "/opt/etc/init.d/S06tailscaled stop; rm -f /opt/var/tailscaled.state; /opt/etc/init.d/S06tailscaled start");
            break;
        case 15:
            request_confirm("Reinstall Entware Tailscale package?",
                            "Reinstall TS",
                            "/opt/etc/init.d/S06tailscaled stop 2>/dev/null; /opt/bin/opkg update; /opt/bin/opkg install --force-reinstall tailscale; /opt/etc/init.d/S06tailscaled start");
            break;
        case 16:
            request_confirm("Completely uninstall ZeroScale from router?",
                            "Uninstall",
                            "INTERNAL_UNINSTALL");
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

    int width = tb_width();
    int is_2col = (width >= 86);

    if (s_config_pending_digit == 1) {
        s_config_pending_digit = 0;
        if (ev->ch >= '0' && ev->ch <= '7') {
            int num = 10 + (ev->ch - '0');
            if (num >= 10 && num <= 17) {
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
        if (g_app.config_selected_idx < 16) g_app.config_selected_idx++;
    } else if (ev->key == TB_KEY_ARROW_UP) {
        if (g_app.config_selected_idx > 0) g_app.config_selected_idx--;
    } else if (is_2col && ev->key == TB_KEY_ARROW_RIGHT) {
        if (g_app.config_selected_idx < 9) {
            g_app.config_selected_idx += 9;
            if (g_app.config_selected_idx > 16) g_app.config_selected_idx = 16;
        }
    } else if (is_2col && ev->key == TB_KEY_ARROW_LEFT) {
        if (g_app.config_selected_idx >= 9) {
            g_app.config_selected_idx -= 9;
        }
    } else if (ev->key == TB_KEY_ENTER) {
        if (g_app.config_selected_idx == 3 && strcasecmp(g_app.config.opmode, "Custom") == 0) {
            request_input(INPUT_CUSTOMPARAMS, "Custom Tailscale Flags", "Enter custom tailscale up flags (e.g. --accept-routes)", g_app.config.customparams);
        } else {
            trigger_config_action(g_app.config_selected_idx);
        }
    } else if (ev->ch == ' ') {
        if (g_app.config_selected_idx == 9) {
            cycle_amtm_email();
        } else if (g_app.config_selected_idx == 10) {
            cycle_schedule();
        } else {
            trigger_config_action(g_app.config_selected_idx);
        }
    } else if (ev->ch == '1') {
        s_config_pending_digit = 1;
        s_config_digit_time = time(NULL);
        g_app.config_selected_idx = 0;
        show_toast("Option (1)... [Press 0-7 for (10)-(17), or Enter for (1)]");
    } else if (ev->ch == '4') {
        if (strcasecmp(g_app.config.opmode, "Custom") == 0) {
            request_input(INPUT_CUSTOMPARAMS, "Custom Tailscale Flags", "Enter custom tailscale up flags (e.g. --accept-routes)", g_app.config.customparams);
        } else {
            trigger_config_action(3);
        }
    } else if (ev->ch >= '2' && ev->ch <= '9') {
        trigger_config_action(ev->ch - '1');
    } else if (ev->ch == 't' || ev->ch == 'T') {
        trigger_config_action(11);
    } else if (ev->ch == 'z' || ev->ch == 'Z') {
        trigger_config_action(12);
    } else if (ev->ch == 'u' || ev->ch == 'U') {
        trigger_config_action(13);
    } else if (ev->ch == 'x' || ev->ch == 'X') {
        trigger_config_action(14);
    } else if (ev->ch == 'i' || ev->ch == 'I') {
        trigger_config_action(15);
    }
}

static void handle_config_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_LEFT) {
        int width = tb_width();
        int height = tb_height();
        if (width >= 86) {
            // 2-Column Mode
            if (ev->x < 47) {
                // Left Column (Options 0..8)
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
                    default: break;
                }
            } else {
                // Right Column (Options 9..16)
                switch (ev->y) {
                    case 5: trigger_config_action(9); break;
                    case 6: trigger_config_action(10); break;
                    case 7: trigger_config_action(11); break;
                    case 10: trigger_config_action(12); break;
                    case 11: trigger_config_action(13); break;
                    case 12: trigger_config_action(14); break;
                    case 13: trigger_config_action(15); break;
                    case 14: trigger_config_action(16); break;
                    default: break;
                }
            }
        } else {
            // Single Column Scrolled Viewport
            int max_visible = height - 9;
            if (max_visible < 5) max_visible = 5;
            if (max_visible > 17) max_visible = 17;

            if (ev->y >= 5 && ev->y < 5 + max_visible) {
                int clicked_idx = g_app.config_scroll + (ev->y - 5);
                if (clicked_idx >= 0 && clicked_idx < 17) {
                    trigger_config_action(clicked_idx);
                }
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Peer Detail Modal Handlers

static void do_peer_ping(void) {
    if (g_app.selected_peer >= 0 && g_app.selected_peer < g_app.filtered_count) {
        int peer_idx = g_app.filtered_indices[g_app.selected_peer];
        PeerInfo *p = &g_app.peers[peer_idx];
        if (g_app.mock_mode) {
            show_toast("[Mock Ping] %s (%s): 11.8 ms", p->name, p->ip);
            return;
        }
        show_toast("Pinging %s (%s)...", p->name, p->ip);
        ui_draw();
        tb_present();

        char cmd[256];
#if defined(__APPLE__) || defined(__MACH__)
        snprintf(cmd, sizeof(cmd), "ping -c 1 -t 1 %s 2>&1", p->ip);
#else
        snprintf(cmd, sizeof(cmd), "ping -c 1 -W 1 %s 2>&1", p->ip);
#endif

        FILE *f = popen(cmd, "r");
        int success = 0;
        char latency[32] = {0};

        if (f) {
            char line[128];
            while (fgets(line, sizeof(line), f)) {
                char *time_str = strstr(line, "time=");
                if (time_str) {
                    sscanf(time_str + 5, "%31s", latency);
                    success = 1;
                }
            }
            int code = pclose(f);
            if (code == 0) success = 1;
        }

        if (success && strlen(latency) > 0) {
            show_toast("Ping to %s: SUCCESS (%s)", p->name, latency);
        } else if (success) {
            show_toast("Ping to %s: SUCCESS (Host Online)", p->name);
        } else {
            show_toast("Ping to %s: FAILED / TIMEOUT (1s)", p->name);
        }
        tb_invalidate();
    }
}

static void do_peer_ts_ping(void) {
    if (g_app.selected_peer >= 0 && g_app.selected_peer < g_app.filtered_count) {
        int peer_idx = g_app.filtered_indices[g_app.selected_peer];
        PeerInfo *p = &g_app.peers[peer_idx];
        if (g_app.mock_mode) {
            show_toast("[Mock Tailscale Ping] direct to %s: 14.2 ms", p->name);
            return;
        }
        show_toast("Testing Tailscale WireGuard latency to %s...", p->name);
        ui_draw();
        tb_present();

        char cmd[256];
        snprintf(cmd, sizeof(cmd), "tailscale ping --timeout=1s -c 1 %s 2>&1", p->ip);

        FILE *f = popen(cmd, "r");
        int success = 0;
        char latency_info[64] = {0};

        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strstr(line, "pong from") != NULL) {
                    char *in_pos = strstr(line, " in ");
                    char *via_pos = strstr(line, " via ");
                    if (in_pos) {
                        snprintf(latency_info, sizeof(latency_info), "%s", in_pos + 4);
                        latency_info[strcspn(latency_info, "\r\n")] = 0;
                    } else if (via_pos) {
                        snprintf(latency_info, sizeof(latency_info), "%s", via_pos + 5);
                        latency_info[strcspn(latency_info, "\r\n")] = 0;
                    }
                    success = 1;
                    break;
                }
            }
            pclose(f);
        }

        if (success && strlen(latency_info) > 0) {
            show_toast("Tailscale Ping to %s: Connected in %s", p->name, latency_info);
        } else if (success) {
            show_toast("Tailscale Ping to %s: Connected!", p->name);
        } else {
            show_toast("Tailscale Ping to %s: Unreachable / Timeout (1s)", p->name);
        }
        tb_invalidate();
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
        int box_w = (width >= 86) ? 76 : (width - 4);
        if (box_w < 38) box_w = width - 2;
        int box_h = 15;
        int start_x = (width - box_w) / 2;
        int start_y = (height - box_h) / 2;

        if (ev->y == start_y + 13) {
            if (box_w >= 54) {
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
            } else {
                if (ev->x >= start_x + 2 && ev->x < start_x + 9) {
                    g_app.peer_detail_selected_btn = 0;
                    do_peer_ping();
                } else if (ev->x >= start_x + 11 && ev->x < start_x + 21) {
                    g_app.peer_detail_selected_btn = 1;
                    do_peer_ts_ping();
                } else if (ev->x >= start_x + 23 && ev->x < start_x + 32) {
                    g_app.peer_detail_selected_btn = 2;
                    g_app.mode = VIEW_DASHBOARD;
                }
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
        char validated[128] = {0};
        if (strlen(g_app.input_buf) > 0) {
            if (!validate_cidr_list(g_app.input_buf, validated, sizeof(validated))) {
                show_toast("Invalid CIDR format (e.g. 192.168.1.0/24)");
                return;
            }
            snprintf(g_app.config.routes, sizeof(g_app.config.routes), "%s", validated);
        } else {
            snprintf(g_app.config.routes, sizeof(g_app.config.routes), "%s", g_app.input_buf);
        }
        save_config();
        log_event("INFO", "Subnet Routes updated to: %s", g_app.config.routes);
        show_toast("Subnet Routes updated: %s", g_app.config.routes);
    } else if (g_app.input_target == INPUT_LOGSIZE) {
        int size = atoi(g_app.input_buf);
        if (size < 100) size = 100;
        if (size > 10000) size = 10000;
        g_app.config.logsize = size;
        save_config();
        log_event("INFO", "Log retention updated to: %d rows.", g_app.config.logsize);
        show_toast("Log Retention updated: %d rows", g_app.config.logsize);
    } else if (g_app.input_target == INPUT_CUSTOMPARAMS) {
        char sanitized[256] = {0};
        sanitize_custom_flags(g_app.input_buf, sanitized, sizeof(sanitized));
        snprintf(g_app.config.customparams, sizeof(g_app.config.customparams), "%s", sanitized);
        save_config();
        log_event("INFO", "Custom Tailscale flags updated to: %s", g_app.config.customparams);
        show_toast("Custom Flags updated: %s", g_app.config.customparams);
    } else if (g_app.input_target == INPUT_SCHEDULE) {
        char val[32];
        strncpy(val, g_app.input_buf, sizeof(val) - 1);
        val[sizeof(val) - 1] = '\0';
        char *p = val;
        while (*p == ' ' || *p == '\t') p++;
        size_t l = strlen(p);
        while (l > 0 && (p[l - 1] == ' ' || p[l - 1] == '\t' || p[l - 1] == '\r' || p[l - 1] == '\n')) p[--l] = '\0';

        if (strcasecmp(p, "off") == 0 || strcasecmp(p, "disable") == 0 || strcasecmp(p, "disabled") == 0 || strcmp(p, "0") == 0 || strcmp(p, "none") == 0) {
            g_app.config.schedule = 0;
            if (!g_app.mock_mode) {
                system("cru d zeroscale_autoupdate >/dev/null 2>&1");
            }
            save_config();
            log_event("INFO", "Auto-update schedule disabled.");
            show_toast("Auto-Update Schedule: Disabled");
        } else {
            int h = 1, m = 0;
            if (strchr(p, ':')) {
                if (sscanf(p, "%d:%d", &h, &m) < 1) {
                    show_toast("Invalid time format (use HH:MM, e.g. 03:30)");
                    return;
                }
            } else {
                h = atoi(p);
                m = 0;
            }
            if (h < 0 || h > 23 || m < 0 || m > 59) {
                show_toast("Invalid time (Hours: 0-23, Minutes: 0-59)");
                return;
            }
            g_app.config.schedule = 1;
            g_app.config.schedulehrs = h;
            g_app.config.schedulemin = m;
            if (!g_app.mock_mode) {
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "cru a zeroscale_autoupdate '%d %d * * * /jffs/scripts/zeroscale --check-update >/dev/null 2>&1' >/dev/null 2>&1", m, h);
                system(cmd);
            }
            save_config();
            log_event("INFO", "Auto-update scheduled for %02d:%02d.", h, m);
            show_toast("Auto-Update scheduled @ %02d:%02d", h, m);
        }
        tb_invalidate();
    } else if (g_app.input_target == INPUT_RATELIMIT) {
        int rate = atoi(g_app.input_buf);
        if (rate < 1) rate = 1;
        if (rate > 24) rate = 24;
        g_app.config.ratelimit = rate;
        save_config();
        log_event("INFO", "amtm email rate limit updated to %d/hour.", rate);
        show_toast("amtm Rate Limit: %d emails/hour", rate);
        tb_invalidate();
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
        int box_w = (width >= 82) ? 76 : (width - 4);
        if (box_w < 38) box_w = width - 2;
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
            if (ev->x >= start_x + 3 && ev->x < start_x + 11) {
                g_app.input_selected_btn = 1;
                save_input_action();
            } else if (ev->x >= start_x + 13 && ev->x < start_x + 23) {
                g_app.input_selected_btn = 2;
                cancel_input_action();
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// View: Confirm Dialog Handlers

static void execute_confirm_action(void) {
    if (strcmp(g_app.confirm_cmd, "INTERNAL_UPDATE_ZEROSCALE") == 0) {
        update_zeroscale();
        return;
    }
    if (strcmp(g_app.confirm_cmd, "INTERNAL_UNINSTALL") == 0) {
        uninstall_zeroscale();
        return;
    }
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
    if (ev->key == TB_KEY_ESC || ev->ch == 'c' || ev->ch == 'C' || ev->ch == 'q' || ev->ch == 'Q' || ev->ch == 'e' || ev->ch == 'E') {
        cancel_confirm_action();
    } else if (ev->key == TB_KEY_ARROW_LEFT || ev->key == TB_KEY_ARROW_RIGHT || ev->key == TB_KEY_TAB) {
        g_app.confirm_selected_btn = 1 - g_app.confirm_selected_btn;
    } else if (ev->key == TB_KEY_ENTER || ev->ch == ' ') {
        if (g_app.confirm_selected_btn == 0) {
            execute_confirm_action();
        } else {
            cancel_confirm_action();
        }
    }
}

static void handle_confirm_mouse(struct tb_event *ev) {
    if (ev->key == TB_KEY_MOUSE_LEFT) {
        int width = tb_width();
        int height = tb_height();

        int line_count = 1;
        for (const char *p = g_app.confirm_prompt; *p; p++) {
            if (*p == '\n') line_count++;
        }

        int box_w = (width >= 72) ? 66 : (width - 4);
        if (box_w < 38) box_w = width - 2;
        int box_h = 6 + line_count;
        if (box_h < 8) box_h = 8;
        int start_x = (width - box_w) / 2;
        int start_y = (height - box_h) / 2;

        const char *action_lbl = (strlen(g_app.confirm_action_label) > 0) ? g_app.confirm_action_label : "Confirm";
        int act_len = (int)strlen(action_lbl) + 2;
        int btn0_x = start_x + 3;
        int btn1_x = btn0_x + act_len + 3;
        int btn_y = start_y + 2 + line_count + 1;

        if (ev->y == btn_y) {
            if (ev->x >= btn0_x && ev->x < btn0_x + act_len + 2) {
                g_app.confirm_selected_btn = 0;
                execute_confirm_action();
            } else if (ev->x >= btn1_x && ev->x < btn1_x + 10) {
                g_app.confirm_selected_btn = 1;
                cancel_confirm_action();
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------
// Global Event Dispatcher & Main Entry Loop

void handle_event(struct tb_event *ev) {
    if (ev->type == TB_EVENT_RESIZE) {
        tb_invalidate();
        return;
    }

    if (ev->type == TB_EVENT_KEY) {
        if (ev->key == TB_KEY_CTRL_L || ev->key == TB_KEY_CTRL_R) {
            tb_invalidate();
            show_toast("Screen Redrawn");
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
        if (strcmp(argv[1], "--mock") == 0 || strcmp(argv[1], "-m") == 0) {
            g_app.mock_mode = 1;
        } else if (strcmp(argv[1], "--check-update") == 0 || strcmp(argv[1], "-c") == 0) {
            return run_headless_update();
        } else if (strcmp(argv[1], "--install") == 0 || strcmp(argv[1], "-i") == 0) {
            install_zeroscale();
            return 0;
        } else if (strcmp(argv[1], "--uninstall") == 0 || strcmp(argv[1], "-u") == 0) {
            uninstall_zeroscale();
            return 0;
        } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("ZeroScale v0.3.5\n");
            printf("Usage: zeroscale [options]\n\n");
            printf("Options:\n");
            printf("  -c, --check-update Run headless background update check (crontab mode)\n");
            printf("  -m, --mock         Run in local desktop simulation mode with synthetic data\n");
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
