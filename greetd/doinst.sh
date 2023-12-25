if ! grep -q ^greeter: /etc/group ; then
  chroot . groupadd -g 380 greeter
  chroot . useradd -d /var/lib/greeter -u 380 -g greeter -G video -s /bin/false greeter
fi
