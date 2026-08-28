# -------------------------------------------------------------------------------------------------------------------------
# LogoNM is a function that displays the BACKUPMON script name in a cool ASCII font without menu options

logoNM ()
{
  clear
  printf "%b" "${CDkGray}"
  cat <<EOF



                      _________    ______    __  _______  _   __
                     /_  __/   |  /  _/ /   /  |/  / __ \/ | / /
                      / / / /| |  / // /   / /|_/ / / / /  |/ /
                     / / / ___ |_/ // /___/ /  / / /_/ / /|  /
                    /_/ /_/  |_/___/_____/_/  /_/\____/_/ |_/ v$version
                                      [  Z  E  R  0  ]


EOF
  printf "%b" "${CClear}"
  printf "%b" "\r                            ${CGreen}    [ INITIALIZING ]     ${CClear}"
  sleep 1
  clear
  printf "%b" "${CYellow}"
  cat <<EOF



                      _________    ______    __  _______  _   __
                     /_  __/   |  /  _/ /   /  |/  / __ \/ | / /
                      / / / /| |  / // /   / /|_/ / / / /  |/ /
                     / / / ___ |_/ // /___/ /  / / /_/ / /|  /
                    /_/ /_/  |_/___/_____/_/  /_/\____/_/ |_/ v$version
                                      [  Z  E  R  0  ]


EOF
  printf "%b" "${CClear}"
  printf "%b" "\r                            ${CGreen}[ INITIALIZING ... DONE ]${CClear}"
  sleep 1
  printf "%b" "\r                            ${CGreen}      [ LOADING... ]     ${CClear}"
  sleep 1
}

logoNMexit ()
{
  clear
  printf "%b" "${CYellow}"
  cat <<EOF



                      _________    ______    __  _______  _   __
                     /_  __/   |  /  _/ /   /  |/  / __ \/ | / /
                      / / / /| |  / // /   / /|_/ / / / /  |/ /
                     / / / ___ |_/ // /___/ /  / / /_/ / /|  /
                    /_/ /_/  |_/___/_____/_/  /_/\____/_/ |_/ v$version
                                      [  Z  E  R  0  ]


EOF
  printf "%b" "${CClear}"
  printf "%b" "\r                            ${CGreen}    [ SHUTTING DOWN ]     ${CClear}"
  sleep 1
  clear
  printf "%b" "${CDkGray}"
  cat <<EOF



                      _________    ______    __  _______  _   __
                     /_  __/   |  /  _/ /   /  |/  / __ \/ | / /
                      / / / /| |  / // /   / /|_/ / / / /  |/ /
                     / / / ___ |_/ // /___/ /  / / /_/ / /|  /
                    /_/ /_/  |_/___/_____/_/  /_/\____/_/ |_/ v$version
                                      [  Z  E  R  0  ]


EOF
  printf "%b" "${CClear}"
  printf "%b" "\r                            ${CGreen}    [ SHUTTING DOWN ]     ${CClear}"
  sleep 1
  printf "%b" "\r                            ${CDkGray}      [ GOODBYE... ]     ${CClear}\n\n"
  sleep 1
}

# -------------------------------------------------------------------------------------------------------------------------
# Promptyn is a simple function that accepts y/n input

promptyn()
{   # No defaults, just y or n
  while true; do
    read -p "$1" -n 1 -r yn
      case "${yn}" in
        [Yy]* ) return 0 ;;
        [Nn]* ) return 1 ;;
        * ) echo -e "\nPlease answer y or n.";;
      esac
  done
}

# -------------------------------------------------------------------------------------------------------------------------
# Spinner is a script that provides a small indicator on the screen to show script activity

spinner()
{
  local spins=$1

  spin=0
  totalspins=$((spins / 4))
  while [ $spin -le $totalspins ]; do
    for spinchar in / - \\ \|; do
      printf "%b" "\r$spinchar"
      sleep 1
    done
    spin=$((spin+1))
  done

  printf "\r"
}

