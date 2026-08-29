# -------------------------------------------------------------------------------------------------------------------------
# autoupdate will automatically download and install new ZeroScale scripts and Tailscale binaries - run via CRON job/switch

autoupdate()
{

  clear

  # Put ZeroScale into maintenance mode
  echo > /jffs/addons/zeroscale.d/updating.txt

  #Display ZeroScale client header
  echo -en "${InvGreen} ${InvDkGray} ZeroScale - v"
  printf "%-8s" "$version"
  echo -e "                      ${CWhite}Run Auto Update${InvDkGray}                  $tzspaces$(date) ${CClear}"
  echo ""

  if [ "$updatetm" -eq 1 ]
    then
      printf "\33[2K\r"
      printf "${CGreen}\r[Checking Local ZeroScale Version]"

      # Copy current version of script into a version file
      echo "$version" > "/jffs/addons/zeroscale.d/localver.txt"
      sleep 1

      printf "\33[2K\r"

      # Download the latest version file from the source repository
      if [ "$track" = "1" ]
        then
        printf "${CGreen}\r[Checking ZeroScale BETA Version]"
        curl --silent --retry 3 --connect-timeout 3 --max-time 6 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/beta/version.txt" -o "/jffs/addons/zeroscale.d/beta.txt"
      else
        printf "${CGreen}\r[Checking Official ZeroScale Version]"
        curl --silent --retry 3 --connect-timeout 3 --max-time 6 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/main/version.txt" -o "/jffs/addons/zeroscale.d/version.txt"
      fi

      sleep 1
      officialverchk=$?
      if [ $officialverchk -ne 0 ]
        then
        printf "\33[2K\r"
        printf "${CGreen}\r[Unable to Determine ZeroScale Version...Exiting]\n"
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Unable to determine ZeroScale version -- please check your internet connection. Autoupdate exiting." >> "$logfile"
        echo -e "${CClear}"
        sendmessage 1 "Unable to reach ZeroScale repository"
        sleep 1
        rm -f /jffs/addons/zeroscale.d/updating.txt >/dev/null 2>&1
        exit 1
      fi
      sleep 1

      printf "\33[2K\r"
      printf "${CGreen}\r[Comparing ZeroScale Versions]"
      sleep 1

      # Check differences in version and download if newer official version is present
      if [ "$track" = "1" ]; then
        local localver=$(cat "/jffs/addons/zeroscale.d/localver.txt")
        local serverver=$(cat "/jffs/addons/zeroscale.d/beta.txt")
      else
        local localver=$(cat "/jffs/addons/zeroscale.d/localver.txt")
        local serverver=$(cat "/jffs/addons/zeroscale.d/version.txt")
      fi
      if [ "$localver" != "$serverver" ]
        then
          printf "\33[2K\r"

          if [ "$track" = "1" ]
            then
            printf \"%s\" "${CGreen}\r[Downloading New ZeroScale BETA v$serverver]\n"
            curl --silent --retry 3 --connect-timeout 3 --max-time 5 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/beta/zeroscale.sh" -o "/jffs/scripts/zeroscale.sh" && chmod 755 "/jffs/scripts/zeroscale.sh"
          else
            printf \"%s\" "${CGreen}\r[Downloading New ZeroScale STABLE v$serverver]\n"
            curl --silent --retry 3 --connect-timeout 3 --max-time 5 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/main/zeroscale.sh" -o "/jffs/scripts/zeroscale.sh" && chmod 755 "/jffs/scripts/zeroscale.sh"
          fi

          echo -e "${CClear}"
          sleep 1
          officialver=$?
          if [ $officialver -ne 0 ]
            then
              printf "\33[2K\r"
              printf "${CGreen}\r[Unable to Download ZeroScale...Exiting]\n"
            echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Unable to download ZeroScale -- please check your internet connection. Autoupdate exiting." >> "$logfile"
              echo -e "${CClear}"
              sendmessage 1 "Unable to reach ZeroScale repository"
              sleep 1
              rm -f /jffs/addons/zeroscale.d/updating.txt >/dev/null 2>&1
              exit 1
          fi
          verdirection=$(vercompare "$serverver" "$localver")
          if [ "$verdirection" = "lt" ]; then
            if [ "$track" = "1" ]; then tracklabel="Beta"; else tracklabel="Stable"; fi
            echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Installed ZeroScale v$localver did not match the configured $tracklabel track -- corrected to v$serverver" >> "$logfile"
          else
            echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Successfully autoupdated ZeroScale from v$localver to v$serverver" >> "$logfile"
          fi
          sendmessage 0 "ZeroScale Script Successfully Updated" "$localver" "$serverver" "$verdirection"
          echo > /jffs/addons/zeroscale.d/updated.txt
      else
        printf "\33[2K\r"
        printf "${CGreen}\r[Local ZeroScale Version is the Latest Available]\n"
        echo -e "${CClear}"
        sleep 1
      fi
      sleep 1
  fi

  if [ "$updatets" -eq 1 ]
    then
      printf "\33[2K\r"
      printf "${CGreen}\r[Checking Local Tailscale Version]"
      sleep 1

      # Checking for local Tailscale version
      tailscale version | awk 'NR==1 {print $1}' > /jffs/addons/zeroscale.d/localtsver.txt
      localtsverchk=$?
      if [ $localtsverchk -ne 0 ]
        then
          printf "\33[2K\r"
          printf "${CGreen}\r[Unable to Determine Local Tailscale Version...Exiting]\n"
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Unable to determine local Tailscale version -- please check your installation. Autoupdate exiting." >> "$logfile"
          echo -e "${CClear}"
          sleep 2
          rm -f /jffs/addons/zeroscale.d/updating.txt >/dev/null 2>&1
          exit 1
      fi
      sleep 1

      printf "\33[2K\r"
      printf "${CGreen}\r[Checking Official Tailscale Version]"
      sleep 1

      # Checking for upstream Tailscale version
      tailscale version --upstream | awk -F":" '/upstream/ {print $2}' | sed "s/^ //" > /jffs/addons/zeroscale.d/tsversion.txt
      upstreamtsverchk=$?
      if [ $upstreamtsverchk -ne 0 ]
        then
          printf "\33[2K\r"
          printf "${CGreen}\r[Unable to Determine Official Tailscale Version...Exiting]\n"
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Unable to determine Official Tailscale version -- please check your installation/internet connection. Autoupdate exiting." >> "$logfile"
          echo -e "${CClear}"
          sendmessage 1 "Unable to reach Tailscale repository"
          sleep 1
          rm -f /jffs/addons/zeroscale.d/updating.txt >/dev/null 2>&1
          exit 1
      fi
      sleep 1

      printf "\33[2K\r"
      printf "${CGreen}\r[Comparing Tailscale Versions]"
      sleep 1

      # Check differences in version and download if newer official version is present
      localtsver=$(cat "/jffs/addons/zeroscale.d/localtsver.txt")
      servertsver=$(cat "/jffs/addons/zeroscale.d/tsversion.txt")
      if [ "$localtsver" != "$servertsver" ]
        then
          printf "\33[2K\r"
          printf \"%s\" "${CGreen}\r[Downloading New Tailscale Binary v$servertsver]\n"
          echo -e "${CClear}"
          sleep 1
          mkdir -p /opt/tmp
          TMPDIR="/opt/tmp" tailscale update --yes
          officialtsver=$?
          if [ $officialtsver -ne 0 ]
            then
              printf "\33[2K\r"
              printf "${CGreen}\r[Unable to Download Tailscale Binary...Exiting]\n"
              echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Unable to download Tailscale Binary - please check your installation/internet connection." >> "$logfile"
              echo -e "${CClear}"
              sendmessage 1 "Unable to reach Tailscale repository"
              sleep 1
              rm -f /jffs/addons/zeroscale.d/updating.txt >/dev/null 2>&1
              exit 1
          fi
          echo ""
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Successfully autoupdated Tailscale Binary from v$localtsver to v$servertsver" >> "$logfile"
          sendmessage 0 "Tailscale Successfully Updated" "$localtsver" "$servertsver"

          # Upon a successful update, restart Tailscale services
          echo ""; echo ""
          printf "\33[2K\r"
          printf "${CGreen}\r[Restarting Tailscale Service/Connection]\n"
          echo -e "${CClear}"
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
          printf "${CGreen}\r[Tailscale Service/Connection Successfully Restarted]\n"
          echo -e "${CClear}"
          sleep 1
          printf "\33[2K\r"
          printf "${CGreen}\r[Autoupdate Completed Successfully]\n"
          echo -e "${CClear}"
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Autoupdate completed successfully." >> "$logfile"
          sleep 1
          rm -f /jffs/addons/zeroscale.d/updating.txt >/dev/null 2>&1
          exit 0

      else
        printf "\33[2K\r"
        printf "${CGreen}\r[Local Tailscale Version is the Latest Available...Exiting]\n"
        echo -e "${CClear}"
        sleep 1

      fi
  fi

  printf "\33[2K\r"
  printf "${CGreen}\r[Autoupdate Completed Successfully]\n"
  echo -e "${CClear}"
  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Autoupdate completed successfully." >> "$logfile"
  sleep 1
  rm -f /jffs/addons/zeroscale.d/updating.txt >/dev/null 2>&1
  exit 0

}

