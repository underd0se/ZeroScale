#include "app.h"
#include <stdarg.h>

void show_toast(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_app.toast_msg, sizeof(g_app.toast_msg), fmt, args);
    va_end(args);
    g_app.toast_expiry = time(NULL) + 3;
}

void log_event(const char *level, const char *fmt, ...) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%b %d %Y %H:%M:%S", tm_info);

    char hostname[64] = "router";
    gethostname(hostname, sizeof(hostname));

    char msg[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    FILE *f = fopen("/jffs/addons/zeroscale.d/zeroscale.log", "a");
    if (!f) return;

    fprintf(f, "%s %s ZEROSCALE[%d] - %s: %s\n", time_buf, hostname, (int)getpid(), level, msg);
    fclose(f);
}

static void get_router_lan_subnet(char *dest, size_t maxlen) {
    FILE *f = popen("nvram get lan_ipaddr 2>/dev/null", "r");
    if (f) {
        char ip[32] = {0};
        if (fgets(ip, sizeof(ip), f)) {
            ip[strcspn(ip, "\r\n")] = 0;
            char *last_dot = strrchr(ip, '.');
            if (last_dot && last_dot != ip) {
                *last_dot = '\0';
                snprintf(dest, maxlen, "%s.0/24", ip);
                pclose(f);
                return;
            }
        }
        pclose(f);
    }
    snprintf(dest, maxlen, "192.168.50.0/24");
}

void load_config(void) {
    AppConfig *cfg = &g_app.config;
    snprintf(cfg->version, sizeof(cfg->version), "0.2.4");
    snprintf(cfg->opmode, sizeof(cfg->opmode), "Userspace");
    snprintf(cfg->customparams, sizeof(cfg->customparams), "--accept-routes --advertise-exit-node");
    cfg->timerloop = 60;
    cfg->keepalive = 1;
    cfg->persistentsettings = 0;
    cfg->exitnode = 1;
    cfg->advroutes = 1;
    cfg->autostart = 1;
    cfg->logsize = 2000;
    cfg->amtmemailsuccess = 0;
    cfg->amtmemailfailure = 1;
    cfg->ratelimit = 5;
    cfg->schedule = 1;
    cfg->schedulehrs = 1;
    cfg->schedulemin = 0;
    cfg->track = 0;
    get_router_lan_subnet(cfg->routes, sizeof(cfg->routes));

    FILE *f = fopen("/jffs/addons/zeroscale.d/zeroscale.cfg", "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        // Trim leading whitespace and quotes
        while (*val == ' ' || *val == '\t' || *val == '"' || *val == '\'') val++;
        // Trim trailing whitespace, newline, and quotes
        size_t len = strlen(val);
        while (len > 0 && (val[len - 1] == '\r' || val[len - 1] == '\n' || val[len - 1] == ' ' || val[len - 1] == '\t' || val[len - 1] == '"' || val[len - 1] == '\'')) {
            val[--len] = '\0';
        }

        if (strcmp(key, "timerloop") == 0) cfg->timerloop = atoi(val);
        else if (strcmp(key, "keepalive") == 0) cfg->keepalive = atoi(val);
        else if (strcmp(key, "persistentsettings") == 0) cfg->persistentsettings = atoi(val);
        else if (strcmp(key, "exitnode") == 0) cfg->exitnode = atoi(val);
        else if (strcmp(key, "advroutes") == 0) cfg->advroutes = atoi(val);
        else if (strcmp(key, "autostart") == 0) cfg->autostart = atoi(val);
        else if (strcmp(key, "logsize") == 0) cfg->logsize = atoi(val);
        else if (strcmp(key, "amtmemailsuccess") == 0) cfg->amtmemailsuccess = atoi(val);
        else if (strcmp(key, "amtmemailfailure") == 0) cfg->amtmemailfailure = atoi(val);
        else if (strcmp(key, "ratelimit") == 0) cfg->ratelimit = atoi(val);
        else if (strcmp(key, "schedule") == 0) cfg->schedule = atoi(val);
        else if (strcmp(key, "schedulehrs") == 0) cfg->schedulehrs = atoi(val);
        else if (strcmp(key, "schedulemin") == 0) cfg->schedulemin = atoi(val);
        else if (strcmp(key, "track") == 0) cfg->track = atoi(val);
        else if (strcmp(key, "routes") == 0 && strlen(val) > 0) snprintf(cfg->routes, sizeof(cfg->routes), "%s", val);
        else if (strcmp(key, "tsoperatingmode") == 0 && strlen(val) > 0) snprintf(cfg->opmode, sizeof(cfg->opmode), "%s", val);
        else if ((strcmp(key, "customparams") == 0 || strcmp(key, "customflags") == 0) && strlen(val) > 0) snprintf(cfg->customparams, sizeof(cfg->customparams), "%s", val);
    }
    fclose(f);

    if (strlen(cfg->routes) == 0) {
        get_router_lan_subnet(cfg->routes, sizeof(cfg->routes));
    }
    if (strlen(cfg->opmode) == 0) {
        snprintf(cfg->opmode, sizeof(cfg->opmode), "Userspace");
    }
}

