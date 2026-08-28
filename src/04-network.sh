# applykernelmode applies the standard settings for the Kernel operating mode

applykernelmode()
{
  if ! grep -q -F "PRECMD=" /opt/etc/init.d/S06tailscaled; then
    sed '5 i PRECMD=\"modprobe tun\"' /opt/etc/init.d/S06tailscaled > /opt/etc/init.d/S06tailscaled2
    rm -f /opt/etc/init.d/S06tailscaled
    mv /opt/etc/init.d/S06tailscaled2 /opt/etc/init.d/S06tailscaled
    chmod 755 /opt/etc/init.d/S06tailscaled
  else
    sed -i "s/^PRECMD=.*/PRECMD=\"modprobe tun\"/" "/opt/etc/init.d/S06tailscaled"
  fi
  sed -i "s/^ARGS=.*/ARGS=\"--state=\/opt\/var\/tailscaled.state\ --statedir=\/opt\/var\/lib\/tailscale\"/" "/opt/etc/init.d/S06tailscaled"
  sed -i "s/^PREARGS=.*/PREARGS=\"nohup\"/" "/opt/etc/init.d/S06tailscaled"

  #modify/create firewall-start
  if [ -f /jffs/scripts/firewall-start ]; then

    if ! grep -q -F "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi" /jffs/scripts/firewall-start; then
      echo "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi # Added by ZeroScale" >> /jffs/scripts/firewall-start
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: firewall-start entries created." >> "$logfile"
    fi

  else
    echo "#!/bin/sh" > /jffs/scripts/firewall-start
    echo "" >> /jffs/scripts/firewall-start
    echo "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi # Added by ZeroScale" >> /jffs/scripts/firewall-start
    chmod 0755 /jffs/scripts/firewall-start
  fi
  inject_s06tailscaled
  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Kernel Mode settings have been applied." >> "$logfile"
}

# -------------------------------------------------------------------------------------------------------------------------
# applycustommode applies the standard settings for the Custom operating mode which initially mimics Kernel mode

applycustommode()
{
  if ! grep -q -F "PRECMD=" /opt/etc/init.d/S06tailscaled; then
    sed '5 i PRECMD=\"modprobe tun\"' /opt/etc/init.d/S06tailscaled > /opt/etc/init.d/S06tailscaled2
    rm -f /opt/etc/init.d/S06tailscaled
    mv /opt/etc/init.d/S06tailscaled2 /opt/etc/init.d/S06tailscaled
    chmod 755 /opt/etc/init.d/S06tailscaled
  else
    sed -i "s/^PRECMD=.*/PRECMD=\"modprobe tun\"/" "/opt/etc/init.d/S06tailscaled"
  fi
  sed -i "s/^ARGS=.*/ARGS=\"--state=\/opt\/var\/tailscaled.state\ --statedir=\/opt\/var\/lib\/tailscale\"/" "/opt/etc/init.d/S06tailscaled"
  sed -i "s/^PREARGS=.*/PREARGS=\"nohup\"/" "/opt/etc/init.d/S06tailscaled"

  #modify/create firewall-start
  if [ -f /jffs/scripts/firewall-start ]; then

    if ! grep -q -F "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi" /jffs/scripts/firewall-start; then
      echo "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi # Added by ZeroScale" >> /jffs/scripts/firewall-start
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: firewall-start entries created." >> "$logfile"
    fi

  else
    echo "#!/bin/sh" > /jffs/scripts/firewall-start
    echo "" >> /jffs/scripts/firewall-start
    echo "if [ -x /opt/bin/tailscale ]; then tailscale down; tailscale up; fi # Added by ZeroScale" >> /jffs/scripts/firewall-start
    chmod 0755 /jffs/scripts/firewall-start
  fi

  if [ "$exitnode" -eq 1 ]; then exitnodecmd="--advertise-exit-node "; else exitnodecmd=""; fi
  if [ "$advroutes" -eq 1 ]; then advroutescmd="--advertise-routes=$routes"; else advroutescmd=""; fi
  customcmdline="$exitnodecmd$advroutescmd"
  saveconfig

  inject_s06tailscaled
  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Custom Mode settings have been applied." >> "$logfile"
}

# -------------------------------------------------------------------------------------------------------------------------
# applycustomchanges applies the custom settings for the Custom operating mode that may have been changed by the user

applycustomchanges()
{
  precmd_regexp="$(printf '%s' "$precmd" | sed -e 's/[]\/$*.^|[]/\\&/g' | sed ':a;N;$!ba;s,\n,\\n,g')"
  if ! grep -q -F "PRECMD=" /opt/etc/init.d/S06tailscaled; then
    sed '5 i PRECMD=\"'"$precmd_regexp"'\"' /opt/etc/init.d/S06tailscaled > /opt/etc/init.d/S06tailscaled2
    rm -f /opt/etc/init.d/S06tailscaled
    mv /opt/etc/init.d/S06tailscaled2 /opt/etc/init.d/S06tailscaled
    chmod 755 /opt/etc/init.d/S06tailscaled
  else
    sed -i "s/^PRECMD=.*/PRECMD=\"$precmd_regexp\"/" "/opt/etc/init.d/S06tailscaled"
  fi

  args_regexp="$(printf '%s' "$args" | sed -e 's/[]\/$*.^|[]/\\&/g' | sed ':a;N;$!ba;s,\n,\\n,g')"
  sed -i "s/^ARGS=.*/ARGS=\"$args_regexp\"/" "/opt/etc/init.d/S06tailscaled"

  preargs_regexp="$(printf '%s' "$preargs" | sed -e 's/[]\/$*.^|[]/\\&/g' | sed ':a;N;$!ba;s,\n,\\n,g')"
  sed -i "s/^PREARGS=.*/PREARGS=\"$preargs_regexp\"/" "/opt/etc/init.d/S06tailscaled"

  saveconfig
  timer=$timerloop
  restartts=1

  echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Custom Mode changes have been applied." >> "$logfile"
}

# -------------------------------------------------------------------------------------------------------------------------
# exitnodets provide a menu interface to allow for selection of router becoming an exitnode

exitnodets()
{
  clear
  exitnodedisp=$(booleantoyesno "$exitnode")
  oldexitnode=$exitnode

  echo -e "${InvGreen} ${InvDkGray}${CWhite} Configure Router as Exit Node                                                         ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} A Tailscale Exit Node is a feature that lets you route all internet traffic,"
  echo -e "${InvGreen} ${CClear} including internet traffic from non-Tailscale devices, through a specific device"
  echo -e "${InvGreen} ${CClear} on your Tailscale network (known as a tailnet). The device routing your traffic"
  echo -e "${InvGreen} ${CClear} (this router) is called an 'exit node'. Please indicate below if you want to"
  echo -e "${InvGreen} ${CClear} enable this feature"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} (Default = No)"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Current: ${CGreen}$exitnodedisp${CClear}"
  echo ""
  echo -e "Configure Router as Exit Node?"
  if promptyn "[y/n]: "
    then
      exitnode=1
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Device has been configured as Exit Node." >> "$logfile"
    else
      exitnode=0
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Exit Node configuration has been disabled." >> "$logfile"
  fi
  saveconfig
  timer=$timerloop

  if [ -f "/opt/bin/tailscale" ] && [ "$exitnode" -ne "$oldexitnode" ]; then
    echo ""
    echo -e "\nChanging exit node configuration options will require a restart of Tailscale. Restart now?"
    if promptyn "[y/n]: "
      then
      echo ""
      echo -e "\n${CGreen}Restarting Tailscale Service and Connection...${CClear}"
      echo ""

      tsdown
      stopts
      setipforwarding
      startts
      tsup

    fi
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# advroutests provide a menu interface to allow for entry of advertised routes

