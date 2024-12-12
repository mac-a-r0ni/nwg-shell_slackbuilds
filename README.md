
This repo contains SlackBuilds (tested on -current) for nwg-shell:  
[https://nwg-piotr.github.io/nwg-shell/](https://nwg-piotr.github.io/nwg-shell/) - website  
[https://github.com/nwg-piotr/nwg-shell](https://github.com/nwg-piotr/nwg-shell) - github  

Scripts for Slackware 15.0 have been submitted to [SBo](https://slackbulds.org), over half of the
packages are maintained by other people, but they all adhere to their guidelines for packaging.
The SBo branch of this repo has a copy of the latest SBo packages for this project. Missing deps
are available [from the site](https://slackbuilds.org). 

If you are on the stable 15.0 release, it is recommended to use SBo (with sboui or sbopkg) to install. 
If you're on -current, you should clone the main branch of this repo and run the `build-nwg-shell.sh [needs updates currently]
 to make packages, or preferably use the package repo for your arch linked below.

This is currently constantly IN DEVELOPMENT and I consider it to be BETA stage due to some patches being 
missing for a few packages. Things may occasionally break. (It does work). If you find a broken package
or something out of date, let me know! Things can slip by me occasionally.

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
as-is, but I cannot make any guarantees. My advice is to build/install fresh packages. Many of the scripts
on SBo pull in extra and unneeded deps for other modules, the scripts here avoid that but YMMV.

I have put up a repo for testing based on -current, these builds will not run on Slackware
15.0. Add the repo to your slackpkg+ config in order to have slackpkg manage the packages and
updates for you.  

Packages for x86_64:  
[https://slackware.lngn.net/pub/x86_64/slackware64-current/nwg-shell/](https://slackware.lngn.net/pub/x86_64/slackware64-current/nwg-shell/)  
Packages for aarch64:  
[https://slackware.lngn.net/pub/aarch64/slackwareaarch64-current/nwg-shell/](https://slackware.lngn.net/pub/aarch64/slackwareaarch64-current/nwg-shell/)  

If you would like to try a portable version of the system, I do periodically create a liveslak
which has a full install of nwg-shell on a slackware64-current system available here: [https://rekt.lngn.net/liveslak/](https://rekt.lngn.net/liveslak/) 

I will likely not produce a 15.0 file repo, please use SBo scripts. I'll only maintain the -current package repos as time allows.  
## Notes

* nwg-shell 0.5.X has added hyprland support, we officially support using Sway or Hyprland.  
* The Go code is using the standard Slackware optimizations.  
* nwg-drawer has small graphic issues above icons with some themes, I've found Adwaita-dark to look the best.  
* The package repos are signed with my GPG key, you can trust them with slackpkg+.  
