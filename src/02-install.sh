# -------------------------------------------------------------------------------------------------------------------------
# Expressinstall script

expressinstallfail()
{
  express_error="$1"

  echo ""
  echo -e "${CRed}ERROR: $express_error${CClear}"
  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Express Install failed: $express_error" >> "$logfile"
  echo ""
  read -rsp $'Press any key to return to setup...\n' -n1 key

  vsetup
  exit 1
}

expressinstall()
{
  echo ""
  echo -e "Ready to Express Install Tailscale?"
  if promptyn "[y/n]: "
  then
    if [ ! -d "/opt" ]; then
      clear
      echo -e "${CRed}ERROR: Entware was not found on this router...${CClear}"
      echo -e "Please install Entware using the AMTM utility before proceeding..."
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Entware was not found installed on router. Please investigate." >> "$logfile"
      echo ""
      read -rsp $'Press any key to continue...\n' -n1 key
      exit 1
    fi
  else
    echo ""
    echo -e "${CClear}"
    exit 0
  fi

  echo ""
  echo -e "\n${CGreen}Updating Entware Packages...${CClear}"
  echo ""
  if ! opkg update; then
    expressinstallfail "Unable to update the Entware package lists."
  fi

  echo ""
  echo -e "Installing Entware ${CGreen}CoreUtils-Timeout${CClear} Package...${CClear}"
  echo ""
  if ! opkg install coreutils-timeout; then
    expressinstallfail "Unable to install the Entware coreutils-timeout package."
  fi

  echo ""
  echo -e "Installing Entware ${CGreen}Screen${CClear} Package...${CClear}"
  echo ""
  if ! opkg install screen; then
    expressinstallfail "Unable to install the Entware screen package."
  fi

  echo ""
  echo -e "${CGreen}Installing Tailscale Package(s)...${CClear}"
  echo ""
  archker=$(opkg print-architecture | grep "armv7-2.6")
  if [ -z "$archker" ]; then
    tspackage="tailscale"
  else
    tspackage="tailscale_nohf"
  fi

  if ! opkg install "$tspackage"; then
    expressinstallfail "Unable to install the Entware $tspackage package."
  fi

  if [ ! -x "/opt/bin/tailscale" ] ||
     [ ! -x "/opt/bin/tailscaled" ] ||
     [ ! -f "/opt/etc/init.d/S06tailscaled" ]; then
    expressinstallfail "The Tailscale binaries or service script were not installed correctly."
  fi

  echo ""
  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Entware package installed." >> "$logfile"

  echo -e "${CGreen}Updating Tailscale Binary to latest version..."
  echo ""
  echo -e "Executing: tailscale update --yes${CClear}"
  echo ""

  tsversion_before="$(tailscale version 2>/dev/null | awk 'NR==1 {print $1}')"
  mkdir -p /opt/tmp
  TMPDIR="/opt/tmp" tailscale update --yes
  tsupdate_rc=$?
  tsversion_after="$(tailscale version 2>/dev/null | awk 'NR==1 {print $1}')"

  if [ -z "$tsversion_after" ]; then
    expressinstallfail "The installed Tailscale version could not be verified after the update."
  fi

  if [ "$tsupdate_rc" -ne 0 ]; then
    if [ "$tsversion_after" = "$tsversion_before" ]; then
      expressinstallfail "The Tailscale binary update failed."
    fi

    # Tailscale may update both binaries successfully but return an error because
    # this Entware service is not managed by systemd or a standard init.d service.
    echo ""
    echo -e "${CYellow}WARNING: tailscale update returned an error, but the binary changed from"
    echo -e "v$tsversion_before to v$tsversion_after and was verified successfully.${CClear}"
  fi

  if [ ! -x "/opt/bin/tailscale" ] || [ ! -x "/opt/bin/tailscaled" ]; then
    expressinstallfail "The updated Tailscale binaries are not executable."
  fi

  echo ""
  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale binary verified at version $tsversion_after." >> "$logfile"

  echo -e "${CGreen}Applying settings for Userspace mode of operation...${CClear}"

  tsoperatingmode="Userspace"
  precmd=""
  args="--tun=userspace-networking --state=/opt/var/tailscaled.state --statedir=/opt/var/lib/tailscale"
  preargs="nohup"
  saveconfig

  echo ""
  echo -e "${CGreen}Applying settings to Tailscale service and connection...${CClear}"

  if ! sed -i "s/^ARGS=.*/ARGS=\"--tun=userspace-networking\ --state=\/opt\/var\/tailscaled.state\ --statedir=\/opt\/var\/lib\/tailscale\"/" "/opt/etc/init.d/S06tailscaled"; then
    expressinstallfail "Unable to apply the Userspace ARGS setting to S06tailscaled."
  fi

  if ! sed -i "s/^PREARGS=.*/PREARGS=\"nohup\"/" "/opt/etc/init.d/S06tailscaled"; then
    expressinstallfail "Unable to apply the Userspace PREARGS setting to S06tailscaled."
  fi

  if ! sed -i -e '/^PRECMD=/d' "/opt/etc/init.d/S06tailscaled"; then
    expressinstallfail "Unable to remove the PRECMD setting from S06tailscaled."
  fi

  inject_s06tailscaled

  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Userspace Mode settings have been applied." >> "$logfile"

  # Remove the legacy firewall-start entry if found.
  if [ -f "/jffs/scripts/firewall-start" ] &&
     grep -q -F "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi" "/jffs/scripts/firewall-start"; then
    sed -i -e '/tailscale down/d' "/jffs/scripts/firewall-start"
  fi

  echo ""
  echo -e "${CGreen}Starting Tailscale service...${CClear}"
  echo ""
  if ! /opt/etc/init.d/S06tailscaled start; then
    expressinstallfail "The Tailscale service command failed to start."
  fi

  service_ready=0
  service_wait=0
  while [ "$service_wait" -lt 5 ]
  do
    sleep 1
    if /opt/etc/init.d/S06tailscaled check >/dev/null 2>&1; then
      service_ready=1
      break
    fi
    service_wait=$((service_wait+1))
  done

  if [ "$service_ready" -ne 1 ]; then
    expressinstallfail "tailscaled did not remain running after startup."
  fi

  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Service started and verified." >> "$logfile"

  echo ""
  echo ""
  echo -e "${CGreen}Starting Tailscale connection...${CClear}"
  echo ""
  echo -e "${CGreen}Please copy the authentication link below into your browser and connect this device"
  echo -e "to your tailnet. Do not paste the link back into the ZeroScale terminal.${CClear}"
  echo ""

  advroutescmd="--advertise-routes=$routes"
  if [ "$sshenable" -eq 1 ]; then
    sshcmd=" --ssh"
  else
    sshcmd=""
  fi

  echo -e "${CGreen}Executing: tailscale up $advroutescmd$sshcmd${CClear}"
  echo ""
  if ! tailscale up $advroutescmd$sshcmd; then
    expressinstallfail "The Tailscale connection did not start correctly."
  fi

  if ! tailscale status >/dev/null 2>&1; then
    expressinstallfail "The Tailscale connection could not be verified after authentication."
  fi

  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Connection started and verified." >> "$logfile"

  echo ""
  echo ""
  echo -e "${CGreen}Express Install Completed Successfully!${CClear}"
  echo ""
  read -rsp $'Press any key to continue...\n' -n1 key

  echo -e "${CClear}"
  return
}