# -------------------------------------------------------------------------------------------------------------------------
# Force Downgrade Tailscale Binary update

check_url()
{
    # Using curl to check for a valid version archive
    curl -s --head --fail "$1" >/dev/null 2>&1
}

tsdowngrade()
{

  printf "\33[2K\r"
  printf "${CGreen}\r[Downgrading Tailscale Binary]"
  sleep 1
  echo ""; echo ""
  echo -e "${CGreen}Messages:${CClear}"

  while true; do
    # Prompt the user for the Tailscale version.
    echo ""
    printf "Please enter the Tailscale version to downgrade to (ex: 1.84.0, e=Exit): "
    read -r TS_VERSION

    if [ -z "$TS_VERSION" ]; then
      echo ""
      echo -e "${CRed}No version entered. Please try again.${CClear}"
      echo ""
      continue
    elif [ "$TS_VERSION" = "e" ]; then
      echo ""
      echo -e "${CClear}[Exiting]"
      sleep 1
      return
    fi

    echo ""
    echo -e "${CClear}Are you downgrading to a Tailscale Beta Version? (y=Beta, n=Stable)?"
    TS_BETA=0
    if promptyn "[y/n]: "
      then
      TS_BETA=1
    fi

    # Determine system architecture to build the correct download URL.
    ARCH=$(uname -m)
    case $ARCH in
      "aarch64" | "arm64")
        TS_ARCH="arm64"
        ;;
      "armv7l")
        TS_ARCH="arm"
        ;;
      *)
        echo ""; echo ""
        echo -e "${CRed}Not sure how you did it, but you're running an unsupported architecture: $ARCH ${CClear}"
        sleep 2
        exit 1
        ;;
    esac

    # Construct the download URL.
    if [ "$TS_BETA" -eq 1 ]; then
       DOWNLOAD_URL="https://pkgs.tailscale.com/unstable/tailscale_${TS_VERSION}_${TS_ARCH}.tgz"
    else
       DOWNLOAD_URL="https://pkgs.tailscale.com/stable/tailscale_${TS_VERSION}_${TS_ARCH}.tgz"
    fi

    # Validate the version by checking if the URL is reachable.
    echo ""; echo ""
    echo -e "${CGreen}Verifying version:${CClear} $TS_VERSION ${CGreen}for architecture:${CClear} $TS_ARCH"
    echo ""
    if ! check_url "$DOWNLOAD_URL"; then
      echo -e "------------------------------------------------------------------"
      echo -e "${CRed}Error: Invalid version or version not found for your architecture.${CClear}"
      echo -e "URL checked: $DOWNLOAD_URL"
      echo -e "Please check the version number and try again."
      echo -e "------------------------------------------------------------------"
      continue # Go back to the start of the loop
    fi

    echo -e "${CGreen}Version is valid. Proceeding with download...${CClear}"
    echo ""

    # Define file paths
    TMP_DIR="/opt/tmp"
    mkdir -p "$TMP_DIR"
    DOWNLOAD_PATH="$TMP_DIR/tailscale_${TS_VERSION}_${TS_ARCH}.tgz"
    EXTRACT_DIR="$TMP_DIR/tailscale_${TS_VERSION}_${TS_ARCH}"
    DEST_DIR="/opt/bin"

    # Download the package to the /tmp folder.
    echo -e "${CGreen}Downloading from:${CClear} $DOWNLOAD_URL"
    echo ""
    if ! curl -L -o "$DOWNLOAD_PATH" "$DOWNLOAD_URL"; then
      echo ""
      echo -e "${CRed}Error: Download failed. Please check your internet connection.${CClear}"
      continue
    fi

    echo ""
    echo -e "${CGreen}Download complete.${CClear}"
    echo ""

    # Extract the 'tailscale' and 'tailscaled' binaries.
    echo -e "${CGreen}Extracting Tailscale binaries to:${CClear} $TMP_DIR"
    echo ""

    # Extract the whole archive and then find our files.
    if ! tar -xzf "$DOWNLOAD_PATH" -C "$TMP_DIR"; then
      echo -e "${CRed}Error: Extraction failed.${CClear}"
      rm -f "$DOWNLOAD_PATH" # Clean up failed download
      continue
    fi

    # The extracted files should be inside a directory like /tmp/tailscale_1.84.0
    SOURCE_TAILSCALE="$EXTRACT_DIR/tailscale"
    SOURCE_TAILSCALED="$EXTRACT_DIR/tailscaled"

    # Verify that the binaries were extracted
    if [ ! -f "$SOURCE_TAILSCALE" ] || [ ! -f "$SOURCE_TAILSCALED" ]; then
      echo -e "${CRed}Error: The required binaries 'tailscale' or 'tailscaled' were not found in the archive.${CClear}"
      # Clean up
      rm -f "$DOWNLOAD_PATH"
      rm -rf "$EXTRACT_DIR"
      continue
    fi

    echo -e "${CGreen}Extraction successful.${CClear}"
    echo ""

    # Check if the destination directory exists.
    if [ ! -d "$DEST_DIR" ]; then
      echo -e "{$CRed}Destination directory${CClear} $DEST_DIR {$CRed}does not exist. Please install Entware...${CClear}"
      exit 1
    fi

    # Stop any running Tailscale serices
    echo -e "${CGreen}Stopping current Tailscale Service and Connection...${CClear}"
    echo ""

    tsdown
    stopts

    # Delete the existing files if they exist.
    echo -e "${CGreen}Removing current Tailscale versions from${CClear} $DEST_DIR..."
    echo ""
    rm -f "$DEST_DIR/tailscale"
    rm -f "$DEST_DIR/tailscaled"

    # Move the two extracted files to /opt/bin.
    echo -e "${CGreen}Moving downloaded Tailscale binaries to${CClear} $DEST_DIR..."
    echo ""
    if ! mv "$SOURCE_TAILSCALE" "$DEST_DIR/"; then
      echo -e "${CRed}Error moving tailscale binary.${CClear}"
      # Clean up
      rm -f "$DOWNLOAD_PATH"
      rm -rf "$EXTRACT_DIR"
      continue
    fi
    if ! mv "$SOURCE_TAILSCALED" "$DEST_DIR/"; then
      echo -e "${CRed}Error moving tailscaled binary.${CClear}"
      # Clean up
      rm -f "$DOWNLOAD_PATH"
      rm -rf "$EXTRACT_DIR"
      continue
    fi

    # Make them both executable.
    echo -e "${CGreen}Setting Tailscale permissions...${CClear}"
    echo ""
    chmod 755 "$DEST_DIR/tailscale"
    chmod 755 "$DEST_DIR/tailscaled"

    echo -e "${CGreen}Tailscale has been successfully updated to${CClear} $TS_VERSION ${CGreen}and installed to${CClear} $DEST_DIR."
    echo ""

    # Clean up the downloaded archive and extracted folder
    echo -e "${CGreen}Cleaning up temporary files...${CClear}"
    echo ""
    rm -f "$DOWNLOAD_PATH"
    rm -rf "$EXTRACT_DIR"

    # Exit the loop on success
    break
  done

  echo -e "${CClear}Restart Tailscale using downgraded version $TS_VERSION?"
  if promptyn "[y/n]: "
    then
    echo ""; echo ""
    restarttsc
  fi

  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale binaries successfully updated to $TS_VERSION" >> "$logfile"
  resettimer=1
}

