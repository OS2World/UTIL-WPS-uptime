/*
 * File: delete.cmd
 *
 * Delete test object in 'uptime' class
 *
 * Bob Eager   October 2004
 *
 */

/* Load REXX system utility functions */

call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs

/* Delete the object */

res = SysDestroyObject('<WP_UPTIME>')

/* Check result and display status message */

if res = 1 then do
   say "Deleted OK"
end
else do
   say "Not deleted"
end

/*
 * End of file: delete.cmd
 *
 */