# -------------------------------------------------------------------------------------------------------------------------
# Install script

installts()
{
  clear
  echo -e "${InvGreen} ${InvDkGray}${CWhite} Install Tailscale                                                                     ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} This installer will download and install Tailscale from the Entware respository."
  echo -e "${InvGreen} ${CClear} It will also check for any prerequisites before install commences. You will also"
  echo -e "${InvGreen} ${CClear} have the opportunity to download the latest version directly from Tailscale"
  echo -e "${InvGreen} ${CClear} once the Entware version has been installed during ths process."
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo ""
  echo -e "Install Tailscale?"
  if promptyn "[y/n]: "
    then
      if [ -d "/opt" ]; then # Does entware exist? If yes proceed, if no error out.
        echo ""
        echo -e "\n${CGreen}Updating Entware Packages...${CClear}"
        echo ""
        opkg update
        echo ""
        echo -e "${CGreen}Installing Tailscale Package(s)...${CClear}"
        echo ""
        archker=$(opkg print-architecture | grep "armv7-2.6")
        if [ -z "$archker" ]; then
          opkg install tailscale
        else
          opkg install tailscale_nohf #install special tailscale package for arm7 kernel 2.6
        fi
        echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Entware package installed." >> "$logfile"

        # The Entware package creates a fresh S06tailscaled service script. Apply the
        # operating mode already selected in ZeroScale so package defaults cannot leave
        # the saved mode and the actual service configuration out of sync.
        if [ ! -f "/opt/etc/init.d/S06tailscaled" ]; then
          echo ""
          echo -e "${CRed}ERROR: Tailscale service script was not found after installation.${CClear}"
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: S06tailscaled was not found after package installation." >> "$logfile"
          echo ""
          read -rsp $'Press any key to continue...\n' -n1 key
          return 1
        fi

        echo ""
        echo -e "${CGreen}Applying selected $tsoperatingmode operating mode...${CClear}"
        case "$tsoperatingmode" in
          Userspace)
            applyuserspacemode
            ;;
          Kernel)
            applykernelmode
            ;;
          Custom)
            applycustommode
            ;;
          *)
            echo -e "${CRed}ERROR: Unknown operating mode: $tsoperatingmode${CClear}"
            echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Unknown operating mode '$tsoperatingmode' after package installation." >> "$logfile"
            echo ""
            read -rsp $'Press any key to continue...\n' -n1 key
            return 1
            ;;
        esac

        echo ""
        read -rsp $'Press any key to continue...\n' -n1 key
      else
        clear
        echo -e "${CRed}ERROR: Entware was not found on this router...${CClear}"
        echo -e "Please install Entware using the AMTM utility before proceeding..."
        echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Entware was not found on router. Please investigate." >> "$logfile"
        echo ""
        read -rsp $'Press any key to continue...\n' -n1 key
        exit 1
      fi
    else
      echo ""
      echo -e "${CClear}[Installation cancelled]"
      sleep 1
      return
  fi
  resettimer=1

  echo ""
  echo -e "${CClear}Update Tailscale to the latest version?"
  if promptyn "[y/n]: "
    then
      echo ""
      echo -e "${CGreen}Updating Tailscale Binary${CClear}"
      echo ""
      echo -e "${CGreen}Executing: tailscale update${CClear}"
      echo ""
      mkdir -p /opt/tmp
      TMPDIR="/opt/tmp" tailscale update
      echo -e "${CClear}"
      echo ""
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale binary updated to latest available version." >> "$logfile"
      echo ""
      read -rsp $'Press any key to continue...\n' -n1 key
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# Uninstall script