# -------------------------------------------------------------------------------------------------------------------------
# schedulevpnreset lets you enable and set a time for a scheduled daily vpn reset

##----------------------------------------##
## Modified by Martinski W. [2024-Oct-06] ##
##----------------------------------------##
scheduleautoupdates()
{

while true
do
  clear
  echo -e "${InvGreen} ${InvDkGray}${CWhite} ZeroScale Autoupdate Scheduler                                                          ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Please indicate below if you would like to enable and schedule a daily autoupdate CRON"
  echo -e "${InvGreen} ${CClear} job. This can check for both ZeroScale and Tailscale updates. Please NOTE: Autoupdate"
  echo -e "${InvGreen} ${CClear} will only update to the latest stable release. Beta updates need to handled manually"
  echo -e "${InvGreen} ${CClear} using the option in the Main Setup & Configuration menu."
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} (Autoupdate Default = Disabled)"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Use the corresponding ${CGreen}()${CClear} key to enable/disable autoupdates:${CClear}"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"

  if [ "$updatetm" == "1" ]; then updatetmdisp="${CGreen}Enabled${CCyan}"; else updatetm=0; updatetmdisp="${CRed}Disabled${CCyan}"; fi
  if [ "$updatets" == "1" ]; then updatetsdisp="${CGreen}Enabled${CCyan}"; else updatets=0; updatetsdisp="${CRed}Disabled${CCyan}"; fi

  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}Autoupdate ZeroScale Script  ${CClear} ${CGreen}(1)   -${CClear} $updatetmdisp${CClear}"
  echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}Autoupdate Tailscale Binary${CClear} ${CGreen}(2)   -${CClear} $updatetsdisp${CClear}"
  echo -e "${InvGreen} ${CClear}"
  if [ "$schedule" = "0" ]
  then
     echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}Enable Autoupdate Schedule ${CClear} ${CGreen}(Y/N) - ${CRed}Disabled${CClear}"
  elif [ "$schedule" = "1" ]
  then
     schedhrs="$(awk "BEGIN {printf \"%02.f\",${schedulehrs}}")"
     schedmin="$(awk "BEGIN {printf \"%02.f\",${schedulemin}}")"
     schedtime="${CGreen}$schedhrs:$schedmin${CClear}"
     echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}Enable Autoupdate Schedule ${CClear} ${CGreen}(Y/N) - Enabled, Daily @ $schedtime${CClear}"
  fi
  echo ""
  read -p "Please select? (1-2, n=Disable Schedule, y=Enable Schedule, e=Exit): " newSchedule
    case $newSchedule in
      1) if [ "$updatetm" == "0" ]; then updatetm=1; updatetmdisp="${CGreen}Enabled${CCyan}"; elif [ "$updatetm" == "1" ]; then updatetm=0; updatetmdisp="${CRed}Disabled${CCyan}"; fi; saveconfig;;

      2) if [ "$updatets" == "0" ]; then updatets=1; updatetsdisp="${CGreen}Enabled${CCyan}"; elif [ "$updatets" == "1" ]; then updatets=0; updatetsdisp="${CRed}Disabled${CCyan}"; fi; saveconfig;;

      [Nn])
          schedule=0
          if [ -f /jffs/scripts/services-start ]
          then
            sed -i -e '/zeroscale.sh/d' /jffs/scripts/services-start
            cru d RunZeroScalecheck
            schedulehrs=1
            schedulemin=0
            echo ""
            echo -e "${CGreen}[Modifiying SERVICES-START file]..."
            sleep 2
            echo ""
            echo -e "${CGreen}[Modifying CRON jobs]..."
            sleep 2
            echo -e "$(date +'%b %d %Y %X') $(_GetLAN_HostName_) ZEROSCALE[$$] - INFO: Autoupdate Scheduled Check Disabled" >> "$logfile"
            saveconfig
          fi
      ;;

      [Yy])
          schedule=1
          echo ""
          echo -e "${InvGreen} ${InvDkGray}${CWhite} Select CRON Job Time                                                                  ${CClear}"
          echo -e "${InvGreen} ${CClear}"
          echo -e "${InvGreen} ${CClear} Please indicate below what time you would like to schedule a daily Autoupdate CRON"
          echo -e "${InvGreen} ${CClear} job. (Default = 1 hr, 0 min = 01:00 = 1:00am)"
          echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
          echo
          read -p 'Schedule HOURS [0-23]?: ' newScheduleHrs
          if [ -z "$newScheduleHrs" ]
          then
              if _ValidateCronJobHour_ "$schedulehrs"
              then scheduleHrsOK=true
              else scheduleHrsOK=false
              fi
          elif _ValidateCronJobHour_ "$newScheduleHrs"
          then
              scheduleHrsOK=true
              schedulehrs="$newScheduleHrs"
          else
              scheduleHrsOK=false
              schedulehrs="${schedulehrs:=1}"
              printf "${CRed}*ERROR*: INVALID Entry.${CClear}\n\n"
          fi
          read -p 'Schedule MINUTES [0-59]?: ' newScheduleMins
          if [ -z "$newScheduleMins" ]
          then
              if _ValidateCronJobMinute_ "$schedulemin"
              then scheduleMinsOK=true
              else scheduleMinsOK=false
              fi
          elif _ValidateCronJobMinute_ "$newScheduleMins"
          then
              scheduleMinsOK=true
              schedulemin="$newScheduleMins"
          else
              scheduleMinsOK=false
              schedulemin="${schedulemin:=0}"
              printf "${CRed}*ERROR*: INVALID Entry.${CClear}\n"
          fi
          if ! "$scheduleHrsOK" || ! "$scheduleMinsOK"
          then
              doResetSave=false
              if ! "$scheduleHrsOK" && ! _ValidateCronJobHour_ "$schedulehrs"
              then schedulehrs=1 ; doResetSave=true
              fi
              if ! "$scheduleMinsOK" && ! _ValidateCronJobMinute_ "$schedulemin"
              then schedulemin=0 ; doResetSave=true
              fi
              if "$doResetSave"
              then
                  schedule=0
                  saveconfig
                  printf "\n${CRed}INVALID input found. Resetting values.${CClear}\n\n"
              else
                  printf "\n${CRed}INVALID input found. No changes made.${CClear}\n\n"
              fi
              echo -e "${CClear}[Exiting]"
              timer="$timerloop"
              sleep 3
              break
          fi
          echo
          echo -e "${CGreen}[Modifying SERVICES-START file]..."
          sleep 2

          if [ -f /jffs/scripts/services-start ]
          then
            if ! grep -q -F "sh /jffs/scripts/zeroscale.sh -autoupdate" /jffs/scripts/services-start
            then
              echo 'cru a RunZeroScalecheck "'"$schedulemin $schedulehrs * * * sh /jffs/scripts/zeroscale.sh -autoupdate"'"' >> /jffs/scripts/services-start
              cru a RunZeroScalecheck "$schedulemin $schedulehrs * * * sh /jffs/scripts/zeroscale.sh -autoupdate"
            else
              #delete and re-add if it already exists in case there's a time change
              sed -i -e '/zeroscale.sh/d' /jffs/scripts/services-start
              cru d RunZeroScalecheck
              echo 'cru a RunZeroScalecheck "'"$schedulemin $schedulehrs * * * sh /jffs/scripts/zeroscale.sh -autoupdate"'"' >> /jffs/scripts/services-start
              cru a RunZeroScalecheck "$schedulemin $schedulehrs * * * sh /jffs/scripts/zeroscale.sh -autoupdate"
            fi
          else
            echo 'cru a RunZeroScalecheck "'"$schedulemin $schedulehrs * * * sh /jffs/scripts/zeroscale.sh -autoupdate"'"' >> /jffs/scripts/services-start
            chmod 755 /jffs/scripts/services-start
            cru a RunZeroScalecheck "$schedulemin $schedulehrs * * * sh /jffs/scripts/zeroscale.sh -autoupdate"
          fi

          echo
          echo -e "${CGreen}[Modifying CRON jobs]..."
          sleep 2
          echo -e "$(date +'%b %d %Y %X') $(_GetLAN_HostName_) ZEROSCALE[$$] - INFO: Autoupdate Scheduled Check Enabled" >> "$logfile"
          saveconfig
      ;;

      [Ee])
          echo ; echo -e "${CClear}[Exiting]"
          sleep 2
          saveconfig
          return
      ;;

    esac