advroutests()
{
  clear
  advroutesdisp=$(booleantoyesno "$advroutes")
  oldadvroutes=$advroutes
  oldroutes=$routes

  echo -e "${InvGreen} ${InvDkGray}${CWhite} Advertise Routes on this Router                                                       ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Tailscale can act as a 'subnet router' that allow you to access multiple devices"
  echo -e "${InvGreen} ${CClear} located on your particular subnet through Tailscale. Subnet routers act as a"
  echo -e "${InvGreen} ${CClear} gateway, relaying traffic from your Tailscale network onto your physical subnet."
  echo -e "${InvGreen} ${CClear} If you need access to other devices, such as NAS, routers, computers, printers,"
  echo -e "${InvGreen} ${CClear} etc. without the need to install Tailscale software on them, it would be"
  echo -e "${InvGreen} ${CClear} recommended to enable this feature.  Please indicate your choice below."
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} (Default = Yes)"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Current: ${CGreen}$advroutesdisp${CClear}"
  echo -e "${InvGreen} ${CClear} ROUTE(S): ${CGreen}$routes${CClear}"
  echo ""
  echo -e "Advertise Routes?"
  if promptyn "[y/n]: "
    then
      echo ""
      echo ""
      echo -e "${InvGreen} ${InvDkGray}${CWhite} Advertise Routes on this Router                                                       ${CClear}"
      echo -e "${InvGreen} ${CClear}"
      echo -e "${InvGreen} ${CClear} Please indicate what subnet you want to advertise to your Tailscale network."
      echo -e "${InvGreen} ${CClear} Typically, you would enter the current subnet of what your router is currently"
      echo -e "${InvGreen} ${CClear} configured for, ex: 192.168.50.0/24. Should you want to advertise multiple"
      echo -e "${InvGreen} ${CClear} subnets that are accessible by your router, comma-delimit them in this way:"
      echo -e "${InvGreen} ${CClear} 192.168.50.0/24,192.168.87.0/24,10.0.100.0/16"
      echo -e "${InvGreen} ${CClear}"
      echo -en "${InvGreen} ${CClear} (Default = "; echo -e "$(nvram get lan_ipaddr | cut -d"." -f1-3).0/24)"
      echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
      echo  ""
      read -p "Please enter valid IP4 subnet range? (e=Exit): " routeinput

      # exit with no changes
      if [ "$routeinput" == "e" ]; then
        echo -e "\n[Exiting]"
        sleep 1
        return
      fi

      if [ -z "$routeinput" ]; then
        routes=$(nvram get lan_ipaddr | cut -d"." -f1-3).0/24
      else
        routes=$routeinput
      fi
      advroutes=1
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Advertised routes enabled with routes=$routes." >> "$logfile"
  else
    advroutes=0
    routes=""
    echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Advertised routes disabled." >> "$logfile"
  fi
  saveconfig
  timer=$timerloop

  if [ -f "/opt/bin/tailscale" ] && { [ "$advroutes" -ne "$oldadvroutes" ] || [ "$routes" != "$oldroutes" ]; }; then
    echo ""
    echo -e "\nChanging advertised routes configuration options will require a restart of Tailscale. Restart now?"
    if promptyn "[y/n]: "
      then
      echo ""
      echo -e "\n${CGreen}Restarting Tailscale Service and Connection...${CClear}"
      echo ""

      tsdown
      stopts
      setipforwarding
      startts
      tsup

    fi
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# accroutests provide a menu interface to allow for entry to accept linux routes

accroutests()
{
  clear
  accroutesdisp=$(booleantoyesno "$accroutes")
  oldaccroutes=$accroutes

  echo -e "${InvGreen} ${InvDkGray}${CWhite} Accept Site-to-Site Functionality on this Router                                      ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Clients on Android, iOS, macOS, tvOS, and Windows automatically pick up your new"
  echo -e "${InvGreen} ${CClear} subnet routes. Only Linux clients using the --accept-routes flag discover the new"
  echo -e "${InvGreen} ${CClear} routes automatically because the default is to use only the Tailscale IP addresses."
  echo -e "${InvGreen} ${CClear} This option provides for the basic functionality to allow for site-to-site routing"
  echo -e "${InvGreen} ${CClear} and communication between networks. Advanced troubleshooting skills may be required"
  echo -e "${InvGreen} ${CClear} when enabling this option. Please indicate 'y' or 'n' below."
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} (Default = No)"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Current: ${CGreen}$accroutesdisp${CClear}"
  echo ""
  echo -e "Accept Routes?"
  if promptyn "[y/n]: "
    then
      accroutes=1
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Accepted Linux routes enabled." >> "$logfile"
    else
      accroutes=0
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Accepted Linux routes disabled." >> "$logfile"
  fi
  saveconfig
  timer=$timerloop

  if [ -f "/opt/bin/tailscale" ] && [ "$accroutes" -ne "$oldaccroutes" ]; then
    echo ""
    echo -e "\nChanging routing configuration options will require a restart of Tailscale. Restart now?"
    if promptyn "[y/n]: "
      then
      echo ""
      echo -e "\n${CGreen}Restarting Tailscale Service and Connection...${CClear}"
      echo ""

      tsdown
      stopts
      setipforwarding
      startts
      tsresetc
      tsup
      sleep 3

    fi
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# sshts provides a menu interface to toggle the Tailscale SSH server (--ssh) on this router

sshts()
{
  clear
  sshenabledisp=$(booleantoyesno "$sshenable")
  oldsshenable=$sshenable

  echo -e "${InvGreen} ${InvDkGray}${CWhite} Enable Tailscale SSH Server on this Router                                            ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Enabling this option adds the ${CGreen}--ssh${CClear} flag to the 'tailscale up' command, allowing"
  echo -e "${InvGreen} ${CClear} you to SSH into this router from other devices on your tailnet using your Tailscale-"
  echo -e "${InvGreen} ${CClear} managed identity (subject to your tailnet ACLs). Because --ssh is a non-default"
  echo -e "${InvGreen} ${CClear} setting, it must be present on every 'tailscale up' that ZeroScale issues, otherwise"
  echo -e "${InvGreen} ${CClear} Tailscale refuses the command and the connection fails. Enabling this toggle makes"
  echo -e "${InvGreen} ${CClear} ZeroScale include it every time. Please indicate 'y' or 'n' below."
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} (Default = No)"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Current: ${CGreen}$sshenabledisp${CClear}"
  echo ""
  echo -e "Enable Tailscale SSH?"
  if promptyn "[y/n]: "
    then
      sshenable=1
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale SSH enabled." >> "$logfile"
    else
      sshenable=0
      echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Tailscale SSH disabled." >> "$logfile"
  fi
  saveconfig
  timer=$timerloop

  if [ -f "/opt/bin/tailscale" ] && [ "$sshenable" -ne "$oldsshenable" ]; then
    echo ""
    echo -e "\nChanging the SSH setting will require a restart of Tailscale. Restart now?"
    if promptyn "[y/n]: "
      then
      echo ""
      echo -e "\n${CGreen}Restarting Tailscale Service and Connection...${CClear}"
      echo ""

      tsdown
      stopts
      setipforwarding
      startts
      tsresetc
      tsup
      sleep 3

    fi
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# amtmevents lets you pick success or failure amtm email notification selections

