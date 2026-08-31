#ifndef APP_H
#define APP_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "../include/termbox2.h"

#define MAX_PEERS 256
#define MAX_LOG_LINES 1500
#define LOG_LINE_LEN 256

typedef enum {
    VIEW_DASHBOARD,
    VIEW_LOGS,
    VIEW_CONFIG,
    VIEW_PEER_DETAIL,
    VIEW_INPUT,
    VIEW_CONFIRM
} ViewMode;

typedef enum {
    FOCUS_NONE,
    FOCUS_HEADER_MENU,
    FOCUS_PEERS
} DashboardFocus;

typedef enum {
    INPUT_ROUTES,
    INPUT_LOGSIZE,
    INPUT_CUSTOMPARAMS
} InputTarget;

typedef struct {
    char ip[40];
    char name[48];
    char user[48];
    char os[24];
    char status[128];
    int is_self;
    int is_exit;
    int is_online;
    int is_idle;
    int is_active;
    int is_direct;
    char relay_info[64];
} PeerInfo;

typedef struct {
    char version[16];
    char tsver[16];
    char opmode[24];
    char customparams[256];
    int keepalive;
    int timerloop;
    int persistentsettings;
    int exitnode;
    int advroutes;
    char routes[128];
    int autostart;
    int logsize;
    int amtmemailsuccess;
    int amtmemailfailure;
    int ratelimit;
    int schedule;
    int schedulehrs;
    int schedulemin;
    int track; // 0=Stable, 1=Beta
    int daemon_running;
    int tailnet_connected;
} AppConfig;

typedef struct {
    ViewMode mode;
    ViewMode prev_mode;
    AppConfig config;

    // Focus & Navigation
    DashboardFocus dash_focus;
    int header_selected_idx;
    int config_selected_idx;

    // Peers Table
    PeerInfo peers[MAX_PEERS];
    int peer_count;
    int peer_scroll;
    int selected_peer;

    // Logs
    char log_lines[MAX_LOG_LINES][LOG_LINE_LEN];
    int log_count;
    int log_scroll;

    // Timer & Status
    int countdown;
    time_t last_tick;
    time_t last_status_refresh;

    // Toast
    char toast_msg[128];
    time_t toast_expiry;

    // Modals
    char confirm_prompt[128];
    char confirm_action_label[32];
    char confirm_cmd[512];
    int confirm_selected_btn;

    // Generic Input Dialog
    InputTarget input_target;
    char input_title[64];
    char input_prompt[128];
    char input_buf[128];
    int input_cursor;
    int input_selected_btn;

    // Peer Detail Modal
    int peer_detail_selected_btn;

    // Terminal & Selection
    char copy_hint[32];
    char term_name[32];

    int mock_mode;
    int running;
} AppState;

extern AppState g_app;

// Core Prototypes
void app_init(void);
void load_mock_data(void);
void detect_terminal(void);
void app_cleanup(void);
void load_config(void);
void save_config(void);
void refresh_tailscale_status(void);
void load_logs(void);
void log_event(const char *level, const char *fmt, ...);
void show_toast(const char *fmt, ...);
void show_splash(const char *status_msg, int duration_ms, uint32_t color);
void request_confirm(const char *prompt, const char *action_label, const char *cmd);
void request_input(InputTarget target, const char *title, const char *prompt, const char *initial);

void execute_action(const char *action, const char *cmd);
void toggle_keepalive(void);
void toggle_persistentsettings(void);
void toggle_autostart(void);
void toggle_exitnode(void);
void toggle_advroutes(void);
void cycle_timerloop(void);
void cycle_amtm_email(void);
void cycle_schedule(void);
void cycle_opmode(void);
void switch_track(void);

void install_zeroscale(void);
void uninstall_zeroscale(void);

// UI Rendering
void ui_draw(void);
void handle_event(struct tb_event *ev);

#endif // APP_H
