#!/bin/sh
/boot/apps/BeMail mailto:berenger@francenet.fr -subject "stampTV v3.0b5 report" -body "Thanks for taking the time to report how stampTV worked for you!

Please edit the following text to reflect your experience when trying stampTV:

1 - How did stampTV 3.0b5 worked?
It crashed/it produced grabage inside the window/it produced gargage all over the screen/it first worked, but stopped working when I did.../other...

2- Launch stampTV 3.0b5 from a terminal, and then quit it (if it hasn't crashed!), then cut & paste here the debug output printed in that terminal.
<paste here what stampTV 3.0b5 prints>

3 - more information:
<put here some additionnal information/comments>


--------------------------------------------------------------------------
Please do not edit the information below, unless you don't want to disclose some information it contains:

Version app_server:
`/bin/version /system/servers/app_server`

Version libbe.so:	
`/bin/version /system/lib/libbe.so`

Version bt848 driver:
`/bin/version /boot/beos/system/add-ons/kernel/drivers/bin/bt848`

/dev/video:
`/bin/ls /dev/video`

/dev/graphics:
`/bin/ls /dev/graphics`

System drivers:
`/bin/ls -lt /boot/beos/system/add-ons/kernel/drivers/bin/`

Custom drivers:
`/bin/ls -lt /boot/home/config/add-ons/kernel/drivers/bin/`
"