amtmevents()
{
while true; do
  clear
  echo -e "${InvGreen} ${InvDkGray}${CWhite} AMTM Email Notifications                                                              ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Please indicate if you would like ZeroScale to send you email notifications for${CClear}"
  echo -e "${InvGreen} ${CClear} Tailscale service/connection failures, or successes, or both?  PLEASE NOTE: This${CClear}"
  echo -e "${InvGreen} ${CClear} does require that AMTM email has been set up successfully under AMTM -> em (email${CClear}"
  echo -e "${InvGreen} ${CClear} settings). Once you are able to send and receive test emails from AMTM, you may${CClear}"
  echo -e "${InvGreen} ${CClear} use this functionality in ZeroScale. Additionally, this functionality will download${CClear}"
  echo -e "${InvGreen} ${CClear} an AMTM email interface library courtesey of @Martinsky, and will be located${CClear}"
  echo -e "${InvGreen} ${CClear} under a new common shared library folder called: /jffs/addons/shared-libs.${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Secondarily, you can choose to rate limit the rate at which emails are sent to${CClear}"
  echo -e "${InvGreen} ${CClear} your email account per hour. (0=Disabled, 1-9999)${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Use the corresponding ${CGreen}()${CClear} key to enable/disable email event notifications:${CClear}"
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"

  if [ "$amtmemailsuccess" == "1" ]; then amtmemailsuccessdisp="${CGreen}Y${CCyan}"; else amtmemailsuccess=0; amtmemailsuccessdisp="${CRed}N${CCyan}"; fi
  if [ "$amtmemailfailure" == "1" ]; then amtmemailfailuredisp="${CGreen}Y${CCyan}"; else amtmemailfailure=0; amtmemailfailuredisp="${CRed}N${CCyan}"; fi
  if [ "$ratelimit" = "0" ]; then ratelimitdisp="Disabled"; else ratelimitdisp=$ratelimit; fi
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}Tailscale Success Event Notifications${CClear} ${CGreen}(1) -${CClear} $amtmemailsuccessdisp${CClear}"
  echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}Tailscale Failure Event Notifications${CClear} ${CGreen}(2) -${CClear} $amtmemailfailuredisp${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}Tailscale Email Rate Limit (per hour)${CClear} ${CGreen}(r) - $ratelimitdisp${CClear}"
  echo ""
  read -p "Please select? (1-2, r=Set Email Rate Limit, t=Test Email, e=Exit): " SelectSlot
    case $SelectSlot in
      1) if [ "$amtmemailsuccess" == "0" ]; then amtmemailsuccess=1; amtmemailsuccessdisp="${CGreen}Y${CCyan}"; elif [ "$amtmemailsuccess" == "1" ]; then amtmemailsuccess=0; amtmemailsuccessdisp="${CRed}N${CCyan}"; saveconfig; fi;;
      2) if [ "$amtmemailfailure" == "0" ]; then amtmemailfailure=1; amtmemailfailuredisp="${CGreen}Y${CCyan}"; elif [ "$amtmemailfailure" == "1" ]; then amtmemailfailure=0; amtmemailfailuredisp="${CRed}N${CCyan}"; saveconfig; fi;;
      [Tt])
         if [ -f "$CUSTOM_EMAIL_LIBFile" ]
           then
           . "$CUSTOM_EMAIL_LIBFile"

           if [ -z "${CEM_LIB_VERSION:+xSETx}" ] || \
             _CheckLibraryUpdates_CEM_ "$CUSTOM_EMAIL_LIBDir" quiet
             then
               _DownloadCEMLibraryFile_ "update"
           fi
           else
             _DownloadCEMLibraryFile_ "install"
         fi

         cemIsFormatHTML=true
         cemIsVerboseMode=true  ## true OR false ##
         emailBodyTitle="Testing Email Notification"
         emailSubject="TEST: ZeroScale Email Notification"
         tmpEMailBodyFile="/tmp/var/tmp/tmpEMailBody_${scriptFileNTag}.$$.TXT"

         {
          printf "This is a <b>TEST</b> to check & verify if sending email notifications is working well from <b>ZeroScale</b>.\n"
         } > "$tmpEMailBodyFile"

         _SendEMailNotification_ "ZeroScale v$version" "$emailSubject" "$tmpEMailBodyFile" "$emailBodyTitle"

         echo ""
         echo ""
         read -rsp $'Press any key to acknowledge...\n' -n1 key
         ;;

      [Rr])
         echo ""
         read -p "Please enter new Email Rate Limit (per hour)? (0=disabled, 1-9999, e=Exit): " newratelimit
         if [ "$newratelimit" = "e" ]
         then
             echo -e "\n[Exiting]"; sleep 2
         elif [ "$newratelimit" -ge 0 ] 2>/dev/null && [ "$newratelimit" -le 9999 ] 2>/dev/null; then
             ratelimit="$newratelimit"
             echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: New Email Rate Limit entered (per hour): $ratelimit" >> "$logfile"
             saveconfig
         else
             previousValue="$ratelimit"
             ratelimit="${ratelimit:=0}"
             [ "$ratelimit" != "$previousValue" ] && \
             echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: New Email Rate Limit entered (per hour): $ratelimit" >> "$logfile"
             saveconfig
         fi
         ;;

      [Ee])
         saveconfig
         echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: AMTM Email notification configuration saved" >> "$logfile"
         timer=$timerloop
         break;;
    esac
done
}

# -------------------------------------------------------------------------------------------------------------------------

########################################################################
# AMTM Email Notification Functionality generously donated by @Martinski!
#
# Creation Date: 2020-Jun-11 [Martinski W.]
# Last Modified: 2024-Feb-07 [Martinski W.]
# Modified for ZeroScale Purposes [Viktor Jaep]
########################################################################

#-----------------------------------------------------------#
_DownloadCEMLibraryFile_()
{
   local msgStr  retCode
   case "$1" in
        update) msgStr="Updating" ;;
       install) msgStr="Installing" ;;
             *) return 1 ;;
   esac

   printf "\33[2K\r"
   printf "${CGreen}\r[INFO: ${msgStr} the shared AMTM email library script file to support email notifications...]${CClear}"
   echo -e "$(date +'%b %d %Y %X') $(nvram get lan_hostname) ZEROSCALE[$$] - INFO: ${msgStr} the shared AMTM email library script file to support email notifications..." >> "$logfile"

   mkdir -m 755 -p "$CUSTOM_EMAIL_LIBDir"
   curl -kLSs --retry 3 --retry-delay 5 --retry-connrefused \
   "${CEM_LIB_URL}/$CUSTOM_EMAIL_LIBName" -o "$CUSTOM_EMAIL_LIBFile"
   curlCode="$?"

   if [ "$curlCode" -eq 0 ] && [ -f "$CUSTOM_EMAIL_LIBFile" ]
   then
       retCode=0
       chmod 755 "$CUSTOM_EMAIL_LIBFile"
       . "$CUSTOM_EMAIL_LIBFile"
       #printf "\nDone.\n"
   else
       retCode=1
       printf "\33[2K\r"
       printf \"%s\" "${CRed}\r[ERROR: Unable to download the shared library script file ($CUSTOM_EMAIL_LIBName).]${CClear}"
       echo -e "$(date +'%b %d %Y %X') $(nvram get lan_hostname) ZEROSCALE[$$] - **ERROR**: Unable to download the shared AMTM email library script file [$CUSTOM_EMAIL_LIBName]." >> "$logfile"
   fi
   return "$retCode"
}