##-------------------------------------------##
## Borrwed from ExtremeFiretop [2026-Apr-11] ##
##-------------------------------------------##
ScriptUpdateFromAMTM()
{
    if ! "$doScriptUpdateFromAMTM"
    then
        printf "Automatic script updates via AMTM are currently disabled.\n\n"
        return 1
    fi

    if [ $# -gt 0 ] && [ "$1" = "check" ]
    then return 0
    fi

    # Force a ZeroScale download and update
    echo ""
    echo -e "${InvGreen} ${CClear} Downloading latest ${CGreen}ZeroScale${CClear}... Please stand by while we add even more Tailscale goodness..."
    curl --silent --retry 3 "https://raw.githubusercontent.com/underd0se/ZeroScale/main/zeroscale.sh" -o "/jffs/scripts/zeroscale.sh" && chmod 755 "/jffs/scripts/zeroscale.sh"
    DLsuccess=$?
    if [ "$DLsuccess" -eq 0 ]; then
      echo -e "${InvGreen} ${CClear} ZeroScale Download/Update Success."
      echo ""
    else
      echo -e "${InvRed} ${CClear} ZeroScale Download/Update Failed. Please check all the things."
      echo ""
    fi

    return "$DLsuccess"
}

# -------------------------------------------------------------------------------------------------------------------------
# Preparebar and Progressbar is a script that provides a nice progressbar to show script activity

preparebar()
{
  barlen=$1
  barspaces=$(printf "%*s" "$1")
  barchars=$(printf "%*s" "$1" | tr ' ' "$2")
}

# Read exactly one visible menu command followed by Enter.
# Normal terminal echo and line editing remain available; pasted multi-character input is ignored.
readmenucommand()
{
  key_press=""
  menu_line_submitted=0
  if [ -z "$ttydev" ]; then ttydev="$(tty 2>/dev/null)"; fi

  if [ -z "$ttydev" ] || [ "$ttydev" = "not a tty" ]; then
    return 1
  fi

  if IFS= read -r -t 1 key_press < "$ttydev"; then
    menu_line_submitted=1
    [ "${#key_press}" -eq 1 ]
    return $?
  fi

  key_press=""
  return 1
}

# Keep the status and the editable command prompt on the same line. The status text is a
# fixed-width field (callers zero-pad their numbers to a constant digit width), so timer
# refreshes can redraw just that field in place at the start of the line without ever
# reaching into the input area that follows, then restore the cursor, preserving terminal
# echo, cursor position and backspace editing for whatever the user has typed there.
drawprogressprompt()
{
  local status_text="$1"
  local input_text="$2"

  laststatustext="$status_text"
  lastinputtext="$input_text"

  if [ "$progresspromptactive" -ne 1 ]; then
    printf "\033[2K\r%b %s\033[2D" "$status_text" "$input_text"
    progresspromptactive=1
  else
    # Save the current input cursor, redraw the fixed-width status field, then restore it.
    printf "\033[s\r%b\033[u" "$status_text"
  fi
}

resetinvalidprogressinput()
{
  # A complete line was submitted, but it was not exactly one command character.
  # read(1) has moved to the following line, so move back and restore a clean prompt.
  printf "\033[1A\33[2K\r%b %s\033[2D" "$laststatustext" "$lastinputtext"
}

# Discard characters typed or pasted while an interactive command owned the terminal.
# This prevents an authentication URL, including an incomplete line with no Enter, from
# being interpreted later as part of a setup-menu selection.
drainpendingttyinput()
{
  local ttydev discarded_input

  if [ -z "$ttydev" ]; then ttydev="$(tty 2>/dev/null)"; fi
  if [ -z "$ttydev" ] || [ "$ttydev" = "not a tty" ]; then
    return 0
  fi

  # Read one character at a time so this also clears a partial pasted line that
  # has not yet received Enter. The final one-second timeout confirms the queue
  # is empty. BusyBox ash already supports this -n1 form elsewhere in ZeroScale.
  while IFS= read -r -n 1 -t 1 discarded_input < "$ttydev"; do
    :
  done
}

# Monitoring Mode requires the complete Entware Tailscale installation, not just
# a saved ZeroScale configuration. Keep this check centralized so direct, SCREEN,
# and Setup-menu launch paths all enforce the same requirement.
tailscaleready()
{
  [ -x "/opt/bin/tailscale" ] &&
  [ -x "/opt/bin/tailscaled" ] &&
  [ -f "/opt/etc/init.d/S06tailscaled" ]
}

monitoringblocked()
{
  echo ""
  echo -e "${CRed}ZeroScale Monitoring Mode is unavailable because Tailscale is not fully installed.${CClear}"
  echo -e "Install Tailscale using option 1 before launching Monitoring Mode."
  echo ""

  if [ "$1" = "pause" ]; then
    read -rsp $'Press any key to continue...\n' -n1 key
  fi
}

progressbaroverride()
{
  insertspc=" "
  bypasswancheck=0

  [ "$1" -eq 1 ] && progresspromptactive=0

  if [ "$1" -eq -1 ]; then
    printf "%b" "\r  $barspaces\r"
  else
    if [ ! -z "$7" ] && [ "$1" -ge "$7" ]; then
      local barch=$(($7*barlen/$2))
      local barsp=$((barlen-barch))
      local progr=$((100*$1/$2))
    else
      local barch=$(($1*barlen/$2))
      local barsp=$((barlen-barch))
      local progr=$((100*$1/$2))
    fi

    if [ ! -z "$6" ]; then AltNum=$6; else AltNum=$1; fi

    if [ "$5" == "Standard" ]; then
      # Zero-pad the timer and percent to a fixed digit width (based on the configured
      # timerloop) so the status field is always the same length across redraws. This
      # keeps the input area that follows it from being disturbed when the timer refreshes.
      tlwidth=${#2}
      AltNumPadded=$(printf "%0${tlwidth}d" "$AltNum")
      progrPadded=$(printf "%03d" "$progr")
      drawprogressprompt "${InvGreen} ${CClear} ${CWhite}${InvDkGray}${AltNumPadded}${4} / ${progrPadded}%${CClear} [${CGreen}e${CClear}=Exit]${CClear}" "[Key+Enter?  ]"
    fi
  fi

  # Require one command character followed by Enter. Pasted text is ignored.
  if readmenucommand; then
      progresspromptactive=0
      echo ""
      case $key_press in
          [Aa]) vconfig;;
          [Cc]) vsetup;;
          [Dd]) tsdown;;
          [Ee]) logoNMexit; echo -e "${CClear}\n"; exit 0;;
          [Kk]) vconfig;;
          [Ll]) vlogs;;
          [Mm]) timerloopconfig;;
          [Oo]) if [ "$tsoperatingmode" == "Custom" ]; then customconfig; fi;;
          [Rr]) restarttsc;;
          [Ss]) startts;;
          [Tt]) stopts;;
          [Uu]) tsup;;
          *) timer=$timerloop;;
      esac
  elif [ "$menu_line_submitted" -eq 1 ]; then
      resetinvalidprogressinput
  fi
}

