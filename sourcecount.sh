# A simple script to count the number of lines of sourcecode, all included.

cd `dirname $0`

wc -l \
  src/common/*.h src/common/*.cpp src/server/*.h src/server/*.cpp \
  src/tools/*.cpp src/gui-remote/*.h src/gui-remote/*.cpp \
  CMakeLists.txt src/CMakeLists.txt \
 | sort -n

# This does not seem to work in a Git Shell window (MinGW)
#find src -name '*.h' -o -name '*.cpp' -o -iname 'CMakeLists.txt' -print0 \
#  | wc -l --files0-from=- | sort -n
