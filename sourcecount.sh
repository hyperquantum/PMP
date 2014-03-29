cd `dirname $0`

wc -l common/*.h common/*.cpp server/*.h server/*.cpp \
  tools/*.cpp remote/*.h remote/*.cpp CMakeLists.txt | sort -n
