# This script packages the binaries for Windows-x86.
# It is meant to be run in a Cygwin shell.

cd $(dirname $0)

SRC_DIR="."
BIN_DIR="bin_win32_release"
DIST_DIR="PMP_win32"
INCR_TMPDIR="PMP_win32_incremental"
ZIP_FILE="PMP_win32.zip"
INCR_ZIP="PMP_win32_incremental.zip"

MINGW_BIN_DIR="/cygdrive/C/MinGW/bin"
QT_BIN_DIR="/cygdrive/C/Qt/5.2.1/mingw48_32/bin"
QT_PLUGINS_DIR="/cygdrive/C/Qt/5.2.1/mingw48_32/plugins"

# (1) cleanup from previous runs (if necessary)
rm -rf "$DIST_DIR" "$INCR_TMPDIR"
mkdir  "$DIST_DIR" "$INCR_TMPDIR"
rm -rf "$ZIP_FILE" "$INCR_ZIP"

# (2) run build
echo "Running make..."
pushd "$BIN_DIR" >/dev/null
mingw32-make || exit
popd >/dev/null

# (3) copy files to release directories
echo "Copying files..."

cp "$SRC_DIR"/README* "$DIST_DIR"/
cp "$SRC_DIR"/*LICENSE* "$DIST_DIR"/

cp "$BIN_DIR"/PMP-Server.exe "$DIST_DIR"/
cp "$BIN_DIR"/PMP-GUI-Remote.exe "$DIST_DIR"/
cp "$BIN_DIR"/PMP-Cmd-Remote.exe "$DIST_DIR"/
cp "$BIN_DIR"/PMP-HashTool.exe "$DIST_DIR"/

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

chmod -R +r "$DIST_DIR"

cp "$DIST_DIR"/libmysql.dll "$INCR_TMPDIR"/
cp "$DIST_DIR"/*.exe "$INCR_TMPDIR"/
cp "$DIST_DIR"/*LICENSE* "$INCR_TMPDIR"/
cp "$DIST_DIR"/README* "$INCR_TMPDIR"/

# (4) create zip files
echo "Creating full archive..."
zip -r -q "$ZIP_FILE" "$DIST_DIR"/* \
  && rm -rf "$DIST_DIR" || exit

echo "Creating incremental archive..."
mv "$INCR_TMPDIR" "$DIST_DIR" \
  && zip -r -q "$INCR_ZIP" "$DIST_DIR"/* \
  && rm -rf "$DIST_DIR" || exit

echo "Finished."