uninstallts()
{
  clear
  echo -e "${InvGreen} ${InvDkGray}${CWhite} Uninstall Tailscale                                                                   ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} This uninstaller utility will remove Tailscale Entware packages from your router"
  echo -e "${InvGreen} ${CClear} along with all files and modifications made to Tailscale."
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo ""
  echo -e "Uninstall Tailscale?"
  if promptyn "[y/n]: "
    then
      if [ -f /opt/bin/tailscale ]; then
        if [ -d "/opt" ]; then # Does entware exist? If yes proceed, if no error out.
          echo ""
          echo -e "\n${CGreen}Shutting down Tailscale...${CClear}"

          tailscale logout
          tailscale down

          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Connection shut down and logged out." >> "$logfile"

          /opt/etc/init.d/S06tailscaled stop

          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Service shut down." >> "$logfile"
          echo ""
          echo -e "\n${CGreen}Removing firewall-start entries...${CClear}"

          #remove firewall-start entry if found
          if [ -f /jffs/scripts/firewall-start ]; then
            if grep -q -F "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi" /jffs/scripts/firewall-start; then
              sed -i -e '/tailscale down/d' /jffs/scripts/firewall-start
              echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: firewall-start entries removed." >> "$logfile"
            fi
          fi

          echo ""
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

          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Entware package removed." >> "$logfile"

          # Removed the various folders tailscale could hide
          rm -f /opt/var/tailscaled.state >/dev/null 2>&1
          rm -r /opt/var/lib/tailscale >/dev/null 2>&1
          rm -r /opt/var/run/tailscale >/dev/null 2>&1
          rm -r /var/run/tailscale >/dev/null 2>&1
          rm -r /var/lib/tailscale >/dev/null 2>&1

          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale files and folders removed." >> "$logfile"
          echo ""
          read -rsp $'Press any key to continue...\n' -n1 key
        else
          clear
          echo -e "${CRed}ERROR: Entware was not found on this router...${CClear}"
          echo -e "Please install Entware using the AMTM utility before proceeding..."
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Entware not found on router. Please investigate." >> "$logfile"
          echo ""
          read -rsp $'Press any key to continue...\n' -n1 key
          exit 1
        fi
      else
        echo ""
        echo -e "\n${CGreen}Tailscale was not found installed on this router.${CClear}"
        echo ""
        read -rsp $'Press any key to continue...\n' -n1 key
      fi
  fi
  resettimer=1
}