done
}

##-------------------------------------##
## Added by Martinski W. [2024-Oct-06] ##
##-------------------------------------##
_ValidateCronJobHour_()
{
   if [ $# -eq 0 ] || [ -z "$1" ] ; then return 1 ; fi
   if [ "$1" -ge 0 ] 2>/dev/null && [ "$1" -lt 24 ] 2>/dev/null
   then return 0 ; else return 1 ; fi
}

_ValidateCronJobMinute_()
{
    if [ $# -eq 0 ] || [ -z "$1" ] ; then return 1 ; fi
    if [ "$1" -ge 0 ] 2>/dev/null && [ "$1" -lt 60 ] 2>/dev/null
    then return 0 ; else return 1 ; fi
}

##-------------------------------------##
## Added by Martinski W. [2024-Oct-05] ##
##-------------------------------------##
_SetLAN_HostName_()
{
   [ -z "${LAN_HostName:+xSETx}" ] && \
   LAN_HostName="$($timeoutcmd$timeoutsec nvram get lan_hostname)"
}

_GetLAN_HostName_()
{ _SetLAN_HostName_ ; echo "$LAN_HostName" ; }

# -------------------------------------------------------------------------------------------------------------------------
# autostart lets you enable the ability for ZeroScale to autostart after a router reboot

autostart()
{
while true; do
  clear
  echo -e "${InvGreen} ${InvDkGray}${CWhite} Reboot Protection                                                                     ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Please indicate below if you would like to enable ZeroScale to autostart after a"
  echo -e "${InvGreen} ${CClear} router reboot. This will ensure continued, uninterrupted Tailscale monitoring."
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} (Default = Disabled)"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo -e "${InvGreen} ${CClear}"
  if [ "$autostart" == "0" ]; then
    echo -e "${InvGreen} ${CClear} Current: ${CRed}Disabled${CClear}"
  elif [ "$autostart" == "1" ]; then
    echo -e "${InvGreen} ${CClear} Current: ${CGreen}Enabled${CClear}"
  fi
  echo ""
  read -p 'Enable Reboot Protection? (0=No, 1=Yes, e=Exit): ' autostart1

  if [ "$autostart1" == "" ] || [ -z "$autostart1" ]; then autostart=0; else autostart="$autostart1"; fi # Using default value on enter keypress

  if [ "$autostart" == "0" ]; then

    if [ -f /jffs/scripts/post-mount ]; then
      sed -i -e '/zeroscale.sh/d' /jffs/scripts/post-mount
      autostart=0
      echo ""
      echo -e "${CGreen}[Modifying POST-MOUNT file]..."
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Reboot Protection Disabled" >> "$logfile"
      saveconfig
      sleep 1
      timer=$timerloop
      break
    fi

  elif [ "$autostart" == "1" ]; then

    if [ -f /jffs/scripts/post-mount ]; then

      if ! grep -q -F "(sleep 30 && /jffs/scripts/zeroscale.sh -screen) & # Added by ZeroScale" /jffs/scripts/post-mount; then
        echo "(sleep 30 && /jffs/scripts/zeroscale.sh -screen) & # Added by ZeroScale" >> /jffs/scripts/post-mount
        autostart=1
        echo ""
        echo -e "${CGreen}[Modifying POST-MOUNT file]..."
        echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Reboot Protection Enabled" >> "$logfile"
        saveconfig
        sleep 1
        timer=$timerloop
        break
      else
        autostart=1
        saveconfig
        sleep 1
      fi

    else
      echo "#!/bin/sh" > /jffs/scripts/post-mount
      echo "" >> /jffs/scripts/post-mount
      echo "(sleep 30 && /jffs/scripts/zeroscale.sh -screen) & # Added by ZeroScale" >> /jffs/scripts/post-mount
      chmod 755 /jffs/scripts/post-mount
      autostart=1
      echo ""
      echo -e "${CGreen}[Modifying POST-MOUNT file]..."
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Reboot Protection Enabled" >> "$logfile"
      saveconfig
      sleep 1
      timer=$timerloop
      break
    fi

  elif [ "$autostart" == "e" ]; then
  timer=$timerloop
  break

  else
    autostart=0
    saveconfig
  fi

done
}

# -------------------------------------------------------------------------------------------------------------------------
# timerloopconfig lets you configure how long you want the timer cycle to last between tailscale checks

timerloopconfig()
{
  clear
  echo -e "${InvGreen} ${InvDkGray}${CWhite} Status Check Loop Interval                                                            ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Sets the delay in seconds between Tailscale service and connection checks.${CClear}"
  echo -e "${InvGreen} ${CClear} (Current: ${CGreen}${timerloop}s${CClear}, Allowed: 10-300s, Default: 60s)"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo ""
  read -p "Enter new interval in seconds (10-300, e=Cancel): " EnterTimerLoop
  case "$EnterTimerLoop" in
    [0-9]|[0-9][0-9]|[0-9][0-9][0-9])
      if [ "$EnterTimerLoop" -ge 10 ] && [ "$EnterTimerLoop" -le 300 ]; then
        timerloop=$EnterTimerLoop
        saveconfig
        timer=$timerloop
        echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Check interval set to ${timerloop}s." >> "$logfile"
      fi
      ;;
  esac
}

