#!/bin/sh

# autogen processing
function do_autogen()
{
  autoreconf -vfi
  #aclocal
  #libtoolize --force
  #autoheader
  #automake -a
  #autoconf
}

do_autogen

#autoreconf


