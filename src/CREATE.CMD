/*
 * File: create.cmd
 *
 * Create an object in the 'uptime' class
 *
 * Bob Eager   October 2004
 *
 */

/* Load REXX system utility functions */

call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs

/* Create the object. Note that, in REXX, a comma at the end of a line
   is not a real comma; it is functionally replaced by a blank and prevents
   the end of the line from terminating the statement. Hence the next line
   below contains THREE arguments to the SysCreateObject call, and the
   final comma is merely a continuation character. The three lines after that
   consist of three strings which make up a SINGLE fourth argument to
   SysCreateObject; this is because each end-of-line comma is equivalent
   to a space, and the || operator concatenates strings!
   The caret (^) at the end of the object name merely adds a 'newline'
   character to the title text on the desktop; this separates the message
   count (if any) and places it on a separate line. */


say 'What name should be given to the object?'
parse pull objname

do forever
   say 'Refresh interval (secs)?'
   parse pull interval
   if (interval >= 1) & (interval <= 7200) then leave
   say 'Refresh interval must be in the range 1 to 7200 seconds'
   say ''
end

do forever
   say 'Uptime method (T,S,D)?'
   parse upper pull method
   if (method = 'T') | (method = 'S') | (method = 'D') then leave
   say 'Method must be T, S or D'
   say ''
end

setup = 'REFRESH_INTERVAL=' || interval || ';' || 'UPTIME_METHOD='method

res = SysCreateObject('uptime', objname, '<WP_DESKTOP>', setup, 'R')

/* Display status message */

if res = 0 then do
   say 'Object not created'
end
else do
   say 'Object created OK'
end

/*
 * End of file: create.cmd
 *
 */