#-----------------------------------------------------------#
# ARG1: The email name/alias to be used as "FROM_NAME"
# ARG2: The email Subject string.
# ARG3: Full path of file containing the email Body text.
# ARG4: The email Body Title string [OPTIONAL].
#-----------------------------------------------------------#
_SendEMailNotification_()
{

   if [ -z "${amtmIsEMailConfigFileEnabled:+xSETx}" ]
   then
       printf "\33[2K\r"
       printf \"%s\" "${CRed}\r[ERROR: Email library script ($CUSTOM_EMAIL_LIBFile) *NOT* FOUND.]${CClear}"
       sleep 5
       echo -e "$(date +'%b %d %Y %X') $(nvram get lan_hostname) ZEROSCALE[$$] - **ERROR**: Email library script [$CUSTOM_EMAIL_LIBFile] *NOT* FOUND." >> "$logfile"
       return 1
   fi

   if [ $# -lt 3 ] || [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]
   then
       printf "\33[2K\r"
       printf "${CRed}\r[ERROR: INSUFFICIENT email parameters]${CClear}"
       sleep 5
       echo -e "$(date +'%b %d %Y %X') $(nvram get lan_hostname) ZEROSCALE[$$] - **ERROR**: INSUFFICIENT email parameters." >> "$logfile"
       return 1
   fi
   local retCode  emailBodyTitleStr=""

   [ $# -gt 3 ] && [ -n "$4" ] && emailBodyTitleStr="$4"

   FROM_NAME="$1"
   _SendEMailNotification_CEM_ "$2" "-F=$3" "$emailBodyTitleStr"
   retCode="$?"

   if [ "$retCode" -eq 0 ]
   then
     printf "\33[2K\r"
     printf \"%s\" "${CGreen}\r[Email notification was sent successfully ($2)]${CClear}"
     echo -e "$(date +'%b %d %Y %X') $(nvram get lan_hostname) ZEROSCALE[$$] - INFO: Email notification was sent successfully [$2]" >> "$logfile"
     sleep 5
   else
     printf "\33[2K\r"
     printf \"%s\" "${CRed}\r[ERROR: Failure to send email notification (Error Code: $retCode - $2).]${CClear}"
     echo -e "$(date +'%b %d %Y %X') $(nvram get lan_hostname) ZEROSCALE[$$] - **ERROR**: Failure to send email notification [$2]" >> "$logfile"
     sleep 5
   fi

   return "$retCode"
}

# -------------------------------------------------------------------------------------------------------------------------
# sendmessage is a function that sends an AMTM email based on activity within ZeroScale
# $1 = Success/Failure 0/1
# $2 = Component
# $3 = VPN Slot

sendmessage()
{

#If AMTM email functionality is disabled, return back to the function call
if [ "$amtmemailsuccess" == "0" ] && [ "$amtmemailfailure" == "0" ]; then
  return
fi

  #Load, install or update the shared AMTM Email integration library
  if [ -f "$CUSTOM_EMAIL_LIBFile" ]
  then
    . "$CUSTOM_EMAIL_LIBFile"

    if [ -z "${CEM_LIB_VERSION:+xSETx}" ] || \
      _CheckLibraryUpdates_CEM_ "$CUSTOM_EMAIL_LIBDir" quiet
    then
      _DownloadCEMLibraryFile_ "update"
    fi
  else
      _DownloadCEMLibraryFile_ "install"
  fi

  cemIsFormatHTML=true
  cemIsVerboseMode=false
  tmpEMailBodyFile="/tmp/var/tmp/tmpEMailBody_${scriptFileNTag}.$$.TXT"

  ratelimiter
  emaillimit="$?"
  if [ "$emaillimit" -eq 0 ]
    then

    #Pick the scenario and send email
    if [ "$1" == "1" ] && [ "$amtmemailfailure" == "1" ]; then
      if [ "$2" == "Tailscale Service settings out-of-sync" ]; then
        emailSubject="ALERT: Tailscale Service settings out-of-sync"
        emailBodyTitle="ALERT: Tailscale Service settings out-of-sync"
        {
        printf "<b>Date/Time:</b> $(date +'%b %d %Y %X')\n"
        printf "\n"
        printf "<b>ALERT: ZeroScale</b> is currently recovering from out-of-sync settings issues! ZeroScale has detected\n"
        printf "that the Tailscale service settings are not in sync with the ZeroScale config. This could be due to a\n"
        printf "Tailscale update. ZeroScale has fixed the settings and restarted the Tailscale service/connection.\n"
        printf "\n"
        } > "$tmpEMailBodyFile"
      elif [ "$2" == "Tailscale Service Restarted" ]; then
        emailSubject="FAILURE: Tailscale Service Restarted"
        emailBodyTitle="FAILURE: Tailscale Service Restarted"
        {
        printf "<b>Date/Time:</b> $(date +'%b %d %Y %X')\n"
        printf "\n"
        printf "<b>FAILURE: ZeroScale</b> has detected that the Tailscale service was dead and not connected. ZeroScale.\n"
        printf "has reset the service, and reestablished a connection to your Tailnet. Please investigate if this\n"
        printf "behavior continues to persist.\n"
        printf "\n"
        } > "$tmpEMailBodyFile"
      elif [ "$2" == "Router has been restarted" ]; then
        emailSubject="WARNING: Router Has Unexpectedly Restarted"
        emailBodyTitle="WARNING: Router Has Unexpectedly Restarted"
        {
        printf "<b>Date/Time:</b> $(date +'%b %d %Y %X')\n"
        printf "\n"
        printf "<b>WARNING: ZeroScale</b> has detected that the router may have rebooted or was restarted. ZeroScale.\n"
        printf "has reset the service, and reestablished a connection to your Tailnet. Please investigate if this\n"
        printf "behavior continues to persist.\n"
        printf "\n"
        } > "$tmpEMailBodyFile"
      # Rung: added request email functionality
      elif [ "$2" == "ZeroScale email requested" ]; then
        emailSubject="WARNING: Router Has Unexpectedly Restarted"
        emailBodyTitle="WARNING: Router Has Unexpectedly Restarted"
        {
        printf "<b>Date/Time:</b> $(date +'%b %d %Y %X')\n"
        printf "\n"
        printf "<b>WARNING: ZeroScale</b> has been requested to send this email from the services-start script.\n"
        printf "If no additional email is received, this means that ZeroScale has failed to start for some reason.\n"
        printf "Please investigate if this behavior continues to persist.\n"
        printf "\n"
        } > "$tmpEMailBodyFile"
      elif [ "$2" == "Unable to reach ZeroScale repository" ]; then
        emailSubject="WARNING: Router unable to reach ZeroScale Repository"
        emailBodyTitle="WARNING: Router unable to reach ZeroScale Repository"
        {
        printf "<b>Date/Time:</b> $(date +'%b %d %Y %X')\n"
        printf "\n"
        printf "<b>WARNING: ZeroScale</b> is unable to reach the ZeroScale repository on GitHub in order to perform\n"
        printf "an autoupdate function. Please check your internet connectivity or any blocking tools in place.\n"
        printf "Please investigate if this behavior continues to persist.\n"
        printf "\n"
        } > "$tmpEMailBodyFile"
      elif [ "$2" == "Unable to reach Tailscale repository" ]; then
        emailSubject="WARNING: Router unable to reach Tailscale Repository"
        emailBodyTitle="WARNING: Router unable to reach Tailscale Repository"
        {
        printf "<b>Date/Time:</b> $(date +'%b %d %Y %X')\n"
        printf "\n"
        printf "<b>WARNING: ZeroScale</b> is unable to reach the Tailscale repository in order to perform an\n"
        printf "autoupdate. Please check your internet connectivity or any blocking tools in place.\n"
        printf "Please investigate if this behavior continues to persist.\n"
        printf "\n"
        } > "$tmpEMailBodyFile"
      fi
      _SendEMailNotification_ "ZeroScale v$version" "$emailSubject" "$tmpEMailBodyFile" "$emailBodyTitle"
    fi

    if [ "$1" == "0" ] && [ "$amtmemailsuccess" == "1" ]; then
      if [ "$2" == "Tailscale Successfully Updated" ]; then
        emailSubject="SUCCESS: Tailscale Binary was successfully updated via autoupdate"
        emailBodyTitle="SUCCESS: Tailscale Binary was successfully updated via autoupdate from v$3 to v$4"
        {
        printf "<b>Date/Time:</b> $(date +'%b %d %Y %X')\n"
        printf "\n"
        printf "<b>SUCCESS: ZeroScale</b> has successfully autoupdated the Tailscale Binary to the latest version.\n"
        printf "\n"
        } > "$tmpEMailBodyFile"
      elif [ "$2" == "ZeroScale Script Successfully Updated" ]; then
        if [ "$5" == "lt" ]; then
          emailSubject="SUCCESS: ZeroScale was corrected to the configured track"
          emailBodyTitle="SUCCESS: ZeroScale v$3 did not match the configured track and was corrected to v$4"
          {
          printf "<b>Date/Time:</b> $(date +'%b %d %Y %X')\n"
          printf "\n"
          printf \"%s\" "<b>SUCCESS: ZeroScale</b> detected that the installed script (v$3) did not match the configured\n"
          printf \"%s\" "update track and has corrected it to the appropriate version (v$4) via autoupdate.\n"
          printf "\n"
          } > "$tmpEMailBodyFile"
        else
          emailSubject="SUCCESS: ZeroScale was successfully updated via autoupdate"
          emailBodyTitle="SUCCESS: ZeroScale was successfully updated via autoupdate from v$3 to v$4"
          {
          printf "<b>Date/Time:</b> $(date +'%b %d %Y %X')\n"
          printf "\n"
          printf "<b>SUCCESS: ZeroScale</b> was successfully updated to the latest version via autoupdate.\n"
          printf "\n"
          } > "$tmpEMailBodyFile"
        fi
      fi
      _SendEMailNotification_ "ZeroScale v$version" "$emailSubject" "$tmpEMailBodyFile" "$emailBodyTitle"
    fi

  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# Function to keep track of emails sent, and determine if they need to be rate-limited
ratelimiter()
{

#if rate limiting is disabled, exit right away
if [ "$ratelimit" = "0" ]; then
  return 0
fi

#Make sure log file exists
touch "$tmemails"

#check current time and 1h into the past
current_time=$(date +%s)
cutoff_time=$((current_time - 3600))

#create a temp file where current data will get moved over into that is less than 1hr old
tmemailstemp="${tmemails}.tmp"
awk -v cutoff="$cutoff_time" '$1 > cutoff' "$tmemails" > "$tmemailstemp"

#check to see how many emails have been sent in the last hour
recent_email_count=$(wc -l < "$tmemailstemp" | tr -d ' ')

printf "\33[2K\r"
printf \"%s\" "${CGreen}\r[Checking email rate limit... $recent_email_count/$ratelimit emails sent within the last hour]"
sleep 2

#logic to determine if rate limit has been hit
if [ "$recent_email_count" -ge "$ratelimit" ]
  then
    printf "\33[2K\r"
    printf "${CGreen}\r[Rate limit exceeded. Emails will be prevented from sending]"
    echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Email Rate limit exceeded ($ratelimit). Emails will be prevented from sending." >> "$logfile"
    sleep 2
    mv "$tmemailstemp" "$tmemails"
    return 1
  else
    printf "\33[2K\r"
    printf "${CGreen}\r[Rate within limits. Proceeding to send email]"
    sleep 1
    echo "$current_time" >> "$tmemailstemp"
    mv "$tmemailstemp" "$tmemails"
    return 0
fi

}

# -------------------------------------------------------------------------------------------------------------------------
# installdependencies checks for existence of entware, and if so proceed and install the packages, then run zeroscale -config

installdependencies()
{
  clear
  if [ -f "/opt/bin/timeout" ] && [ -f "/opt/sbin/screen" ]; then
    vconfig
  else
    clear
    echo -e "${InvGreen} ${InvDkGray}${CWhite} Install Dependencies                                                                  ${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} Missing dependencies required by ZeroScale will be installed during this process."
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo ""
    echo -e "ZeroScale has some dependencies in order to function correctly, namely, CoreUtils-Timeout"
    echo -e "and the Screen utility. These utilities require you to have Entware already installed"
    echo -e "using the AMTM tool. If Entware is present, the Timeout and Screen utilities will"
    echo -e "automatically be downloaded and installed during this process."
    echo ""
    echo -e "${CGreen}CoreUtils-Timeout${CClear} is a utility that provides more stability for certain routers (like"
    echo -e "the RT-AC86U) which has a tendency to randomly hang scripts running on this router model."
    echo ""
    echo -e "${CGreen}Screen${CClear} is a utility that allows you to run SSH scripts in a standalone environment"
    echo -e "directly on the router itself, instead of running your commands or a script from a network-"
    echo -e "attached SSH client. This can provide greater stability due to it running on the router"
    echo -e "itself."
    echo ""
    [ -z "$($timeoutcmd$timeoutsec nvram get odmpid)" ] && RouterModel="$($timeoutcmd$timeoutsec nvram get productid)" || RouterModel="$($timeoutcmd$timeoutsec nvram get odmpid)" # Thanks @thelonelycoder for this logic
    echo -e "Your router model is: ${CGreen}$RouterModel${CClear}"
    echo ""
    echo -e "Ready to install?"
    if promptyn "[y/n]: "
      then
        if [ -d "/opt" ]; then # Does entware exist? If yes proceed, if no error out.
          echo ""
          echo -e "\n${CClear}Updating Entware Packages..."
          echo ""
          opkg update
          echo ""
          echo -e "Installing Entware ${CGreen}CoreUtils-Timeout${CClear} Package...${CClear}"
          echo ""
          opkg install coreutils-timeout
          echo ""
          echo -e "Installing Entware ${CGreen}Screen${CClear} Package...${CClear}"
          echo ""
          opkg install screen
          echo ""
          echo -e "Install completed..."
          echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Entware dependencies installed." >> "$logfile"
          echo ""
          read -rsp $'Press any key to continue...\n' -n1 key
          echo ""
          echo -e "Executing Configuration Utility..."
          sleep 1
          vconfig
        else
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
      echo -e "\n${CClear}[Exiting]"
      echo ""
      sleep 1
      exit 0
    fi
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# reinstalldependencies force re-installs the entware packages

reinstalldependencies()
{
  clear
  echo -e "${InvGreen} ${InvDkGray}${CWhite} Re-install Dependencies                                                               ${CClear}"
  echo -e "${InvGreen} ${CClear}"
  echo -e "${InvGreen} ${CClear} Missing dependencies required by ZeroScale will be re-installed during this process."
  echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
  echo ""
  echo -e "Would you like to re-install the CoreUtils-Timeout and the Screen utility? These"
  echo -e "utilities require you to have Entware already installed using the AMTM tool. If Entware"
  echo -e "is present, the Timeout and Screen utilities will be uninstalled, downloaded and re-"
  echo -e "installed during this setup process..."
  echo ""
  echo -e "${CGreen}CoreUtils-Timeout${CClear} is a utility that provides more stability for certain routers (like"
  echo -e "the RT-AC86U) which has a tendency to randomly hang scripts running on this router"
  echo -e "model."
  echo ""
  echo -e "${CGreen}Screen${CClear} is a utility that allows you to run SSH scripts in a standalone environment"
  echo -e "directly on the router itself, instead of running your commands or a script from a"
  echo -e "network-attached SSH client. This can provide greater stability due to it running on"
  echo -e "the router itself."
  echo ""
  [ -z "$($timeoutcmd$timeoutsec nvram get odmpid)" ] && RouterModel="$($timeoutcmd$timeoutsec nvram get productid)" || RouterModel="$($timeoutcmd$timeoutsec nvram get odmpid)" # Thanks @thelonelycoder for this logic
  echo -e "Your router model is: ${CGreen}$RouterModel${CClear}"
  echo ""
  echo -e "Force Re-install?"
  if promptyn "[y/n]: "
    then
      if [ -d "/opt" ]; then # Does entware exist? If yes proceed, if no error out.
        echo ""
        echo -e "\nUpdating Entware Packages..."
        echo ""
        opkg update
        echo ""
        echo -e "Force Re-installing Entware ${CGreen}CoreUtils-Timeout${CClear} Package..."
        echo ""
        opkg install --force-reinstall coreutils-timeout
        echo ""
        echo -e "Force Re-installing Entware ${CGreen}Screen${CClear} Package..."
        echo ""
        opkg install --force-reinstall screen
        echo ""
        echo -e "Re-install completed..."
        echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Entware dependencies re-installed." >> "$logfile"
        echo ""
        read -rsp $'Press any key to continue...\n' -n1 key
      else
        clear
        echo -e "${CRed}ERROR: Entware was not found on this router...${CClear}"
        echo -e "Please install Entware using the AMTM utility before proceeding..."
        echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - ERROR: Entware was not found installed on router. Please investigate." >> "$logfile"
        echo ""
        read -rsp $'Press any key to continue...\n' -n1 key
        exit 1
      fi
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# vsetup provide a menu interface to allow for initial component installs, uninstall, etc.

vsetup()
{
  if [ ! -f "/opt/bin/timeout" ] || [ ! -f "/opt/sbin/screen" ]; then
    installdependencies
  fi

  # Grab the ZeroScale config file and read it in
  if [ -f "$config" ]; then
    source "$config"
  else
    initialsetup
  fi

  while true; do

    clear # Initial Setup
    if [ -f "$config" ]; then
      source "$config"
    else
      saveconfig
    fi

    if tailscaleready; then tsinstalleddisp="Installed"; else tsinstalleddisp="Not Installed"; fi
    if [ "$exitnode" -eq 0 ]; then exitnodedisp="No"; elif [ "$exitnode" -eq 1 ]; then exitnodedisp="Yes"; fi
    if [ "$advroutes" -eq 0 ]; then advroutesdisp="No"; elif [ "$advroutes" -eq 1 ]; then advroutesdisp="Yes ($routes)"; fi
    if [ "$accroutes" -eq 0 ]; then accroutesdisp="No"; elif [ "$accroutes" -eq 1 ]; then accroutesdisp="Yes"; fi
    if [ "$sshenable" -eq 0 ]; then sshenabledisp="No"; elif [ "$sshenable" -eq 1 ]; then sshenabledisp="Yes"; fi
    tsver=$(tailscale version | awk 'NR==1 {print $1}') >/dev/null 2>&1
    if [ -z "$tsver" ]; then tsver="0.00"; fi

    echo -e "${InvGreen} ${InvDkGray}${CWhite} ZeroScale Main Setup and Configuration Menu                                             ${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} Please choose from the various options below, which allow you to perform high level${CClear}"
    echo -e "${InvGreen} ${CClear} actions in the management of the ZeroScale script.${CClear}"
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 1)${CClear} : Install Tailscale Entware Package(s)         : ${CGreen}$tsinstalleddisp${CClear}"

    printf "\33[2K\r"
    printf "${CGreen}\r[Checking Services...Stand By]"

    /opt/etc/init.d/S06tailscaled check >/dev/null 2>&1
    tsservice=$?
    if [ $tsservice -ne 0 ]; then tsservicedisp="Stopped"; else tsservicedisp="Started"; fi

    tailscale status >/dev/null 2>&1
    tsconn=$?
    if [ $tsconn -ne 0 ]; then tsconndisp="Disconnected"; else tsconndisp="Connected"; fi

    printf "\33[2K\r"

    if [ "$tsinstalleddisp" == "Installed" ]; then
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}  |-${CClear}-- ${CGreen}(R)${CClear}e-${CGreen}(S)${CClear}tart / S${CGreen}(T)${CClear}op Tailscale Service${CClear}      |--- ${CGreen}$tsservicedisp${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}  |-${CClear}-- ${CGreen}(U)${CClear}p / ${CGreen}(D)${CClear}own Tailscale Connection${CClear}           |--- ${CGreen}$tsconndisp${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}  |-${CClear}-- U${CGreen}(P)${CClear}date Tailscale Binary to latest version  |--- ${CGreen}v$tsver${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}  |-${CClear}-- Update Tailscale Binary to latest ${CGreen}(B)${CRed}ETA${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}  |-${CClear}-- ${CGreen}(F)${CClear}orce Downgrade to Older Tailscale version${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}  |-${CClear}-- ${CGreen}(I)${CClear}ssue Connection '--reset' Command${CClear}"
    fi

    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 2)${CClear} : Uninstall Tailscale Entware Package(s)${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 3)${CClear} : Set Tailscale Operating Mode                 : ${CGreen}$tsoperatingmode${CClear}"
    if [ "$tsoperatingmode" == "Custom" ]; then
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}  |-${CClear}-- Edit Custom ${InvGreen}${CWhite}(O)${CClear}peration Mode Settings${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 4)${CClear}${CDkGray} : Configure this Router as Exit Node           : $exitnodedisp${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 5)${CClear}${CDkGray} : Advertise Routes on this router              : $advroutesdisp${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 6)${CClear}${CDkGray} : Enable Site-to-Site functionality on router  : $accroutesdisp${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 7)${CClear}${CDkGray} : Enable Tailscale SSH server                  : $sshenabledisp${CClear}"
    else
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 4)${CClear} : Configure this Router as Exit Node           : ${CGreen}$exitnodedisp${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 5)${CClear} : Advertise Routes on this router              : ${CGreen}$advroutesdisp${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 6)${CClear} : Enable Site-to-Site functionality on router  : ${CGreen}$accroutesdisp${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 7)${CClear} : Enable Tailscale SSH server                  : ${CGreen}$sshenabledisp${CClear}"
    fi
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 8)${CClear} : Custom configuration options for ZeroScale${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( 9)${CClear} : Force reinstall Entware dependencies${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(10)${CClear} : Check for latest updates${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(11)${CClear} : Uninstall ZeroScale${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}  | ${CClear}"
    if tailscaleready; then
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( L)${CClear} : Launch ZeroScale in Monitoring Mode (${CGreen}sh /jffs/scripts/zeroscale.sh${CClear})"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( M)${CClear} : Launch ZeroScale in Monitoring Mode using SCREEN (${CGreen}sh /jf..ts/zeroscale.sh -screen${CClear})"
    else
      echo -e "${InvGreen} ${CClear} ${InvDkGray}( L) : Launch ZeroScale in Monitoring Mode              : Unavailable (install Tailscale first)${CClear}"
      echo -e "${InvGreen} ${CClear} ${InvDkGray}( M) : Launch ZeroScale using SCREEN                   : Unavailable (install Tailscale first)${CClear}"
    fi
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}  | ${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}( e)${CClear} : Exit${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo ""
    if [ "$tsinstalleddisp" == "Installed" ]; then
      if [ "$tsoperatingmode" == "Custom" ]; then
        read -p "Please select? (1-11, R/S/T/U/D/P/B/F/I/O/L/M, e=Exit): " SelectSlot
      else
        read -p "Please select? (1-11, R/S/T/U/D/P/B/F/I/L/M, e=Exit): " SelectSlot
      fi
    else
      read -p "Please select? (1-11, e=Exit): " SelectSlot
    fi
      case $SelectSlot in

        [Rr]) echo ""; restarttsc;;

        [Ss]) echo ""; startts;;

        [Tt]) echo ""; stopts;;

        [Uu]) echo ""; tsup;;

        [Dd]) echo ""; tsdown;;

        [Ll])
          if tailscaleready; then
            timer=$timerloop
            break
          else
            monitoringblocked pause
          fi
          ;;

        [Mm])
          if tailscaleready; then
            exec sh /jffs/scripts/zeroscale.sh -screen -now
          else
            monitoringblocked pause
          fi
          ;;

        [Oo]) if [ "$tsoperatingmode" == "Custom" ]; then
                customconfig
              fi ;;

        [Pp]) echo ""; tsupdate;;

        [Bb]) echo ""; tsbeta;;

        [Ff]) echo ""; tsdowngrade;;

        [Ii]) echo ""; tsreset;;

        1) installts;;

        2) uninstallts;;

        3) operatingmode;;

        4) if [ "$tsoperatingmode" != "Custom" ]; then
             exitnodets
           fi ;;

        5) if [ "$tsoperatingmode" != "Custom" ]; then
             advroutests
           fi ;;

        6) if [ "$tsoperatingmode" != "Custom" ]; then
             accroutests
           fi ;;

        7) if [ "$tsoperatingmode" != "Custom" ]; then
             sshts
           fi ;;

        8) installdependencies;;

        9) reinstalldependencies;;

        10) vupdate;;

        11) vuninstall;;

        [Ee]) echo -e "${CClear}"; timer=$timerloop; break;;

      esac
  done
}

