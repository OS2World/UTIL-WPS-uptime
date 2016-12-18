/*
 * File: ctest.cmd
 *
 * Create test object in 'uptime' class
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
   Note further that the object ID '<WP_UPTIME>' can be any string within
   '<' and '>'; it is not essential and can be replaced by a null string;
   however, having an object ID makes it a lot easier to identify the object
   later (e.g. in a REXX script to delete it).
   The caret (^) at the end of the object name merely adds a 'newline'
   character to the title text on the desktop; this separates the message
   count (if any) and places it on a separate line. */

res = SysCreateObject('uptime', 'test^', '<WP_DESKTOP>',,
        'OBJECTID=<WP_UPTIME>;REFRESH_INTERVAL=5;')

/* Check result and display status message */

if res = 1 then do
   say "Created OK"
end
else do
   say "Not created"
end

/*
 * End of file: ctest.cmd
 *
 */