# -------------------------------------------------------------------------------------------------------------------------
# start service script

startts()
{
      printf "\33[2K\r"
      printf "${CGreen}\r[Starting Tailscale Service]"
      sleep 1
      printf "\33[2K\r"
      echo -e "${CGreen}Messages:"
      echo ""
      /opt/etc/init.d/S06tailscaled start
      tsstat=$?
      if [ "$tsstat" -ne 0 ];
        then
          echo ""
          echo -e "${CRed}ERROR: Tailscale Service did not start correctly${CClear}"
          echo ""
          #Display a standard timer#
          timer=0
          while [ $timer -le 5 ]
          do
            timer="$((timer+1))"
                progressbarpause $timer 5 "" "s" "Standard"
          done
          printf "\33[2K\r"
      fi

      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Service started." >> "$logfile"
      echo ""
      resettimer=1
}

# -------------------------------------------------------------------------------------------------------------------------
# stop service script

stopts()
{
      printf "\33[2K\r"
      printf "${CGreen}\r[Stopping Tailscale Service]"
      sleep 1
      printf "\33[2K\r"
      echo -e "${CGreen}Messages:"
      echo ""
      /opt/etc/init.d/S06tailscaled stop
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Service stopped." >> "$logfile"
      echo ""
      resettimer=1
}

# -------------------------------------------------------------------------------------------------------------------------
# restart service and connection

restarttsc()
{
      printf "\33[2K\r"
      printf "${CGreen}\r[Restarting Tailscale Service/Connection]${CClear}"
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

      startts
      tsup

      echo ""
      printf "\33[2K\r"
      printf "${CGreen}\r[Tailscale Service/Connection Successfully Restarted]${CClear}"
      echo -e "\n"
      read -rsp $'Press any key to continue...\n' -n1 key
}

# -------------------------------------------------------------------------------------------------------------------------
# Tailscale reset connection routine

