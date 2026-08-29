#!/bin/sh

# ZeroScale (zeroscale.sh) is an all-in-one script that is optimized to install, maintain and monitor a Tailscale service and
# connection from your Asus-Merlin FW router in swapless environments.
# Based on TAILMON by Viktor Jaep, licensed under GPLv3.
# Last Updated: 2026-Aug-28

#Preferred standard router binaries path
export PATH="/sbin:/bin:/usr/sbin:/usr/bin:$PATH"
export GOMAXPROCS=1
unset LD_LIBRARY_PATH

##-------------------------------------##
## Added by Martinski W. [2026-Apr-13] ##
##-------------------------------------##
[ "$HOME" != "/root" ] && export HOME="/root"
export SCREENDIR="${HOME}/.screen"

#Static Variables - please do not change
version="1.1.0"
beta=0                                                               # Beta indicator on/off
track=0                                                              # Stable (0) / Beta (1) Track subscription
apppath="/jffs/scripts/zeroscale.sh"                                   # Static path to the app
config="/jffs/addons/zeroscale.d/zeroscale.cfg"                          # Static path to the config file
dlverpath="/jffs/addons/zeroscale.d/version.txt"                       # Static path to the version file
bverpath="/jffs/addons/zeroscale.d/beta.txt"                           # Static path to the beta version file
logfile="/jffs/addons/zeroscale.d/zeroscale.log"                         # Static path to the log
tmemails="/jffs/addons/zeroscale.d/tmemails.txt"                       # Static path to email rate limit file
routerboot=0                                                         # Tracking router reboot notifications
tsinstalled=0
keepalive=0
timerloop=60
logsize=2000
autostart=0
schedule=0                                                           # Scheduler enable y/n
schedulehrs=1                                                        # Scheduler hours
schedulemin=0                                                        # Scheduler mins
updatetm=0                                                           # Autoupdate ZeroScale Script
updatets=0                                                           # Autoupdate Tailscale Binaries
amtmemailsuccess=0
amtmemailfailure=0
ratelimit=0                                                          # Rate limiting number of emails/houre
exitnode=0
advroutes=1
accroutes=0
sshenable=0
persistentsettings=0
tsoperatingmode="Userspace"
precmd=""
args="--tun=userspace-networking --state=/opt/var/tailscaled.state --statedir=/opt/var/lib/tailscale"
preargs="nohup"
routes="$(nvram get lan_ipaddr | cut -d"." -f1-3).0/24"
customcmdline=""
progresspromptactive=0                                             # Tracks the monitoring prompt line
laststatustext=""                                                  # Last status text drawn on the prompt line
lastinputtext=""                                                   # Last input hint text drawn on the prompt line

#AMTM Email Notification Variables
readonly scriptFileName="${0##*/}"
readonly scriptFileNTag="${scriptFileName%.*}"
readonly CEM_LIB_TAG="master"
readonly CEM_LIB_URL="https://raw.githubusercontent.com/Martinski4GitHub/CustomMiscUtils/${CEM_LIB_TAG}/EMail"
readonly CUSTOM_EMAIL_LIBDir="/jffs/addons/shared-libs"
readonly CUSTOM_EMAIL_LIBName="CustomEMailFunctions.lib.sh"
readonly CUSTOM_EMAIL_LIBFile="${CUSTOM_EMAIL_LIBDir}/$CUSTOM_EMAIL_LIBName"

# Color variables
CRed="\e[1;31m"
InvRed="\e[1;41m"
CGreen="\e[1;32m"
InvGreen="\e[1;42m"
CDkGray="\e[1;90m"
InvDkGray="\e[1;100m"
CYellow="\e[1;33m"
InvYellow="\e[1;43m"
CCyan="\e[1;36m"
CWhite="\e[1;37m"
CClear="\e[0m"

# To support automatic script updates from AMTM #
doScriptUpdateFromAMTM=true

# -------------------------------------------------------------------------------------------------------------------------
# FUNCTIONS BEGIN
# -------------------------------------------------------------------------------------------------------------------------


# -------------------------------------------------------------------------------------------------------------------------
# Global Trap Cleanup
# -------------------------------------------------------------------------------------------------------------------------
cleanup() {
  local exit_code=$?
  rm -f /opt/tmp/tailscaled 2>/dev/null
  rm -f /jffs/scripts/zeroscale.sh.tmp 2>/dev/null
  # Exit cleanly with the original exit code
  exit "$exit_code"
}
trap cleanup EXIT INT TERM


# Progressbar variables (optimized)
barlen=46
barspaces="                                              "