# -------------------------------------------------------------------------------------------------------------------------
# vconfig is a function that provides a UI to choose various options for ZeroScale

vconfig()
{
  # Grab the ZeroScale config file and read it in
  if [ -f "$config" ]; then
    source "$config"
  else
    initialsetup
  fi

  while true; do

    if [ "$keepalive" -eq 0 ]; then
      keepalivedisp="No"
    else
      keepalivedisp="Yes"
    fi

    if [ "$persistentsettings" -eq 0 ]; then
      persistentsettingsdisp="No"
    else
      persistentsettingsdisp="Yes"
    fi

    if [ "$amtmemailsuccess" == "0" ] && [ "$amtmemailfailure" == "0" ]; then
      amtmemailsuccfaildisp="Disabled"
    elif [ "$amtmemailsuccess" == "1" ] && [ "$amtmemailfailure" == "0" ]; then
      amtmemailsuccfaildisp="Success"
    elif [ "$amtmemailsuccess" == "0" ] && [ "$amtmemailfailure" == "1" ]; then
      amtmemailsuccfaildisp="Failure"
    elif [ "$amtmemailsuccess" == "1" ] && [ "$amtmemailfailure" == "1" ]; then
      amtmemailsuccfaildisp="Success, Failure"
    else
      amtmemailsuccfaildisp="Disabled"
    fi

    rldisp=""
    if [ "$amtmemailsuccess" = "1" ] || [ "$amtmemailfailure" = "1" ]
      then
        if [ "$ratelimit" = "0" ]; then
          rldisp="| ${CRed}RL"
        else
          rldisp="| ${CGreen}RL:$ratelimit/h"
        fi
    fi

    if [ "$autostart" -eq 0 ]; then
      autostartdisp="Disabled"
    elif [ "$autostart" -eq 1 ]; then
      autostartdisp="Enabled"
    fi

    #scheduler colors and indicators
    if [ "$schedule" = "0" ]
    then
       schedtime="${CDkGray}01:00${CClear}"
    elif [ "$schedule" = "1" ]
    then
       schedhrs="$(printf "%02d" "$schedulehrs")"
       schedmin="$(printf "%02d" "$schedulemin")"
       schedtime="${CGreen}$schedhrs:$schedmin${CClear}"
    fi

    clear
    echo -e "${InvGreen} ${InvDkGray}${CWhite} ZeroScale Configuration Option                                                          ${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} Please choose from the various options below, which allow you to modify certain${CClear}"
    echo -e "${InvGreen} ${CClear} customizable parameters that affect the operation of this script.${CClear}"
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(1)${CClear} : Keep Tailscale Service Alive                 : ${CGreen}$keepalivedisp"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(2)${CClear} : Timer Check Loop Interval                    : ${CGreen}${timerloop}sec"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(3)${CClear} : Custom Event Log size (rows)                 : ${CGreen}$logsize"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(4)${CClear} : AMTM Email Notifications / Rate Limiting     : ${CGreen}$amtmemailsuccfaildisp $rldisp"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(5)${CClear} : Keep settings on Tailscale Entware updates   : ${CGreen}$persistentsettingsdisp"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(6)${CClear} : Autostart ZeroScale on Reboot                  : ${CGreen}$autostartdisp"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(7)${CClear} : Schedule ZeroScale + Tailscale Autoupdate      : ${CGreen}$schedtime${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite} | ${CClear}"
    echo -e "${InvGreen} ${CClear} ${InvDkGray}${CWhite}(e)${CClear} : Exit${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo ""
    read -p "Please select? (1-7, e=Exit): " SelectSlot
      case $SelectSlot in
        1)
          clear
          echo -e "${InvGreen} ${InvDkGray}${CWhite} Keep Tailscale Service Alive                                                          ${CClear}"
          echo -e "${InvGreen} ${CClear}"
          echo -e "${InvGreen} ${CClear} Please indicate if you want ZeroScale to check the status of the Tailscale Service${CClear}"
          echo -e "${InvGreen} ${CClear} and restart it if necessary? While Tailscale overall is fairly stable, there are${CClear}"
          echo -e "${InvGreen} ${CClear} instances where the service with terminate."
          echo -e "${InvGreen} ${CClear}"
          echo -e "${InvGreen} ${CClear} (Default = Yes)${CClear}"
          echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
          echo ""
          echo -e "${CClear}Current: ${CGreen}$keepalivedisp${CClear}"
          echo ""
          echo -e "Keep Alive?"
          if promptyn "[y/n]: "
            then
              keepalive=1
              echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale keepalive enabled." >> "$logfile"
            else
              keepalive=0
              echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale keepalive disabled." >> "$logfile"
          fi
          saveconfig
        ;;

        2) timerloopconfig
        ;;

        3)
          clear
          echo -e "${InvGreen} ${InvDkGray}${CWhite} Custom Event Log Size                                                                 ${CClear}"
          echo -e "${InvGreen} ${CClear}"
          echo -e "${InvGreen} ${CClear} Please indicate below how large you would like your Event Log to grow. I'm a poet${CClear}"
          echo -e "${InvGreen} ${CClear} and didn't even know it. By default, with 2000 rows, you will have many months of${CClear}"
          echo -e "${InvGreen} ${CClear} Event Log data."
          echo -e "${InvGreen} ${CClear}"
          echo -e "${InvGreen} ${CClear} Use 0 to Disable, max number of rows is 9999. (Default = 2000)"
          echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
          echo ""
          echo -e "${CClear}Current: ${CGreen}$logsize${CClear}"
          echo ""
          read -p "Please enter Log Size (in rows)? (0-9999, e=Exit): " NEWLOGSIZE

            if [ "$NEWLOGSIZE" == "e" ]; then
              echo -e "\n[Exiting]"; sleep 1
            elif [ "$NEWLOGSIZE" -ge 0 ] && [ "$NEWLOGSIZE" -le 9999 ]; then
              logsize=$NEWLOGSIZE
              saveconfig
              echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: Event log size configured for $logsize rows." >> "$logfile"
            else
              logsize=2000
              saveconfig
            fi
        ;;

        4)
          amtmevents
          source "$config"
        ;;

        5)
          clear
          echo -e "${InvGreen} ${InvDkGray}${CWhite} Keep Settings Persistent on Tailscale Entware Updates                                 ${CClear}"
          echo -e "${InvGreen} ${CClear}"
          echo -e "${InvGreen} ${CClear} Please indicate if you want ZeroScale to check the Tailscale Service settings on${CClear}"
          echo -e "${InvGreen} ${CClear} a regular basis to determine if settings are out-of-sync due to a possible${CClear}"
          echo -e "${InvGreen} ${CClear} Tailscale Entware upgrade? A common side-effect after updating the Tailscale${CClear}"
          echo -e "${InvGreen} ${CClear} Entware package is that it will remove your previously configured settings,${CClear}"
          echo -e "${InvGreen} ${CClear} which could cause your router to no longer participate on your tailnet.${CClear}"
          echo -e "${InvGreen} ${CClear}"
          echo -e "${InvGreen} ${CClear} (Default = No)${CClear}"
          echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
          echo ""
          echo -e "${CClear}Current: ${CGreen}$persistentsettingsdisp${CClear}"
          echo ""
          echo -e "Keep Settings Persistent?"
          if promptyn "[y/n]: "
            then
              persistentsettings=1
              echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale Keep Settings Persistent enabled." >> "$logfile"
            else
              persistentsettings=0
              echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale Keep Settings Persistent disabled." >> "$logfile"
          fi
          saveconfig
        ;;

        6) autostart;;

        7) scheduleautoupdates;;

        [Ee]) echo -e "${CClear}\n[Exiting]"; sleep 1; resettimer=1; break ;;

      esac
  done
}

