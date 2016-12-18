/*
 * File: reg.cmd
 *
 * Register the 'uptime' class for WPS objects
 *
 * Bob Eager   October 2004
 *
 */

/* Load REXX system utility functions */

call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs

/* Register the class */

res = SysRegisterObjectClass('uptime', 'F:\OTHERDLL\UPTIME.DLL')

/* Check result and display status message */

if res = 1 then do
   say "Registered OK"
end
else do
   say "Not registered"
end

/*
 * End of file: reg.cmd
 *
 */


