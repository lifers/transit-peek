import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: widget

    TransitFeed {
        id: feed
        presets: Plasmoid.configuration.presets
    }

    ArrivalsModel {
        id: rowsModel
        feed: feed
    }

    fullRepresentation: Item {
        id: popupView
        Layout.preferredWidth: 450
        Layout.preferredHeight: 150

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 4

            ArrivalsList {
                id: arrivalsList
                visible: arrivalsList.count > 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: rowsModel
            }

            PlasmaComponents.Label {
                visible: arrivalsList.count === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "No arrivals"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
