/*
 * REXX script to demonstrate UPTIME$ driver.
 *
 * Bob Eager   October 2004
 *
 */

device          = 'UPTIME$'

if stream(device, 'C', 'OPEN READ') \= 'READY:' then do
   say 'Cannot open device' device
   exit 1
end

uptime = charin(device,,4)
call stream device, 'C', 'CLOSE'
if length(uptime) \= 4 then do
   say 'Read error on' device 'device'
   exit 1
end

/* Convert characters to a value; least significant byte is first */

uptime = reverse(uptime)

seconds = c2x(uptime)

say 'System up for' uptime 'seconds'

exit 0

