/* */
numeric digits 40

freq = x2d('001234cf')
hi = '00000002'
lo = '4c0fc2e3'
all = hi || lo

hi = x2d(hi)
lo = x2d(lo)
all = x2d(all)

say "Frequency =" freq
say "High =" hi
say "Low =" lo
say "All =" all

secs = all%freq
say "Secs =" secs

mins = secs%60
secs = secs//60
hours = mins%60
mins = mins//60 
days = hours%24
hours = hours//24
years = days%365
days = days//365

ut = right(hours,2,'0') || ":" || right(mins,2,'0') || ":" || right(secs,2,'0')
say "Uptime =" years "years," days "days," ut

exit