void save_config(void) {
    AppConfig *cfg = &g_app.config;
    FILE *f = fopen("/jffs/addons/zeroscale.d/zeroscale.cfg", "w");
    if (!f) return;

    fprintf(f, "track=%d\n", cfg->track);
    fprintf(f, "keepalive=%d\n", cfg->keepalive);
    fprintf(f, "timerloop=%d\n", cfg->timerloop);
    fprintf(f, "logsize=%d\n", cfg->logsize);
    fprintf(f, "autostart=%d\n", cfg->autostart);
    fprintf(f, "schedule=%d\n", cfg->schedule);
    fprintf(f, "schedulehrs=%d\n", cfg->schedulehrs);
    fprintf(f, "schedulemin=%d\n", cfg->schedulemin);
    fprintf(f, "updatetm=0\n");
    fprintf(f, "updatets=0\n");
    fprintf(f, "amtmemailsuccess=%d\n", cfg->amtmemailsuccess);
    fprintf(f, "amtmemailfailure=%d\n", cfg->amtmemailfailure);
    fprintf(f, "ratelimit=%d\n", cfg->ratelimit);
    fprintf(f, "tsoperatingmode=\"%s\"\n", cfg->opmode);
    fprintf(f, "customparams=\"%s\"\n", cfg->customparams);
    fprintf(f, "persistentsettings=%d\n", cfg->persistentsettings);
    fprintf(f, "exitnode=%d\n", cfg->exitnode);
    fprintf(f, "advroutes=%d\n", cfg->advroutes);
    fprintf(f, "routes=\"%s\"\n", cfg->routes);

    fclose(f);
    log_event("INFO", "ZeroScale config has been updated.");
}

void toggle_keepalive(void) {
    g_app.config.keepalive = !g_app.config.keepalive;
    save_config();
    log_event("INFO", "Watchdog keepalive %s.", g_app.config.keepalive ? "enabled" : "disabled");
    show_toast("Watchdog Keepalive: %s", g_app.config.keepalive ? "Enabled" : "Disabled");
}

void toggle_persistentsettings(void) {
    g_app.config.persistentsettings = !g_app.config.persistentsettings;
    save_config();
    log_event("INFO", "Persistent settings %s.", g_app.config.persistentsettings ? "enabled" : "disabled");
    show_toast("Persistent Settings: %s", g_app.config.persistentsettings ? "Enabled" : "Disabled");
}

void toggle_autostart(void) {
    g_app.config.autostart = !g_app.config.autostart;
    if (g_app.config.autostart) {
        system("if [ -f /jffs/scripts/post-mount ]; then "
               "  if ! grep -q 'S06tailscaled start' /jffs/scripts/post-mount; then "
               "    echo '(sleep 30 && /opt/etc/init.d/S06tailscaled start) & # Added by ZeroScale' >> /jffs/scripts/post-mount; "
               "  fi; "
               "else "
               "  echo '#!/bin/sh' > /jffs/scripts/post-mount; "
               "  echo '(sleep 30 && /opt/etc/init.d/S06tailscaled start) & # Added by ZeroScale' >> /jffs/scripts/post-mount; "
               "  chmod 755 /jffs/scripts/post-mount; "
               "fi");
    } else {
        system("sed -i -e '/zeroscale/d' -e '/S06tailscaled/d' /jffs/scripts/post-mount 2>/dev/null");
    }
    save_config();
    log_event("INFO", "Autostart on boot %s.", g_app.config.autostart ? "enabled" : "disabled");
    show_toast("Autostart on Boot: %s", g_app.config.autostart ? "Enabled" : "Disabled");
}

