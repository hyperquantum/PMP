# This script is necessary when using bzr on Cygwin in combination with
# an editor/IDE outside Cygwin.

cd `dirname $0`

chmod -x common/*.h common/*.cpp server/*.h server/*.cpp \
  tools/*.cpp gui-remote/*.h gui-remote/*.cpp *.txt
