#!/bin/sh
#
# install - install a program, script, or datafile
#
# topio: uninstall.sh, 2020-05-25 13:00 by smaug
#
# Copyright TOPNetwork Technology
#
# Permission to use, copy, modify, distribute, and sell this software and its
# documentation for any purpose is hereby granted without fee, provided that
# the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation, and that the name of M.I.T. not be used in advertising or
# publicity pertaining to distribution of the software without specific,
# written prior permission.  M.I.T. makes no representations about the
# suitability of this software for any purpose.  It is provided "as is"
# without express or implied warranty.
#
# This script is compatible with the BSD install script, but was written
# from scratch.
#

osname=`uname`
echo "osname: $osname"

pwd

echo_and_run() { echo "$*" ; "$@" ; }

echo_and_run echo "rm -f /usr/bin/topio" | bash -
echo_and_run echo "rm -f /lib/libxtopchain.so*" | bash -


echo "uninstall topio done"
exit 0