# -------------------------------------------------------------------------------------------------------------------------
# customconfig lets you edit the args and settings for tailscale

customconfig()
{
restartts=0
while true; do
  clear
  echo -e "${InvGreen} ${InvDkGray}${CWhite} Custom Tailscale Configuration                                                        ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} This functionality allows you to choose your own Tailscale ARGS, PREARGS and PRECMD${CClear}"
  echo -e "${InvGreen} ${CClear} entries, and allows you to modify the Tailscale connection commandline options.${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} ${CYellow}Proceed at your own risk!${CClear}"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Current Operating Mode: ${CGreen}$tsoperatingmode${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Current values in Tailscale Service (/opt/etc/init.d/S06tailscaled):${CClear}"

  s06args=$(sed -n 's/^ARGS=//p' /opt/etc/init.d/S06tailscaled) 2>/dev/null
  if [ -z "$s06args" ]; then
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(1)${CClear} ${CGreen}ARGS=\"\"${CClear}"
  else
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(1)${CClear} ${CGreen}ARGS=$s06args${CClear}"
  fi

  s06preargs=$(sed -n 's/^PREARGS=//p' /opt/etc/init.d/S06tailscaled) 2>/dev/null
  if [ -z "$s06preargs" ]; then
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(2)${CClear} ${CGreen}PREARGS=\"\"${CClear}"
  else
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(2)${CClear} ${CGreen}PREARGS=$s06preargs${CClear}"
  fi

  s06precmd=$(sed -n 's/^PRECMD=//p' /opt/etc/init.d/S06tailscaled) 2>/dev/null
  if [ -z "$s06precmd" ]; then
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(3)${CClear} ${CGreen}PRECMD=\"\"${CClear}"
  else
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(3)${CClear} ${CGreen}PRECMD=$s06precmd${CClear}"
  fi

  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Current custom values being used for Tailscale Connection commandline:${CClear}"

  if [ -z "$customcmdline" ]; then
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(4)${CClear} ${CGreen}CMD=\"\"${CClear}"
  else
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(4)${CClear} ${CGreen}CMD=\"$customcmdline\"${CClear}"
  fi

  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo ""
  read -p "Please enter item to modify (1-4)? (e=Exit): " EnterTimerLoop
  case $EnterTimerLoop in
    1)
      echo ""
      echo -e "${CClear}When entering a custom statement, please do not use quotes or other abnormal characters."
      echo -e "${CClear}Example: --tun=userspace-networking --state=/opt/var/tailscaled.state --statedir=/opt/var/lib/tailscale"
      echo ""
      read -p "Enter new ARGS= " EnterNewArgs
      tsoperatingmode="Custom"
      args=$EnterNewArgs
      args_regexp="$(printf '%s' "$args" | sed -e 's/[]\/$*.^|[]/\\&/g' | sed ':a;N;$!ba;s,\n,\\n,g')"
      sed -i "s/^ARGS=.*/ARGS=\"$args_regexp\"/" "/opt/etc/init.d/S06tailscaled"
      saveconfig
      timer=$timerloop
      restartts=1
    ;;

    2)
      echo ""
      echo -e "${CClear}When entering a custom statement, please do not use quotes or other abnormal characters."
      echo -e "${CClear}Example: nohup"
      echo ""
      read -p "Enter new PREARGS= " EnterNewPreArgs
      tsoperatingmode="Custom"
      preargs=$EnterNewPreArgs
      preargs_regexp="$(printf '%s' "$preargs" | sed -e 's/[]\/$*.^|[]/\\&/g' | sed ':a;N;$!ba;s,\n,\\n,g')"
      sed -i "s/^PREARGS=.*/PREARGS=\"$preargs_regexp\"/" "/opt/etc/init.d/S06tailscaled"
      saveconfig
      timer=$timerloop
      restartts=1

    ;;

    3)
      echo ""
      echo -e "${CClear}When entering a custom statement, please do not use quotes or other abnormal characters."
      echo -e "${CClear}Example: modprobe tun"
      echo ""
      read -p "Enter new PRECMD= " EnterNewPreCmd
      tsoperatingmode="Custom"
      precmd=$EnterNewPreCmd
      precmd_regexp="$(printf '%s' "$precmd" | sed -e 's/[]\/$*.^|[]/\\&/g' | sed ':a;N;$!ba;s,\n,\\n,g')"

      if ! grep -q -F "PRECMD=" /opt/etc/init.d/S06tailscaled; then
        sed '5 i PRECMD=\"'"$precmd_regexp"'\"' /opt/etc/init.d/S06tailscaled > /opt/etc/init.d/S06tailscaled2
        rm -f /opt/etc/init.d/S06tailscaled
        mv /opt/etc/init.d/S06tailscaled2 /opt/etc/init.d/S06tailscaled
        chmod 755 /opt/etc/init.d/S06tailscaled
      else
        sed -i "s/^PRECMD=.*/PRECMD=\"$precmd_regexp\"/" "/opt/etc/init.d/S06tailscaled"
      fi

      saveconfig
      timer=$timerloop
      restartts=1
    ;;

    4)
      echo ""
      echo -e "${CClear}When entering a custom statement, please do not use quotes or other abnormal characters."
      echo -e "${CClear}Example: --advertise-exit-node --advertise-routes=192.168.50.0/24,192.168.87.0/24"
      echo ""
      read -p "Enter new Commandline Options: " EnterNewCmdOptions
      tsoperatingmode="Custom"
      customcmdline=$EnterNewCmdOptions
      saveconfig
      timer=$timerloop
      restartts=1
    ;;

    *)

      if [ -f "/opt/bin/tailscale" ]; then
        if [ $restartts -eq 1 ]; then
          echo ""
          echo -e "Changing custom configuration options will require a restart of Tailscale. Restart now?"
          if promptyn "[y/n]: "
            then
            echo ""
            echo -e "\n${CGreen}Restarting Tailscale Service and Connection...${CClear}"
            echo ""

            tsdown
            stopts
            startts
            tsup

          fi
        fi
      fi
      echo ""
      echo -e "${CClear}[Exiting]"
      timer=$timerloop
      break
    ;;
  esac