# -------------------------------------------------------------------------------------------------------------------------
# vupdate is a function that provides a UI to check for script updates and allows you to install the latest version...

vupdate()
{

  while true; do

    updatecheck # Check for the latest stable version from source repository
    betacheck # Check for the latest beta version from source repository

    if [ "$track" = "0" ]; then
      trackdisp="Stable"
    else
      trackdisp="Beta"
    fi

    clear
    echo -e "${InvGreen} ${InvDkGray}${CWhite} Update Utility                                                                        ${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} This utility allows you to check, download and install updates from your preferred"
    echo -e "${InvGreen} ${CClear} Beta/Stable track subscription."
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} (Default = 0 - Stable)"
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear}${CWhite} Stable Track${CClear}"
    echo -e "${InvGreen} ${CClear} Local Version:       ${CGreen}$version${CClear}"
    echo -e "${InvGreen} ${CClear} Official Version:    ${CGreen}$DLversion${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear}${CWhite} Beta Track"
    echo -e "${InvGreen} ${CClear} Local Version:       ${CGreen}$version${CClear}"
    echo -e "${InvGreen} ${CClear} Latest Beta Version: ${CGreen}$Bversion${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear}${CDkGray}---------------------------------------------------------------------------------------${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo -e "${InvGreen} ${CClear} Your subscribed track: ${CGreen}$trackdisp${CClear}"
    echo -e "${InvGreen} ${CClear}"
    echo ""

    if [ "$track" = "0" ]; then
      if [ "$version" == "$DLversion" ]
      then
        echo -e "You are on the latest ${CGreen}STABLE${CClear} version! Download & Overwrite, or change Tracks?${CClear}"
        read -p "(Stable = 0, Beta = 1, Download = y/n, e=Exit): " SelectUpdate
        case $SelectUpdate in
          0)
            track=0
            saveconfig
            ;;

          1)
            track=1
            saveconfig
            ;;

          [Yy])
            echo ""
            echo -e "\nDownloading ZeroScale ${CGreen}STABLE${CClear}"
            curl --silent --retry 3 --connect-timeout 3 --max-time 5 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/main/zeroscale.sh" -o "/jffs/scripts/zeroscale.sh" && chmod 755 "/jffs/scripts/zeroscale.sh"
            echo ""
            echo -e "Download successful!${CClear}"
            echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale update successfully downloaded and installed." >> "$logfile"
            echo ""
            read -rsp $'Press any key to restart ZeroScale...\n' -n1 key
            exec /jffs/scripts/zeroscale.sh -setup
            ;;

          [Nn])
            echo ""
            echo ""
            echo -e "${CClear}Exiting Update Utility..."
            sleep 1
            return
            ;;

          [Ee])
            echo ""
            echo ""
            echo -e "${CClear}Exiting Update Utility..."
            sleep 1
            return
            ;;
        esac

      else

        echo -e "New ${CGreen}STABLE${CClear} version available! Download & Upgrade, or change Tracks?${CClear}"
        read -p "(Stable = 0, Beta = 1, Download = y/n, e=Exit): " SelectUpdate
        case $SelectUpdate in
          0)
            track=0
            saveconfig
            ;;

          1)
            track=1
            saveconfig
            ;;

          [Yy])
            echo ""
            echo -e "\nDownloading ZeroScale ${CGreen}STABLE${CClear}"
            curl --silent --retry 3 --connect-timeout 3 --max-time 5 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/main/zeroscale.sh" -o "/jffs/scripts/zeroscale.sh" && chmod 755 "/jffs/scripts/zeroscale.sh"
            echo ""
            echo -e "Download successful!${CClear}"
            echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale update successfully downloaded and installed." >> "$logfile"
            echo ""
            read -rsp $'Press any key to restart ZeroScale...\n' -n1 key
            exec /jffs/scripts/zeroscale.sh -setup
            ;;

          [Nn])
            echo ""
            echo ""
            echo -e "${CClear}Exiting Update Utility..."
            sleep 1
            return
            ;;

          [Ee])
            echo ""
            echo ""
            echo -e "${CClear}Exiting Update Utility..."
            sleep 1
            return
            ;;
        esac

      fi

    elif [ "$track" = "1" ]; then

        if [ "$version" == "$Bversion" ]; then
          echo -e "You are on the latest ${CGreen}BETA${CClear} version! Download & Overwrite, or change Tracks?${CClear}"
          read -p "(Stable = 0, Beta = 1, Download = y/n, e=Exit): " SelectUpdate
          case $SelectUpdate in
            0)
              track=0
              saveconfig
              ;;

            1)
              track=1
              saveconfig
              ;;

            [Yy])
              echo ""
              echo -e "\nDownloading ZeroScale ${CGreen}BETA${CClear}"
              curl --silent --retry 3 --connect-timeout 3 --max-time 5 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/beta/zeroscale.sh" -o "/jffs/scripts/zeroscale.sh" && chmod 755 "/jffs/scripts/zeroscale.sh"
              echo ""
              echo -e "Download successful!${CClear}"
              echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale BETA update successfully downloaded and installed." >> "$logfile"
              echo ""
              read -rsp $'Press any key to restart ZeroScale...\n' -n1 key
              exec /jffs/scripts/zeroscale.sh -setup
              ;;

            [Nn])
              echo ""
              echo ""
              echo -e "${CClear}Exiting Update Utility..."
              sleep 1
              return
              ;;

            [Ee])
              echo ""
              echo ""
              echo -e "${CClear}Exiting Update Utility..."
              sleep 1
              return
              ;;
          esac


        else

          echo -e "New ${CGreen}BETA${CClear} version available! Download & Upgrade, or change Tracks?${CClear}"
          read -p "(Stable = 0, Beta = 1, Download = y/n, e=Exit): " SelectUpdate
          case $SelectUpdate in
            0)
              track=0
              saveconfig
              ;;

            1)
              track=1
              saveconfig
              ;;

            [Yy])
              echo ""
              echo -e "\nDownloading ZeroScale ${CGreen}BETA${CClear}"
              curl --silent --retry 3 --connect-timeout 3 --max-time 5 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/beta/zeroscale.sh" -o "/jffs/scripts/zeroscale.sh" && chmod 755 "/jffs/scripts/zeroscale.sh"
              echo ""
              echo -e "Download successful!${CClear}"
              echo -e "$(date +'%b %d %Y %X') $($timeoutcmd$timeoutsec nvram get lan_hostname) ZEROSCALE[$$] - INFO: ZeroScale BETA update successfully downloaded and installed." >> "$logfile"
              echo ""
              read -rsp $'Press any key to restart ZeroScale...\n' -n1 key
              exec /jffs/scripts/zeroscale.sh -setup
              ;;

            [Nn])
              echo ""
              echo ""
              echo -e "${CClear}Exiting Update Utility..."
              sleep 1
              return
              ;;

            [Ee])
              echo ""
              echo ""
              echo -e "${CClear}Exiting Update Utility..."
              sleep 1
              return
              ;;
          esac
        fi
    fi
  done
}

