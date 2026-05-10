#!/bin/sh

# environment variables
export PATH="./build/cfglib/:$PATH"

# base commands
quit()    { send-cmd quit "$@"; }
setv()    { send-cmd setv "$1" "$2"; }
getv()    { send-cmd getv "$@"; }
enew()    { send-cmd enew; }
insert()  { send-cmd insert "$@"; }
finsert() { send-cmd finsert "$@"; }
write()   { send-cmd write "$@"; }
lcur()    { send-cmd lcur; }
rcur()    { send-cmd rcur; }
lcur_u()  { send-cmd lcur_u "$@"; }
rcur_u()  { send-cmd rcur_u "$@"; }
map()    { send-cmd bind "$1" "$2"; }

# aliases
q() { quit; }
e() { edit $@; }
w() { write $@; }
