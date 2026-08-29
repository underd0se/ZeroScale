# -------------------------------------------------------------------------------------------------------------------------
# vuninstall is a function that uninstalls and removes all traces of ZeroScale/tailscale from your router...

vuninstall()
{
  while true; do
    clear
    echo -e "${InvGreen} ${InvDkGray}${CWhite} Uninstall Utility                                                                     ${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} You are about to uninstall ZeroScale and optionally, Tailscale from your router! This"
    echo -e "${InvGreen} ${CClear} action is irreversible."
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo ""
    echo -e "Do you wish to proceed?${CClear}"
    if promptyn "[y/n]: "; then
      echo ""
      echo -e "\nAre you sure? Please type 'y' to validate you wish to proceed.${CClear}"
        if promptyn "[y/n]: "; then
          #Remove and uninstall files/directories
          if [ -n "$old_overcommit" ]; then
            echo -e "\n${CGreen}Restoring vm.overcommit_memory to $old_overcommit...${CClear}"
            echo "$old_overcommit" > /proc/sys/vm/overcommit_memory 2>/dev/null
          fi
          

          
          # Clean up any residual temporary download files
          rm -f /opt/tmp/tailscaled 2>/dev/null
          rm -f /opt/tmp/tailscale 2>/dev/null
          rm -f /jffs/scripts/zeroscale.sh.tmp /jffs/scripts/tailmon-zero.sh.tmp /jffs/scripts/tailmon.sh.tmp 2>/dev/null

          rm -f -r /jffs/addons/zeroscale.d /jffs/addons/tailmon-zero.d /jffs/addons/tailmon.d >/dev/null 2>&1
          rm -f /jffs/scripts/zeroscale.sh /jffs/scripts/tailmon-zero.sh /jffs/scripts/tailmon.sh >/dev/null 2>&1
          rm -f /opt/bin/zeroscale /opt/bin/tailmon-zero /opt/bin/tailmon-zer0 >/dev/null 2>&1
          sed -i -e '/zeroscale\.sh/d' -e '/tailmon-zero\.sh/d' -e '/tailmon\.sh/d' /jffs/scripts/post-mount >/dev/null 2>&1
          sed -i -e '/zeroscale/d' -e '/tailmon-zero\.sh/d' -e '/tailmon-zer0/d' -e '/tailmon\.sh/d' /jffs/configs/profile.add >/dev/null 2>&1
          echo ""
          echo -e "\n${CGreen}ZeroScale has been uninstalled...${CClear}"
          echo ""
          if [ -f "/opt/bin/tailscale" ]; then
            echo -e "Would you also like to uninstall Tailscale from your router?"
            if promptyn "[y/n]: "; then
              if [ -d "/opt" ]; then # Does entware exist? If yes proceed, if no error out.
                echo ""
                echo -e "\n${CGreen}Shutting down Tailscale...${CClear}"
                tailscale logout
                tailscale down
                /opt/etc/init.d/S06tailscaled stop
                echo ""
                echo -e "\n${CGreen}Removing firewall-start entries...${CClear}"
                #remove firewall-start entry if found
                if [ -f /jffs/scripts/firewall-start ]; then
                  if grep -q -F "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi" /jffs/scripts/firewall-start; then
                    sed -i -e '/tailscale down/d' /jffs/scripts/firewall-start
                  fi
                fi
                echo -e "\n${CGreen}Updating Entware Packages...${CClear}"
                echo ""
                opkg update
                echo ""
                echo -e "${CGreen}Uninstalling Entware Tailscale Package(s)...${CClear}"
                echo ""

                archker=$(opkg print-architecture | grep "armv7-2.6")
                if [ -z "$archker" ]; then
                  opkg remove tailscale
                else
                  opkg remove tailscale_nohf #remove special tailscale package for arm7 kernel 2.6
                fi
                rm -f /opt/etc/init.d/S06tailscaled >/dev/null 2>&1
                rm -f /opt/var/tailscaled.state >/dev/null 2>&1
                rm -r /opt/var/lib/tailscale >/dev/null 2>&1
                rm -r /opt/var/run/tailscale >/dev/null 2>&1
                rm -r /var/run/tailscale >/dev/null 2>&1
                rm -r /var/lib/tailscale >/dev/null 2>&1

                echo ""
                read -rsp $'Press any key to continue...\n' -n1 key
                echo ""
                echo -e "${CClear}"
                exit 0
                break
              else
                clear
                echo -e "${CRed}ERROR: Entware was not found on this router...${CClear}"
                echo -e "Please install Entware using the AMTM utility before proceeding..."
                echo ""
                read -rsp $'Press any key to continue...\n' -n1 key
                exit 1
              fi
              exit 0
            else
              echo ""
              echo -e "\nWould you like to RETAIN the ZeroScale memory optimizations for Tailscale?"
              echo -e "This is highly recommended to prevent your router from crashing."
              if promptyn "[y/n]: "; then
                echo ""
                echo -e "\n${CGreen}Tailscale memory optimizations preserved.${CClear}"
              else
                echo -e "\nStripping memory optimizations..."
                if [ -f "/opt/etc/init.d/S06tailscaled" ]; then
                  sed -i '/# TAILMON Zer.*: Dynamic Swapless/d' "/opt/etc/init.d/S06tailscaled"
                  sed -i '/export GOMAXPROCS=1/d' "/opt/etc/init.d/S06tailscaled"
                  sed -i '/export GOMEMLIMIT=20MiB/d' "/opt/etc/init.d/S06tailscaled"
                  sed -i '/export GOGC=20/d' "/opt/etc/init.d/S06tailscaled"
                  sed -i '/swap_total=$(free/d' "/opt/etc/init.d/S06tailscaled"
                  sed -i '/if \[ "\$swap_total" = "0" \]; then/d' "/opt/etc/init.d/S06tailscaled"
                  sed -i '/echo 0 > \/proc\/sys\/vm\/overcommit_memory/d' "/opt/etc/init.d/S06tailscaled"
                  sed -i '/^fi$/d' "/opt/etc/init.d/S06tailscaled" 2>/dev/null || true
                  
                  # Clean new block logic
                  sed -i '/# ZeroScale: Dynamic Swapless Block Start/,/# ZeroScale: Dynamic Swapless Block End/d' "/opt/etc/init.d/S06tailscaled"
                fi
              fi
              echo ""
              echo -e "\nExiting Uninstall Utility...${CClear}"
              sleep 1
              echo ""
              echo -e "${CClear}"
              exit 0
            fi
          fi
          exit 0
        else
          echo ""
          echo -e "\nExiting Uninstall Utility...${CClear}"
          sleep 1
          return
        fi
    fi
  done
}