progressbarpause()
{
  insertspc=" "
  bypasswancheck=0

  [ "$1" -eq 1 ] && progresspromptactive=0

  if [ "$1" -eq -1 ]
  then
     printf "%b" "\r  $barspaces\r"
  else
    if [ $# -gt 6 ] && [ -n "$7" ] && [ "$1" -ge "$7" ]
    then
       local barch="$(($7*barlen/$2))"
       local barsp="$((barlen-barch))"
       local progr="$((100*$1/$2))"
    else
       local barch="$(($1*barlen/$2))"
       local barsp="$((barlen-barch))"
       local progr="$((100*$1/$2))"
    fi

    if [ $# -gt 5 ] && [ -n "$6" ]; then AltNum="$6" ; else AltNum="$1" ; fi

    if [ "$5" = "Standard" ]
    then
       drawprogressprompt "${InvGreen} ${CClear} ${CWhite}${InvDkGray}Continuing in $AltNum/5...${CClear} [${CGreen}s${CClear}=Setup] [${CGreen}e${CClear}=Exit]${CClear}" "[Key+Enter?  ]"
    fi
  fi

  # Require one command character followed by Enter. Pasted text is ignored.
  if readmenucommand
  then
      progresspromptactive=0
      echo ""
      case $key_press in
          [Ss]) vsetup;;
          [Ee]) logoNMexit; echo -e "${CClear}\n"; exit 0;;
      esac
  elif [ "$menu_line_submitted" -eq 1 ]; then
      resetinvalidprogressinput
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# Initial setup menu

legacy_cleanup() {
    clear
    echo -e "${CRed}WARNING: Legacy Installation Detected!${CClear}"
    echo -e "ZeroScale cannot run alongside legacy TAILMON or TAILMON ZER0."
    echo -e "Continuing will automatically remove legacy files and configurations."
    echo ""
    if promptyn "Would you like to remove legacy installations and continue setup? [y/n]: "; then
        echo -e "\n${CGreen}Cleaning up legacy installations...${CClear}"
        rm -f /jffs/scripts/tailmon.sh.tmp 2>/dev/null
        rm -f /jffs/scripts/tailmon-zero.sh.tmp 2>/dev/null
        cru d tailmon >/dev/null 2>&1
        cru d RunTAILMONcheck >/dev/null 2>&1
        cru d RunTAILMONZER0check >/dev/null 2>&1
        rm -f -r /jffs/addons/tailmon.d >/dev/null 2>&1
        rm -f -r /jffs/addons/tailmon-zero.d >/dev/null 2>&1
        rm -f /jffs/scripts/tailmon.sh >/dev/null 2>&1
        rm -f /jffs/scripts/tailmon-zero.sh >/dev/null 2>&1
        rm -f /opt/bin/tailmon-zer0 2>/dev/null
        sed -i -e '/tailmon\.sh/d' /jffs/scripts/post-mount >/dev/null 2>&1
        sed -i -e '/tailmon-zero\.sh/d' /jffs/scripts/post-mount >/dev/null 2>&1
        sed -i -e '/tailmon\.sh/d' /jffs/configs/profile.add >/dev/null 2>&1
        sed -i -e '/tailmon-zero\.sh/d' /jffs/configs/profile.add >/dev/null 2>&1
        sed -i -e '/tailmon-zer0/d' /jffs/configs/profile.add >/dev/null 2>&1
        echo -e "${CGreen}Legacy files removed successfully.${CClear}"
        sleep 2
    else
        echo -e "\nSetup aborted. Please manually uninstall legacy installations first."
        exit 1
    fi
}

initialsetup()
{
    if [ ! -d "/jffs/addons/zeroscale.d" ]; then
        mkdir -p "/jffs/addons/zeroscale.d"
    fi

    if [ -d "/jffs/addons/tailmon.d" ] || [ -f "/jffs/scripts/tailmon.sh" ] || [ -d "/jffs/addons/tailmon-zero.d" ] || [ -f "/jffs/scripts/tailmon-zero.sh" ]; then
        legacy_cleanup
    fi
    clear
    echo -e "${InvGreen} ${InvDkGray}${CWhite} ZeroScale Initial Setup                                                                     ${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} ZeroScale has not been configured yet, and Tailscale will need to be installed and${CClear}"
    echo -e "${InvGreen} ${CClear} configured. You can choose between 'Express Install' and 'Advanced Install'.${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} 1) Express Install will automatically download and install Tailscale, choosing the${CClear}"
    echo -e "${InvGreen} ${CClear} 'Userspace' mode of operation and configures it to advertise routes of your local${CClear}"
    echo -e "${InvGreen} ${CClear} subnet by default. A URL prompt will appear which will require you to copy this link"
    echo -e "${InvGreen} ${CClear} into your browser to connect this device to your tailnet."
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} 2) Advanced Install will launch the ZeroScale Setup/Configuration Menu, and allows${CClear}"
    echo -e "${InvGreen} ${CClear} you to manually choose your preferred settings, such as 'Kernel' vs. 'Userspace'${CClear}"
    echo -e "${InvGreen} ${CClear} mode, and letting you pick the exit node option along with additional subnets."
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} Before starting, please familiarize yourself with how Tailscale works. Please use${CClear}"
    echo -e "${InvGreen} ${CClear} @ColinTaylor's Wiki available here:${CClear}"
    echo -e "${InvGreen} ${CClear} https://github.com/RMerl/asuswrt-merlin.ng/wiki/Installing-Tailscale-through-Entware${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} It is also advised to have an account set and ready to go on https://tailscale.com${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo ""
    read -p "Please select? (1=Express Install, 2=Advanced Install, e=Exit): " SelectSetup
      case $SelectSetup in
        1)
        echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale Express Install initiated." >> "$logfile"
        expressinstall
        return
        ;;

        2)
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale Advanced Install initiated." >> "$logfile"
          saveconfig
          vsetup
          return
          ;;

        [Ee]) echo -e "${CClear}"; echo ""; exit 0;;
      esac
}

