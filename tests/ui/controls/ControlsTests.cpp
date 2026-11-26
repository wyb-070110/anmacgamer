// Runs the QML tests next to this file (tst_*.qml) against the GameScrcpy QML modules linked in.

#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

Q_IMPORT_QML_PLUGIN(Anmacgamer_ThemePlugin)
Q_IMPORT_QML_PLUGIN(Anmacgamer_IconsPlugin)
Q_IMPORT_QML_PLUGIN(Anmacgamer_ControlsPlugin)

QUICK_TEST_MAIN(gs_controls)
