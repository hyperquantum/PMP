cd `dirname $0`

SRC_DIR="."
BIN_DIR="win32_build"
DIST_DIR="PMP_win32"
INCR_TMPDIR="PMP_win32_incremental"
ZIP_FILE="PMP_win32.zip"
INCR_ZIP="PMP_win32_incremental.zip"

MINGW_BIN_DIR="/cygdrive/C/MinGW/bin"
QT_BIN_DIR="/cygdrive/C/Qt/5.2.1/mingw48_32/bin"
QT_PLUGINS_DIR="/cygdrive/C/Qt/5.2.1/mingw48_32/plugins"

rm -rf "$DIST_DIR" "$INCR_TMPDIR"
mkdir  "$DIST_DIR" "$INCR_TMPDIR"
rm -rf "$ZIP_FILE" "$INCR_ZIP"

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

zip -r -q "$ZIP_FILE" "$DIST_DIR"/* \
  && rm -rf "$DIST_DIR" \
  && mv "$INCR_TMPDIR" "$DIST_DIR" \
  && zip -r -q "$INCR_ZIP" "$DIST_DIR"/* \
  && rm -rf "$DIST_DIR"