# -------------------------------------------------------------------------------------------------------------------------
# vlogs is a function that calls the nano text editor to view the ZeroScale log file

vlogs()
{
  export TERM=linux
  nano +999999 --linenumbers "$logfile"
  timer=$timerloop
  trimlogs
}

# -------------------------------------------------------------------------------------------------------------------------
# trimlogs will cut down log size (in rows) based on custom value

trimlogs()
{
  if [ "$logsize" -gt 0 ]
  then
      currlogsize="$(wc -l < "$logfile")" # Determine the number of rows in the log

      if [ "$currlogsize" -gt "$logsize" ] # If it's bigger than the max allowed, tail/trim it!
      then
          tail -"$logsize" "$logfile" > "${logfile}.tmp"
          mv "${logfile}.tmp" "$logfile"
          echo "$(date +'%b %d %Y %X') $(_GetLAN_HostName_) ZEROSCALE[$$] - INFO: Trimmed the log file down to $logsize lines" >> "$logfile"
      fi
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# saveconfig saves the zeroscale.cfg file after every major change, and applies that to the script on the fly

saveconfig()
{
   { echo 'track='"$track"
     echo 'keepalive='"$keepalive"
     echo 'timerloop='"$timerloop"
     echo 'logsize='"$logsize"
     echo 'autostart='"$autostart"
     echo 'schedule='"$schedule"
     echo 'schedulehrs='"$schedulehrs"
     echo 'schedulemin='"$schedulemin"
     echo 'updatetm='"$updatetm"
     echo 'updatets='"$updatets"
     echo 'amtmemailsuccess='"$amtmemailsuccess"
     echo 'amtmemailfailure='"$amtmemailfailure"
     echo 'ratelimit='"$ratelimit"
     echo 'tsoperatingmode="'"$tsoperatingmode"'"'
     echo 'persistentsettings='"$persistentsettings"
     echo 'exitnode='"$exitnode"
     echo 'advroutes='"$advroutes"
     echo 'accroutes='"$accroutes"
     echo 'sshenable='"$sshenable"
     echo 'precmd="'"$precmd"'"'
     echo 'args="'"$args"'"'
     echo 'preargs="'"$preargs"'"'
     echo 'routes="'"$routes"'"'
     echo 'customcmdline="'"$customcmdline"'"'
     echo 'old_overcommit="'"$old_overcommit"'"'
   } > "$config"
   echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale config has been updated." >> "$logfile"

   if [ -f "$config" ]; then
     source "$config"
   fi
}

# -------------------------------------------------------------------------------------------------------------------------
# Begin main commandline switch logic
# -------------------------------------------------------------------------------------------------------------------------

# Remove Maintenance Mode file lock
rm -f /jffs/addons/zeroscale.d/updating.txt >/dev/null 2>&1

# Check for updates
updatecheck
betacheck

if [ "$1" = "amtmupdate" ]
then
    shift
    ScriptUpdateFromAMTM "$@"
    exit "$?"
fi

# Check for and add an alias for ZeroScale
if ! grep -F "sh /jffs/scripts/zeroscale.sh" /jffs/configs/profile.add >/dev/null 2>/dev/null; then
  # Clean up legacy aliases
  sed -i -e '/tailmon\.sh/d' -e '/tailmon-zero\.sh/d' -e '/tailmon-zer0/d' /jffs/configs/profile.add >/dev/null 2>&1
  echo "alias zeroscale=\"sh /jffs/scripts/zeroscale.sh\" # added by ZeroScale" >> /jffs/configs/profile.add
fi

# Ensure global symlink is available immediately without relogin
if [ -d "/opt/bin" ]; then
  rm -f /opt/bin/tailmon-zer0 /opt/bin/tailmon-zero 2>/dev/null
  if [ ! -L "/opt/bin/zeroscale" ]; then
    ln -s /jffs/scripts/zeroscale.sh /opt/bin/zeroscale 2>/dev/null
  fi
fi

# Check and see if any commandline option is being used
if [ $# -eq 0 ]
  then
    set -- "-noswitch"
fi

# Check and see if an invalid commandline option is being used
# Rung: adding email switch
if [ "$1" == "-h" ] || [ "$1" == "-help" ] || [ "$1" == "-setup" ] || [ "$1" == "-bw" ] || [ "$1" == "-noswitch" ] || [ "$1" == "-screen" ] || [ "$1" == "-now" ] || [ "$1" == "-email" ] || [ "$1" == "-autoupdate" ]
  then
    clear
  else
    clear
    echo ""
    echo "ZeroScale v$version"
    echo ""
    echo "Exiting due to invalid commandline options!"
    echo "(run 'zeroscale.sh -h' for help)"
    echo ""
    echo -e "${CClear}"
    exit 0
fi

# Check to see if the help option is being called
if [ "$1" == "-h" ] || [ "$1" == "-help" ]
  then
  clear
  echo ""
  echo "ZeroScale v$version Commandline Option Usage:"
  echo ""
  echo "zeroscale -h | -help"
  echo "zeroscale -setup"
  echo "zeroscale -bw"
  echo "zeroscale -screen"
  echo "zeroscale -screen -now"
  echo ""
  echo " -h | -help (this output)"
  echo " -setup (displays the setup menu)"
  echo " -bw (runs ZeroScale in monochrome mode)"
  echo " -screen (runs ZeroScale in screen background)"
  echo " -screen -now (runs ZeroScale in screen background immediately)"
  echo ""
  echo -e "${CClear}"
  exit 0
fi

# Rung: added email switch
if [ "$1" == "-email" ]
  then
  amtmemailfailure=1
  sendmessage 1 "ZeroScale email requested"
  exit 0
fi

# Check to see if autoupdate is being called
if [ "$1" == "-autoupdate" ]
  then
    # Grab the ZeroScale config file and read it in
    if [ -f "$config" ]; then
      source "$config"
    else
      initialsetup
    fi

    autoupdate
    exit 0
fi

# Check to see if a second command is being passed to remove color
if [ "$1" == "-bw" ] || [ "$2" == "-bw" ]
  then
    blackwhite
fi

# Check to see if the -now parameter is being called to bypass the screen timer
if [ "$2" == "-now" ]
  then
    bypassscreentimer=1
fi

# Check to see if the setup option is being called
if [ "$1" == "-setup" ]
  then
    # Create the necessary folder/file structure for ZeroScale under /jffs/addons
    if [ ! -d "/jffs/addons/zeroscale.d" ]; then
      mkdir -p "/jffs/addons/zeroscale.d"
    fi
    logoNM
    vsetup
    exit 0
fi

# Check to see if the screen option is being called and run operations normally using the screen utility
if [ "$1" == "-screen" ]
  then
    if ! tailscaleready; then
      clear
      monitoringblocked
      exit 1
    fi

    /opt/sbin/screen -wipe >/dev/null 2>&1 # Kill any dead screen sessions
    sleep 1
    ScreenSess=$(/opt/sbin/screen -ls | awk '/zeroscale/ {split($1,a,"."); print a[1]}')
      if [ -z "$ScreenSess" ]; then
        if [ "$bypassscreentimer" == "1" ]; then
          /opt/sbin/screen -dmS "zeroscale" "$apppath" -noswitch
          sleep 1
          /opt/sbin/screen -r zeroscale
        else
          clear
          echo -e "${CClear}Executing ${CGreen}ZeroScale v$version${CClear} using the SCREEN utility..."
          echo ""
          echo -e "${CClear}IMPORTANT:"
          echo -e "${CClear}In order to keep ZeroScale running in the background,"
          echo -e "${CClear}properly exit the SCREEN session by using: ${CGreen}CTRL-A + D${CClear}"
          echo ""
          /opt/sbin/screen -dmS "zeroscale" "$apppath" -noswitch
          sleep 5
          /opt/sbin/screen -r zeroscale
          exit 0
        fi
      else
        if [ "$bypassscreentimer" == "1" ]; then
          sleep 1
        else
          clear
          echo -e "${CClear}Connecting to existing ${CGreen}ZeroScale v$version${CClear} SCREEN session...${CClear}"
          echo ""
          echo -e "${CClear}IMPORTANT:${CClear}"
          echo -e "${CClear}In order to keep ZeroScale running in the background,${CClear}"
          echo -e "${CClear}properly exit the SCREEN session by using: ${CGreen}CTRL-A + D${CClear}"
          echo ""
          echo -e "${CClear}Switching to the SCREEN session in T-5 sec...${CClear}"
          echo -e "${CClear}"
          spinner 5
        fi
      fi
    /opt/sbin/screen -dr "$ScreenSess"
    exit 0
fi

# Check to see if the noswitch  option is being called
if [ "$1" == "-noswitch" ]
  then
    clear #last switch before the main program starts

    if ! tailscaleready; then
      monitoringblocked
      sleep 1
      vsetup
    fi

    if [ ! -f "$config" ]; then
      initialsetup
    else
      source "$config"
    fi

    #Display ZeroScale Update Log Notifications
    if [ "$track" = "0" ]; then
      if [ "$UpdateNotify" != "0" ]
        then
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: New ZeroScale STABLE TRACK v$DLversion available for download/install." >> "$logfile"
      fi
    fi

    if [ "$track" = "1" ]; then
      if [ "$BUpdateNotify" != "0" ]
        then
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: New ZeroScale BETA TRACK v$Bversion available for download/install." >> "$logfile"
      fi
    fi
fi

# A direct monochrome launch also enters Monitoring Mode without using -noswitch.
if [ "$1" == "-bw" ] || [ "$1" == "-now" ]; then
  if ! tailscaleready; then
    clear
    monitoringblocked
    sleep 1
    vsetup
  fi
fi

# -------------------------------------------------------------------------------------------------------------------------
# Begin ZeroScale Main Loop
# -------------------------------------------------------------------------------------------------------------------------

#DEBUG=; set -x # uncomment/comment to enable/disable debug mode
#{              # uncomment/comment to enable/disable debug mode

# Create the necessary folder/file structure for ZeroScale under /jffs/addons
if [ ! -d "/jffs/addons/zeroscale.d" ]; then
  mkdir -p "/jffs/addons/zeroscale.d"
fi

# (Alias and symlink logic moved to the top of script execution)

if [ ! -f "/opt/bin/timeout" ] || [ ! -f "/opt/sbin/screen" ]; then
  installdependencies
fi

if [ -f "/opt/bin/timeout" ] # If the timeout utility is available then use it and assign variables
  then
    timeoutcmd="timeout "
    timeoutsec="10"
    timeoutlng="60"
  else
    timeoutcmd=""
    timeoutsec=""
    timeoutlng=""
fi

resettimer=0
while true; do

  clear

  # Grab the ZeroScale config file and read it in
  if [ -f "$config" ]; then
    source "$config"
  else
    initialsetup
    if [ -f "$config" ]; then
      source "$config"
    else
      echo "Setup aborted."
      exit 0
    fi
  fi

  while [ -f /jffs/addons/zeroscale.d/updating.txt ]; do
    clear
    echo -e "${CGreen}[ZeroScale is in Maintenance Mode]${CClear}"
    echo ""
    echo -e "Trying again in 30 seconds..."
    echo ""
    spinner 30
  done

  if [ -f "/opt/bin/tailscale" ]; then
    tsinstalled=1

    # Check daemon status
    /opt/etc/init.d/S06tailscaled check >/dev/null 2>&1
    tsservice=$?
    if [ $tsservice -ne 0 ]; then
      tsservicedisp="${CRed}Stopped${CClear}"
    else
      tsservicedisp="${CGreen}Started${CClear}"
    fi

    # Check connection status
    tailscale status >/dev/null 2>&1
    tsconn=$?
    if [ $tsconn -ne 0 ]; then
      tsconndisp="${CRed}Disconnected${CClear}"
    else
      tsconndisp="${CGreen}Connected${CClear}"
    fi

    if [ "$keepalive" -eq 1 ]; then
      keepalivedisp="${CGreen}Active${CClear}"
    else
      keepalivedisp="${CDkGray}Off${CClear}   "
    fi

    if [ "$exitnode" -eq 1 ]; then
      exitnodedisp="${CGreen}Yes${CClear}"
    else
      exitnodedisp="${CDkGray}No${CClear} "
    fi

    if [ "$advroutes" -eq 1 ] && [ -n "$routes" ]; then
      routesdisp="${CGreen}$routes${CClear}"
    else
      routesdisp="${CDkGray}No${CClear}"
    fi

    tsver=$(tailscale version | awk 'NR==1 {print $1}') >/dev/null 2>&1
    if [ -z "$tsver" ]; then tsver="0.00"; fi

    # Display ZeroScale Update Notifications
    if [ "$track" = "0" ]; then
      if [ "$UpdateNotify" != "0" ]; then
        echo -e "$UpdateNotify"
      fi
    fi

    if [ "$track" = "1" ]; then
      if [ "$BUpdateNotify" != "0" ]; then
        echo -e "$BUpdateNotify"
      fi
    fi

    # Display Modern Unified Header
    echo -e "${InvGreen} ${InvDkGray}${CWhite} ZeroScale v$version ── Live Monitor                          $(date +"%a %b %d, %Y %H:%M:%S") ${CClear}"
    echo -e "${InvGreen} ${CClear} Service: [ $tsservicedisp ] (v$tsver)  ${CDkGray}│${CClear} Tailnet: [ $tsconndisp ] (${tsoperatingmode} Mode)"
    echo -e "${InvGreen} ${CClear} Watchdog: [ $keepalivedisp ] (${timerloop}s loop)  ${CDkGray}│${CClear} Exit Node: [ $exitnodedisp ]  ${CDkGray}│${CClear} Routes: [ $routesdisp ]"
    echo -e "${InvGreen} ${CClear}${CDkGray}───────────────────────────────────────────────────────────────────────────────────────${CClear}"
    echo -e "${InvGreen} ${CClear} ${CGreen}(U)${CClear}p/${CGreen}(D)${CClear}own  ${CDkGray}│${CClear}  ${CGreen}(R)${CClear}estart/${CGreen}(S)${CClear}tart/S${CGreen}(t)${CClear}op  ${CDkGray}│${CClear}  ${CGreen}(L)${CClear}ogs  ${CDkGray}│${CClear}  ${CGreen}(C)${CClear}onfiguration  ${CDkGray}│${CClear}  ${CGreen}(E)${CClear}xit"
    echo -e "${InvGreen} ${CClear}${CDkGray}───────────────────────────────────────────────────────────────────────────────────────${CClear}"
    echo ""
    echo -e "${InvDkGray}${CWhite} Tailscale Peer & Network Status:                                                      ${CClear}"
    tailscale status
    echo ""

  else
    echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Tailscale binaries not found. Please investigate." >> "$logfile"
    tsinstalled=0
    vsetup
    continue
  fi

  #Determine if a ZeroScale autoupdate has happened and restart script
  if [ -f /jffs/addons/zeroscale.d/updated.txt ]
    then
      printf "\33[2K\r"
      printf "${CGreen}\r[Replacing ZeroScale with Latest Version]"
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Replacing ZeroScale with latest version." >> "$logfile"
      sleep 1
      rm -f /jffs/addons/zeroscale.d/updated.txt >/dev/null 2>&1
      exec sh /jffs/scripts/zeroscale.sh
      exit 0
  fi

  #Determine if S06tailscaled service settings have changed
  if [ $tsinstalled -eq 1 ] && [ "$persistentsettings" -eq 1 ]; then

    s06args=$(sed -n 's/^ARGS=//p' /opt/etc/init.d/S06tailscaled) 2>/dev/null
    zerocaleargs="\"$args\""

    if [ "$s06args" != "$zerocaleargs" ]; then
      printf "\33[2K\r"
      printf "${CGreen}\r[Tailscale Service settings out-of-sync]"
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Tailscale Service settings are out-of-sync." >> "$logfile"
      sleep 1

      tsdown
      stopts

      #make mods to the S06tailscaled service for Userspace mode
      if [ "$tsoperatingmode" == "Userspace" ]; then
        applyuserspacemode
      #make mods to the S06tailscaled service for Kernel mode
      elif [ "$tsoperatingmode" == "Kernel" ]; then
        applykernelmode
      #make mods to the S06tailscaled service for Custom mode
      elif [ "$tsoperatingmode" == "Custom" ]; then
        applycustomchanges
      fi

      printf "\33[2K\r"
      printf "${CGreen}\r[Tailscale Service settings synced]"
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Tailscale Service settings synced." >> "$logfile"
      sleep 1

      startts
      tsup

      echo ""
      sendmessage 1 "Tailscale Service settings out-of-sync"
      resettimer=1
    fi

  fi

  #Determine if Tailscale service is down
  if [ "$tsinstalled" -eq 1 ] &&
     [ "$keepalive" -eq 1 ] &&
     [ "$tsservice" -ne 0 ]; then

    printf "\33[2K\r"
    printf "${CGreen}\r[Tailscale Status producing errors...Restarting services]"
    echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Tailscale Status producing errors. Restarting services." >> "$logfile"

    sleep 1
    tsdown
    stopts
    startts
    tsup

    resettimer=1
    echo ""
    sendmessage 1 "Tailscale Service Restarted"
  fi

  #Determine if router rebooted (uptime < 10 mins)
  read -r uptime_sec _ < /proc/uptime
  uptime_sec="${uptime_sec%.*}"
  
  if [ "$uptime_sec" -le 600 ] && [ "$routerboot" -eq 0 ]; then
    # Router must have rebooted and send a notification
    printf "\33[2K\r"
    printf "${CGreen}\r[Router appears to have been restarted]"
    echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - WARNING: Router appears to have been unexpectedly restarted." >> "$logfile"
    sleep 1
    echo ""
    sendmessage 1 "Router has been restarted"
    routerboot=1
  fi

  #display a standard timer
  if [ "$resettimer" == "0" ]; then
    timer=0
    while [ "$timer" -ne "$timerloop" ]
      do
        timer=$(($timer+1))
        progressbaroverride $timer "$timerloop" "" "s" "Standard"
        if [ "$resettimer" == "1" ]; then timer=$timerloop; fi
      done
  fi
  resettimer=0

done

exit 0

#} #2>&1 | tee $LOG | logger -t $(basename $0)[$$]  # uncomment/comment to enable/disable debug mode