tsreset()
{
      printf "\33[2K\r"
      printf "${CGreen}\r[Initiating Forced Tailscale Connection Reset]"
      sleep 1
      echo -e "\n"
      echo -e "${CRed}WARNING:${CClear} Executing this function will send a 'tailscale up --reset' command which "
      echo -e "will reset any default switches that are configured on your Tailscale connection. "
      echo -e "This action may be necessary at times when these switches are inadvertently set and "
      echo -e "registered with Tailscale, or due to switch functionality being altered or changed "
      echo -e "by the Tailscale developers themselves. Once the '--reset' switch has been sent, "
      echo -e "ZeroScale will reinitialize the connection back to its regular defaults."
      echo ""
      echo -e "${CRed}PLEASE NOTE:${CClear} If you have configured any custom commandline switches that you want "
      echo -e "to reset, you would need to run your own custom Tailscale command in a separate "
      echo -e "prompt to disable the switch that is currently enabled. Please know that the switch "
      echo -e "itself is not removed, but basically disabled. Please consider finding more info at "
      echo -e "the https://tailscale.com/kb site for other references. Examples: "
      echo -e "${CGreen}tailscale up --accept-routes=false${CClear} -or-"
      echo -e "${CGreen}tailscale up --advertise-routes=${CClear}"
      echo ""
      echo -e "Reset Tailscale Connection?"
      if promptyn "[y/n]: "
        then
          echo -e "\n"

          tsdown

          echo "Executing: tailscale up --reset"
          echo ""
          tailscale up --reset
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Connection Reset using --reset switch." >> "$logfile"

          echo ""
          tsdown
          tsup

          echo ""
          printf "\33[2K\r"
          printf "${CGreen}\r[Tailscale Connection Successfully Reset]${CClear}"
          echo -e "\n"
          read -rsp $'Press any key to continue...\n' -n1 key
      fi
}

# -------------------------------------------------------------------------------------------------------------------------
# Tailscale connection reset

tsresetc()
{
      printf "\33[2K\r"
      printf "${CGreen}\r[Resetting Tailscale Connection]"
      sleep 1
      printf "\33[2K\r"
      echo -e "${CGreen}Messages:${CClear}"
      echo ""
      echo "Executing: tailscale up --reset"
      echo ""
      tailscale up --reset
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Connection Reset using --reset switch." >> "$logfile"
      resettimer=1
      echo -e "\n"
      read -rsp $'Press any key to continue...\n' -n1 key
}

# -------------------------------------------------------------------------------------------------------------------------
# Tailscale connection up

tsup()
{
      printf "\33[2K\r"
      printf "${CGreen}\r[Activating Tailscale Connection]"
      sleep 1
      printf "\33[2K\r"

      if [ "$exitnode" -eq 1 ]; then exitnodecmd="--advertise-exit-node "; else exitnodecmd=""; fi
      if [ "$advroutes" -eq 1 ]; then advroutescmd="--advertise-routes=$routes "; else advroutescmd=""; fi
      if [ "$accroutes" -eq 1 ]; then accroutescmd="--accept-routes "; else accroutescmd=""; fi
      if [ "$sshenable" -eq 1 ]; then sshcmd="--ssh"; else sshcmd=""; fi

      echo -e "${CGreen}Messages:${CClear}"
      echo ""

      if [ "$tsoperatingmode" == "Custom" ]; then
        echo "Executing: tailscale up $customcmdline"
        echo ""
        tailscale up $customcmdline
        tsstat=$?
        if [ "$tsstat" -ne 0 ];
          then
            echo -e "${CRed}ERROR: Tailscale Connection did not start correctly${CClear}"
            echo ""
            #Display a standard timer#
            timer=0
            while [ $timer -le 5 ]
            do
              timer="$((timer+1))"
                    progressbarpause $timer 5 "" "s" "Standard"
            done
            printf "\33[2K\r"
        fi
      else
        echo "Executing: tailscale up $exitnodecmd$advroutescmd$accroutescmd$sshcmd"
        echo ""
        tailscale up $exitnodecmd$advroutescmd$accroutescmd$sshcmd
        tsstat=$?
        if [ "$tsstat" -ne 0 ];
          then
            echo -e "${CRed}ERROR: Tailscale Connection did not start correctly${CClear}"
            echo ""
            #Display a standard timer#
            timer=0
            while [ $timer -le 5 ]
            do
              timer="$((timer+1))"
                    progressbarpause $timer 5 "" "s" "Standard"
            done
            printf "\33[2K\r"
        fi
      fi

      # `tailscale up` may leave pasted authentication text/newlines queued on the TTY.
      # Drain that pending terminal input before returning to the setup menu.
      drainpendingttyinput

      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Connection started." >> "$logfile"
      resettimer=1
}

# -------------------------------------------------------------------------------------------------------------------------
# Tailscale connection down

