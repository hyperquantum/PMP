cd `dirname $0`

wc -l common/*.h common/*.cpp server/*.h server/*.cpp \
  tools/*.cpp gui-remote/*.h gui-remote/*.cpp CMakeLists.txt | sort -n