done
}

# -------------------------------------------------------------------------------------------------------------------------
# operating mode lets the user choose between userspace and kernel modes of operation

operatingmode()
{
restartts=0
while true; do
  clear
  echo -e "${InvGreen} ${InvDkGray}${CWhite} Operating Mode Configuration                                                          ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Tailscale has 2 main modes of operation: 'Userspace' and 'Kernel' mode. By default,${CClear}"
  echo -e "${InvGreen} ${CClear} the installer will configure Tailscale to operate in 'Userspace' mode, but in the${CClear}"
  echo -e "${InvGreen} ${CClear} end, should not make much difference performance-wise based on the hardware available${CClear}"
  echo -e "${InvGreen} ${CClear} in our routers. More info below:${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} In general, kernel mode (and thus only Linux, for now) should be used for heavily${CClear}"
  echo -e "${InvGreen} ${CClear} used subnet routers, where 'heavy' is some combination of number of users, number${CClear}"
  echo -e "${InvGreen} ${CClear} of flows, bandwidth. The userspace mode should be more than sufficient for smaller${CClear}"
  echo -e "${InvGreen} ${CClear} numbers of users or low bandwidth. Even though Tailscale's userspace subnet routing${CClear}"
  echo -e "${InvGreen} ${CClear} is not as optimized as the Linux kernel, it makes up for it slightly in being able${CClear}"
  echo -e "${InvGreen} ${CClear} to avoid some context switches to the kernel.${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} A 3rd option (Custom) is also available, that allows you to enter your own custom${CClear}"
  echo -e "${InvGreen} ${CClear} settings for the ARGS, PREARGS, PRECMD and Tailscale Commandline. ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear}${CYellow} NOTE: ZeroScale will apply changes to modes after hitting the (e)xit key. If 'Custom'${CClear}"
  echo -e "${InvGreen} ${CClear}${CYellow} operating mode is chosen, you will be presented with the option to edit custom${CClear}"
  echo -e "${InvGreen} ${CClear}${CYellow} Tailscale settings after changes have been applied.${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} (Default = Userspace Mode)${CClear}"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Current: ${CGreen}$tsoperatingmode${CClear}"
  echo ""
  read -p "Please enter value (1=Userspace, 2=Kernel, 3=Custom)? (e=Exit/Apply Changes): " EnterOperatingMode
  case $EnterOperatingMode in
    1)
      if [ "$tsoperatingmode" != "Userspace" ]; then restartts=1; fi
      echo -e "\n${CGreen}[Userspace Operating Mode Selected]"
      sleep 1
      tsoperatingmode="Userspace"
      precmd=""
      args="--tun=userspace-networking --state=/opt/var/tailscaled.state --statedir=/opt/var/lib/tailscale"
      preargs="nohup"
      customcmdline=""
      saveconfig
      timer=$timerloop
    ;;

    2)
      if [ "$tsoperatingmode" != "Kernel" ]; then restartts=1; fi
      echo -e "\n${CGreen}[Kernel Operating Mode Selected]"
      sleep 1
      tsoperatingmode="Kernel"
      precmd="modprobe tun"
      args="--state=/opt/var/tailscaled.state --statedir=/opt/var/lib/tailscale"
      preargs="nohup"
      customcmdline=""
      saveconfig
      timer=$timerloop
    ;;

    3)
      if [ "$tsoperatingmode" != "Custom" ]; then restartts=1; fi
      echo -e "\n${CGreen}[Custom Operating Mode Selected]"
      sleep 1
      tsoperatingmode="Custom"
      precmd="modprobe tun"
      args="--state=/opt/var/tailscaled.state --statedir=/opt/var/lib/tailscale"
      preargs="nohup"
      if [ "$exitnode" -eq 1 ]; then exitnodecmd="--advertise-exit-node "; else exitnodecmd=""; fi
      if [ "$advroutes" -eq 1 ]; then advroutescmd="--advertise-routes=$routes"; else advroutescmd=""; fi
      customcmdline="$exitnodecmd$advroutescmd"
      saveconfig
      timer=$timerloop
    ;;

    *)

      if [ -f "/opt/bin/tailscale" ]; then
        if [ $restartts -eq 1 ]; then
          echo ""
          echo -e "Changing operating modes will require a restart of Tailscale. Restart now?"
          if promptyn "[y/n]: "
            then
            echo ""
            echo -e "\n${CGreen}Restarting Tailscale Service and Connection...${CClear}"
            echo ""

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
              applycustommode
            fi

            startts
            tsup

            if [ "$tsoperatingmode" == "Custom" ]; then
              echo ""
              echo -e "Would you like to customize your Tailscale settings now?"
              if promptyn "[y/n]: "
              then
                customconfig
              fi
            fi

          fi

          echo ""
          echo -e "${CClear}[Exiting]"
          timer=$timerloop
          break

        else

          echo ""
          echo -e "${CClear}[Exiting]"
          timer=$timerloop
          break

        fi
      else
        echo ""
        echo -e "${CClear}[Exiting]"
        timer=$timerloop
        break
      fi
    ;;

  esac

