import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PC
import id.lifers.transit_peek

PlasmoidItem {
    id: root

    Timer {
        interval: 60000
        repeat: true
        running: true
        triggeredOnStart: true
        onTriggered: GTFSReader.request()
    }

    fullRepresentation: ColumnLayout {
        anchors.fill: parent
        PC.Label {
            Layout.alignment: Qt.AlignCenter
            text: "hey ho"
        }
    }
}