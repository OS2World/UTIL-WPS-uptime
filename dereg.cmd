/*
 * File: dereg.cmd
 *
 * Deregister the 'uptime' class for WPS objects
 *
 * Bob Eager   October 2004
 *
 */

/* If this script appears to fail, reboot the system and try again.
   In fact, it may work but report failure for some reason; the safest
   approach is simply to reboot and run the script a second time, then
   assume all is OK. */

/* Load REXX system utility functions */

call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs

/* Deregister the class */

res = SysDeRegisterObjectClass('uptime')

/* Check result and display status message */

if res = 1 then do
   say "Deregistered OK"
end
else do
   say "Not deregistered"
end

/*
 * End of file: dereg.cmd
 *
 */
