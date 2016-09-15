# This script packages the binaries for Windows-x86.
# It is meant to be run in a Cygwin shell.

# PREREQUISITES:
#   - Cygwin environment with "zip" tool installed
#   - CMake installed in Windows  (C:\Program Files (x86)\CMake)
#   - MinGW32 installed  (C:\MinGW)
#   - QT5 installed in Windows  (C:\Qt\5.2.1\mingw48_32)
#   - taglib and pkg-config installed in the MinGW path as described in the README
#   - libmysql.dll needs to be copied into the build directory as described in the README

cd $(dirname "$0")

SRC_DIR="."
BIN_DIR="bin_win32_release"
ZIPROOT_DIR="PMP_win32"
INCR_TMPDIR="PMP_win32_incremental"
FULL_ZIP="PMP_win32.zip"
INCR_ZIP="PMP_win32_incremental.zip"
ZIPSRCFULL_DIR="archive_full"
ZIPSRCINCR_DIR="archive_incremental"

MINGW_BIN_DIR="/cygdrive/C/MinGW/bin"
QT_BIN_DIR="/cygdrive/C/Qt/5.2.1/mingw48_32/bin"
QT_PLUGINS_DIR="/cygdrive/C/Qt/5.2.1/mingw48_32/plugins"

CMAKE_EXE="/cygdrive/C/Program Files (x86)/CMake/bin/cmake.exe"

# (1) cleanup from previous runs (if necessary)
rm -rf "$ZIPSRCFULL_DIR" "$ZIPSRCINCR_DIR"
mkdir -p "$ZIPSRCFULL_DIR"/"$ZIPROOT_DIR" "$ZIPSRCINCR_DIR"/"$ZIPROOT_DIR"
rm -rf "$FULL_ZIP" "$INCR_ZIP"

# (2) create build directory if first time
if [ ! -d "$BIN_DIR" ]; then
    echo "Creating build directory..."
    mkdir "$BIN_DIR" || exit
fi

# (3) run CMake for the first time
if [ ! -f "$BIN_DIR"/Makefile ]; then
    if [ ! -x "$CMAKE_EXE" ] ; then
        >&2 echo "ERROR: CMake executable not found: $CMAKE_EXE"
        exit
    fi
    
    echo "Running CMake..."
    pushd "$BIN_DIR" >/dev/null
    "$CMAKE_EXE" \
        -G "MinGW Makefiles" \
        -D "CMAKE_CXX_COMPILER=mingw32-g++.exe" \
        -D "CMAKE_BUILD_TYPE:STRING=Release" .. || exit
    popd >/dev/null
fi

# (4) run build
echo "Running make..."
pushd "$BIN_DIR" >/dev/null
mingw32-make || exit
popd >/dev/null

# (5) copy files to release directories

if [ ! -f "$BIN_DIR"/libmysql.dll ] ; then
    >&2 echo "ERROR: libmysql.dll not found in directory $BIN_DIR"
    exit
fi

echo "Copying files..."

DIST_DIR="$ZIPSRCFULL_DIR"/"$ZIPROOT_DIR"
INCR_DIST_DIR="$ZIPSRCINCR_DIR"/"$ZIPROOT_DIR"

# Provide placeholders for old files that have been renamed. We want to
# overwrite the file with the old name in existing installations.
#  - README.txt has been renamed to README.md
echo "This file is obsolete. Open README.md instead." > "$INCR_DIST_DIR"/README.txt
#  - BUGS.txt has been renamed to TODOs.txt
echo "This file is obsolete. Open TODOs.txt instead." > "$INCR_DIST_DIR"/BUGS.txt

cp "$SRC_DIR"/README* "$DIST_DIR"/
cp "$SRC_DIR"/TODOs* "$DIST_DIR"/
cp "$SRC_DIR"/*LICENSE* "$DIST_DIR"/

cp "$BIN_DIR"/src/PMP-Server.exe "$DIST_DIR"/
cp "$BIN_DIR"/src/PMP-GUI-Remote.exe "$DIST_DIR"/
cp "$BIN_DIR"/src/PMP-Cmd-Remote.exe "$DIST_DIR"/
cp "$BIN_DIR"/src/PMP-HashTool.exe "$DIST_DIR"/

cp "$BIN_DIR"/libmysql.dll "$DIST_DIR"/

cp "$MINGW_BIN_DIR"/libtag.dll "$DIST_DIR"/

cp "$QT_BIN_DIR"/libgcc_s_dw2-1.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/libstdc++-6.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/libwinpthread-1.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/icudt51.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/icuin51.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/icuuc51.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/Qt5Concurrent.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/Qt5Core.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/Qt5Gui.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/Qt5Multimedia.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/Qt5MultimediaWidgets.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/Qt5Network.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/Qt5OpenGL.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/Qt5Sql.dll "$DIST_DIR"/
cp "$QT_BIN_DIR"/Qt5Widgets.dll "$DIST_DIR"/

cp -r "$QT_PLUGINS_DIR"/iconengines "$DIST_DIR"
cp -r "$QT_PLUGINS_DIR"/imageformats "$DIST_DIR"
cp -r "$QT_PLUGINS_DIR"/mediaservice "$DIST_DIR"
cp -r "$QT_PLUGINS_DIR"/platforms "$DIST_DIR"
cp -r "$QT_PLUGINS_DIR"/sqldrivers "$DIST_DIR"

# files that are part of the incremental archive
cp "$DIST_DIR"/*.exe "$INCR_DIST_DIR"/
cp "$DIST_DIR"/*LICENSE* "$INCR_DIST_DIR"/
cp "$DIST_DIR"/README* "$INCR_DIST_DIR"/
cp "$DIST_DIR"/TODOs* "$INCR_DIST_DIR"/
cp "$DIST_DIR"/libtag.dll "$INCR_DIST_DIR"/  # TagLib has been updated to version 1.11 on 2016-09-12

chmod -R +r "$DIST_DIR"
chmod -R +r "$INCR_DIST_DIR"

# (6) create zip files

echo "Creating incremental archive..."

pushd "$ZIPSRCINCR_DIR" >/dev/null \
  && zip -r -q "$INCR_ZIP" "$ZIPROOT_DIR"/* \
  && popd >/dev/null \
  && mv "$ZIPSRCINCR_DIR"/"$INCR_ZIP" . \
  && rm -rf "$ZIPSRCINCR_DIR" \
  || exit

echo "Creating full archive..."

pushd "$ZIPSRCFULL_DIR" >/dev/null \
  && zip -r -q "$FULL_ZIP" "$ZIPROOT_DIR"/* \
  && popd >/dev/null \
  && mv "$ZIPSRCFULL_DIR"/"$FULL_ZIP" . \
  && rm -rf "$ZIPSRCFULL_DIR" \
  || exit

echo "Finished."