tsdown()
{
      printf "\33[2K\r"
      printf "${CGreen}\r[Bringing Tailscale Connection Down]"
      sleep 1
      printf "\33[2K\r"
      echo -e "${CGreen}Messages:${CClear}"
      echo ""
      echo "Executing: tailscale down"
      echo ""
      tailscale down
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale Connection stopped." >> "$logfile"
      resettimer=1
}

# -------------------------------------------------------------------------------------------------------------------------
# Force Tailscale Binary update

tsupdate()
{
      printf "\33[2K\r"
      printf "${CGreen}\r[Updating Tailscale Binary]"
      sleep 1
      printf "\33[2K\r"

      echo -e "${CGreen}Messages:${CClear}"
      echo ""

      echo "Executing: tailscale update"
      echo ""
      mkdir -p /opt/tmp
      TMPDIR="/opt/tmp" tailscale update

      echo ""
      echo -e "Restart Tailscale?"
      if promptyn "[y/n]: "
        then
        echo ""; echo ""
        restarttsc
      fi

      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale binary updated to latest available version." >> "$logfile"
      resettimer=1
}

# -------------------------------------------------------------------------------------------------------------------------
# Force Tailscale Binary update to latest BETA

tsbeta()
{
  printf "\33[2K\r"
  printf "${CGreen}\r[Updating Tailscale Binary to Latest BETA]"
  sleep 1
  printf "\33[2K\r"

  echo -e "${CGreen}Messages:${CClear}"
  echo ""

  echo "Executing: tailscale update --track unstable"
  echo ""
  mkdir -p /opt/tmp
  TMPDIR="/opt/tmp" tailscale update --track unstable

  echo ""
  echo -e "Restart Tailscale?"
  if promptyn "[y/n]: "
    then
    echo ""; echo ""
    restarttsc
  fi

  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale binary updated to latest BETA version." >> "$logfile"
  resettimer=1
}

# -------------------------------------------------------------------------------------------------------------------------
# setipforwarding is a function that enables or disables IP forwarding on the router
setipforwarding()
{
  # Check if IPv4 forwarding is already enabled. if so, we assume this is either already set, or managed elsewhere
  if [ -f "/proc/sys/net/ipv4/ip_forward" ] && grep -q "^1$" /proc/sys/net/ipv4/ip_forward; then
    return
  fi

  echo 1 > /proc/sys/net/ipv4/ip_forward
  echo 1 > /proc/sys/net/ipv6/conf/all/forwarding
  if [ ! -f "/jffs/scripts/init-start" ]; then
    echo "#!/bin/sh" > /jffs/scripts/init-start
    chmod 755 /jffs/scripts/init-start
  fi
  if ! grep -q -F "echo 1 > /proc/sys/net/ipv4/ip_forward" /jffs/scripts/init-start; then
    echo "echo 1 > /proc/sys/net/ipv4/ip_forward" >> /jffs/scripts/init-start
  fi
  if ! grep -q -F "echo 1 > /proc/sys/net/ipv6/conf/all/forwarding" /jffs/scripts/init-start; then
    echo "echo 1 > /proc/sys/net/ipv6/conf/all/forwarding" >> /jffs/scripts/init-start
  fi
  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: IP Forwarding enabled." >> "$logfile"
}

# -------------------------------------------------------------------------------------------------------------------------
# booleantoyesno converts boolean values to yes or no for display
booleantoyesno()
{
  if [ "$1" -eq 1 ]; then
    echo "Yes"
  else
    echo "No"
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# vercompare compares two dotted version strings numerically (ignoring any non-numeric beta suffix,
# e.g. "5b1" is treated as "5") and echoes "gt", "lt", or "eq" depending on whether $1 is greater
# than, less than, or equal to $2.
vercompare()
{
  awk -v v1="$1" -v v2="$2" 'BEGIN {
    n1=split(v1,a,".")
    n2=split(v2,b,".")
    n=(n1>n2)?n1:n2
    for(i=1;i<=n;i++) {
      x=(i<=n1)?a[i]+0:0
      y=(i<=n2)?b[i]+0:0
      if (x>y) { print "gt"; exit }
      if (x<y) { print "lt"; exit }
    }
    print "eq"
  }'
}

