
This repo contains SlackBuilds (tested on -current) for nwg-shell:  
[https://nwg-piotr.github.io/nwg-shell/](https://nwg-piotr.github.io/nwg-shell/) - web  
[https://github.com/nwg-piotr/nwg-shell](https://github.com/nwg-piotr/nwg-shell) - git  

This is currently IN DEVELOPMENT and I consider it to be ALPHA stage and may very well be broken
or not work at all. (It does work).

The first package you need to build/install is google-go-lang and you should have python3 already.
One or two of the nwg packages dep on another, but I haven't done a clean build yet to know for
sure which ones dep on which ones. There may even be some missing yet. After building go, install
all the python deps, then all other apps (following deps as much as you can) and the nwg-* ones
last.

Follow  [https://github.com/nwg-piotr/nwg-shell/wiki](https://github.com/nwg-piotr/nwg-shell/wiki) 
this doc skipping any Arch-based info for initial setup. We don't have the AUR or baph on Slackware, 
so you won't get those features. Only time will tell if I create an alternative for Slackware for 
nwg-shell.

Some of these scripts were originally from the [SBo](https://slackbuilds.org) website they
have been edited from their packages on SBo, if you have any of them installed, they should work 
as-is, but I cannot make any guarantees. My advice is to build/install fresh packages.

I have put up a repo for testing based on -current, these builds will likely not run on Slackware 
15.0. When I feel it's officially stable enough, I'll make packages for the stable branch. Add the 
repo to your slackpkg+ config in order to have slackpkg manage the packages and updates for you.
[https://slackware.lngn.net/pub/x86_64/slackware64-current/nwg-shell/](https://slackware.lngn.net/pub/x86_64/slackware64-current/nwg-shell/)

## Notes

* The Go code is using the standard Slackware optimizations.  
* The shutdown/restart menu options currently don't work  
* gtklock is unsupported, use the alternative swaylock.  
* nwg-drawer has graphic issues above icons with some themes.  
* Scripts currently need online access to build, offline support "coming soon" (tm).  