done
}

# -------------------------------------------------------------------------------------------------------------------------
# inject_s06tailscaled applies ZeroScale swapless logic and memory constraints

inject_s06tailscaled()
{
  # Capture old_overcommit for uninstall unconditionally to ensure safe restoration
  if [ -z "$old_overcommit" ]; then
    old_overcommit=$(cat /proc/sys/vm/overcommit_memory 2>/dev/null)
    saveconfig
  fi

  if [ -f "/opt/etc/init.d/S06tailscaled" ]; then
    # Clean old messy injections to prevent duplicates during update
    sed -i '/# TAILMON Zer.*: Dynamic Swapless/d' "/opt/etc/init.d/S06tailscaled"
    sed -i '/export GOMAXPROCS=1/d' "/opt/etc/init.d/S06tailscaled"
    sed -i '/export GOMEMLIMIT=20MiB/d' "/opt/etc/init.d/S06tailscaled"
    sed -i '/export GOGC=20/d' "/opt/etc/init.d/S06tailscaled"
    sed -i '/export GODEBUG=tlsmlkem=0/d' "/opt/etc/init.d/S06tailscaled"
    sed -i '/swap_total=$(free/d' "/opt/etc/init.d/S06tailscaled"
    sed -i '/swap_total=\$(free/d' "/opt/etc/init.d/S06tailscaled"
    sed -i '/if \[ "\$swap_total" = "0" \]; then/d' "/opt/etc/init.d/S06tailscaled"
    sed -i '/echo 0 > \/proc\/sys\/vm\/overcommit_memory/d' "/opt/etc/init.d/S06tailscaled"
    sed -i '/^fi$/d' "/opt/etc/init.d/S06tailscaled" 2>/dev/null || true
    
    # Clean new block logic
    sed -i '/# ZeroScale: Dynamic Swapless Block Start/,/# ZeroScale: Dynamic Swapless Block End/d' "/opt/etc/init.d/S06tailscaled"

    # Inject new logic
    awk -v old_oc="$old_overcommit" 'NR==2{
      print "# ZeroScale: Dynamic Swapless Block Start"
      print "export GODEBUG=tlsmlkem=0"
      print "swap_total=$(free | awk '"'"'/^Swap:/ {print $2}'"'"')"
      print "if [ \"$swap_total\" = \"0\" ]; then"
      print "    export GOMAXPROCS=1"
      print "    export GOMEMLIMIT=20MiB"
      print "    export GOGC=20"
      print "    echo 0 > /proc/sys/vm/overcommit_memory 2>/dev/null"
      print "else"
      print "    [ -n \"" old_oc "\" ] && echo \"" old_oc "\" > /proc/sys/vm/overcommit_memory 2>/dev/null"
      print "fi"
      print "# ZeroScale: Dynamic Swapless Block End"
    }1' "/opt/etc/init.d/S06tailscaled" > "/tmp/S06tailscaled.tmp" && mv "/tmp/S06tailscaled.tmp" "/opt/etc/init.d/S06tailscaled" && chmod +x "/opt/etc/init.d/S06tailscaled"
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# applyuserspacemode applies the standard settings for the Userspace operating mode

applyuserspacemode()
{
  sed -i "s/^ARGS=.*/ARGS=\"--tun=userspace-networking\ --state=\/opt\/var\/tailscaled.state\ --statedir=\/opt\/var\/lib\/tailscale\"/" "/opt/etc/init.d/S06tailscaled"
  sed -i "s/^PREARGS=.*/PREARGS=\"nohup\"/" "/opt/etc/init.d/S06tailscaled"
  sed -i -e '/^PRECMD=/d' "/opt/etc/init.d/S06tailscaled"

  #remove firewall-start entry if found
  if [ -f /jffs/scripts/firewall-start ]; then

    if grep -q -F "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi" /jffs/scripts/firewall-start; then
      sed -i -e '/tailscale down/d' /jffs/scripts/firewall-start
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: firewall-start entries removed." >> "$logfile"
    fi

  fi
  inject_s06tailscaled
  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Userspace Mode settings have been applied." >> "$logfile"
}

# -------------------------------------------------------------------------------------------------------------------------
