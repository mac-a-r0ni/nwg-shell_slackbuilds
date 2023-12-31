if ! grep -q ^greeter: /etc/group ; then
  chroot . groupadd -g 381 greeter
  chroot . useradd -d /var/lib/greeter -u 381 -g greeter -G video -s /bin/false greeter
fi