# -------------------------------------------------------------------------------------------------------------------------
# updatecheck is a function that downloads the latest update version file, and compares it with what's currently installed

updatecheck()
{

  # Download the latest version file from the source repository
  curl --silent --retry 3 --connect-timeout 3 --max-time 6 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/main/version.txt" -o "/jffs/addons/zeroscale.d/version.txt"

  if [ -f "$dlverpath" ]
    then
      # Read in its contents for the current version file
      DLversion=$(cat "$dlverpath")

      # Compare the new version with the old version and log it
      if [ "$beta" == "1" ]; then   # Check if Dev/Beta Mode is enabled and disable notification message
        UpdateNotify=0
      elif [ "$DLversion" != "$version" ]; then
        DLversionPF=$(printf "%-8s" "$DLversion")
        versionPF=$(printf "%-8s" "$version")
        UpdateNotify="${InvYellow} ${InvDkGray}${CWhite} Stable Track Update available: v$versionPF -> v$DLversionPF                                                        ${CClear}"
      else
        UpdateNotify=0
      fi
  fi
}

# -------------------------------------------------------------------------------------------------------------------------
# betacheck is a function that downloads the latest beta version file, and compares it with what's currently installed

betacheck()
{

  # Download the latest version file from the source repository
  curl --silent --retry 3 --connect-timeout 3 --max-time 6 --retry-delay 1 --retry-all-errors --fail "https://raw.githubusercontent.com/underd0se/ZeroScale/beta/version.txt" -o "/jffs/addons/zeroscale.d/beta.txt"

  if [ -f "$bverpath" ]
    then
      # Read in its contents for the current version file
      Bversion=$(cat "$bverpath")

      # Compare the new version with the old version and log it
      if [ "$beta" == "1" ]; then   # Check if Dev/Beta Mode is enabled and disable notification message
        BUpdateNotify=0
      elif [ "$Bversion" != "$version" ]; then
        BversionPF=$(printf "%-8s" "$Bversion")
        versionPF=$(printf "%-8s" "$version")
        BUpdateNotify="${InvYellow} ${InvDkGray}${CWhite} Beta Track Update available: v$versionPF -> v$BversionPF                                                          ${CClear}"
      else
        BUpdateNotify=0
      fi

  fi
}