void toggle_exitnode(void) {
    g_app.config.exitnode = !g_app.config.exitnode;
    save_config();
    log_event("INFO", "%s.", g_app.config.exitnode ? "Device configured as Exit Node" : "Exit Node configuration disabled");
    show_toast("Advertise as Exit Node: %s", g_app.config.exitnode ? "Enabled" : "Disabled");
}

void toggle_advroutes(void) {
    g_app.config.advroutes = !g_app.config.advroutes;
    save_config();
    log_event("INFO", "Subnet Routes advertisement %s.", g_app.config.advroutes ? "enabled" : "disabled");
    show_toast("Advertise Subnet Routes: %s (%s)", g_app.config.advroutes ? "Enabled" : "Disabled", g_app.config.routes);
}

void cycle_timerloop(void) {
    int cur = g_app.config.timerloop;
    if (cur <= 10) g_app.config.timerloop = 30;
    else if (cur <= 30) g_app.config.timerloop = 60;
    else if (cur <= 60) g_app.config.timerloop = 120;
    else if (cur <= 120) g_app.config.timerloop = 300;
    else g_app.config.timerloop = 10;

    g_app.countdown = g_app.config.timerloop;
    save_config();
    log_event("INFO", "Status check interval set to %ds.", g_app.config.timerloop);
    show_toast("Status Check Interval: %ds", g_app.config.timerloop);
}

void cycle_amtm_email(void) {
    int s = g_app.config.amtmemailsuccess;
    int f = g_app.config.amtmemailfailure;

    if (!s && !f) {
        g_app.config.amtmemailsuccess = 0;
        g_app.config.amtmemailfailure = 1;
        show_toast("Email Alerts: Failures only");
    } else if (!s && f) {
        g_app.config.amtmemailsuccess = 1;
        g_app.config.amtmemailfailure = 0;
        show_toast("Email Alerts: Success only");
    } else if (s && !f) {
        g_app.config.amtmemailsuccess = 1;
        g_app.config.amtmemailfailure = 1;
        show_toast("Email Alerts: Success & Failures");
    } else {
        g_app.config.amtmemailsuccess = 0;
        g_app.config.amtmemailfailure = 0;
        show_toast("Email Alerts: Disabled");
    }
    save_config();
    log_event("INFO", "AMTM Email alert configuration updated.");
}

void cycle_schedule(void) {
    g_app.config.schedule = !g_app.config.schedule;
    save_config();
    log_event("INFO", "Autoupdate schedule %s.", g_app.config.schedule ? "enabled @ 01:00" : "disabled");
    show_toast("Autoupdate Schedule: %s", g_app.config.schedule ? "Enabled @ 01:00" : "Disabled");
}

void cycle_opmode(void) {
    if (strcasecmp(g_app.config.opmode, "Userspace") == 0) {
        snprintf(g_app.config.opmode, sizeof(g_app.config.opmode), "Kernel");
        show_toast("Operating Mode: Kernel (TUN)");
        save_config();
        log_event("INFO", "Operating mode set to Kernel.");
    } else if (strcasecmp(g_app.config.opmode, "Kernel") == 0) {
        snprintf(g_app.config.opmode, sizeof(g_app.config.opmode), "Custom");
        save_config();
        log_event("INFO", "Operating mode set to Custom.");
        request_input(INPUT_CUSTOMPARAMS, "Custom Tailscale Flags", "Enter custom tailscale up flags (e.g. --accept-routes)", g_app.config.customparams);
    } else {
        snprintf(g_app.config.opmode, sizeof(g_app.config.opmode), "Userspace");
        show_toast("Operating Mode: Userspace");
        save_config();
        log_event("INFO", "Operating mode set to Userspace.");
    }
}

void switch_track(void) {
    g_app.config.track = !g_app.config.track;
    save_config();
    log_event("INFO", "Release track switched to: %s.", g_app.config.track ? "Beta" : "Stable");
    show_toast("Track switched to: %s", g_app.config.track ? "Beta" : "Stable");
}

void refresh_tailscale_status(void) {
    AppConfig *cfg = &g_app.config;

    // Check daemon
    int res = system("/opt/etc/init.d/S06tailscaled check >/dev/null 2>&1");
    cfg->daemon_running = (res == 0);

    // Check tailscale version
    FILE *f = popen("tailscale version 2>/dev/null | awk 'NR==1 {print $1}'", "r");
    if (f) {
        if (fgets(cfg->tsver, sizeof(cfg->tsver), f)) {
            cfg->tsver[strcspn(cfg->tsver, "\r\n")] = 0;
        }
        pclose(f);
    }
    if (strlen(cfg->tsver) == 0) snprintf(cfg->tsver, sizeof(cfg->tsver), "1.102.2");

    // Fetch self IP
    char self_ip[40] = {0};
    FILE *sif = popen("tailscale ip -4 2>/dev/null", "r");
    if (sif) {
        if (fgets(self_ip, sizeof(self_ip), sif)) {
            self_ip[strcspn(self_ip, "\r\n")] = 0;
        }
        pclose(sif);
    }

    // Refresh peer table
    g_app.peer_count = 0;
    FILE *pf = popen("tailscale status 2>/dev/null", "r");
    if (pf) {
        char line[512];
        cfg->tailnet_connected = 0;
        while (fgets(line, sizeof(line), pf) && g_app.peer_count < MAX_PEERS) {
            cfg->tailnet_connected = 1;
            PeerInfo *p = &g_app.peers[g_app.peer_count];
            memset(p, 0, sizeof(*p));
            
            char ip[40], name[48], user[48], os[24], status[128];
            int n = sscanf(line, "%39s %47s %47s %23s %127[^\n]", ip, name, user, os, status);
            if (n >= 4) {
                snprintf(p->ip, sizeof(p->ip), "%s", ip);
                snprintf(p->name, sizeof(p->name), "%s", name);
                snprintf(p->user, sizeof(p->user), "%s", user);
                snprintf(p->os, sizeof(p->os), "%s", os);
                if (n >= 5) snprintf(p->status, sizeof(p->status), "%s", status);
                else snprintf(p->status, sizeof(p->status), "-");

                p->is_self = (g_app.peer_count == 0 || (strlen(self_ip) > 0 && strcmp(p->ip, self_ip) == 0));
                p->is_exit = (strstr(p->status, "offers exit node") != NULL || strstr(p->status, "exit node") != NULL);
                p->is_online = (strstr(p->status, "offline") == NULL);
                p->is_idle = (strstr(p->status, "idle") != NULL);
                p->is_active = (strstr(p->status, "active") != NULL && !p->is_self);
                p->is_direct = (strstr(p->status, "direct") != NULL);
                
                if (p->is_self) {
                    snprintf(p->relay_info, sizeof(p->relay_info), "Local Router (Self)");
                } else if (strstr(p->status, "relay") != NULL) {
                    snprintf(p->relay_info, sizeof(p->relay_info), "DERP Relay");
                } else if (p->is_direct) {
                    snprintf(p->relay_info, sizeof(p->relay_info), "Direct Connection");
                } else {
                    snprintf(p->relay_info, sizeof(p->relay_info), "-");
                }

                g_app.peer_count++;
            }
        }
        pclose(pf);
    }
}

void load_logs(void) {
    g_app.log_count = 0;
    FILE *f = fopen("/jffs/addons/zeroscale.d/zeroscale.log", "r");
    if (!f) {
        snprintf(g_app.log_lines[0], LOG_LINE_LEN, "Log file not found (/jffs/addons/zeroscale.d/zeroscale.log).");
        g_app.log_count = 1;
        g_app.log_scroll = 0;
        return;
    }

    char line[LOG_LINE_LEN];
    while (fgets(line, sizeof(line), f) && g_app.log_count < MAX_LOG_LINES) {
        line[strcspn(line, "\r\n")] = 0;
        snprintf(g_app.log_lines[g_app.log_count], LOG_LINE_LEN, "%s", line);
        g_app.log_count++;
    }
    fclose(f);

    int max_visible = tb_height() - 6;
    if (max_visible < 1) max_visible = 20;

    if (g_app.log_count > max_visible) {
        g_app.log_scroll = g_app.log_count - max_visible;
    } else {
        g_app.log_scroll = 0;
    }
}
