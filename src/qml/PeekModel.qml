import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami

Item {
    id: root

    ArrivalsModel {
        id: model
    }

    RowLayout {
        id: contentRow
        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        Rectangle {
            id: routeBadge
            Layout.preferredWidth: root.routeBadgeWidth
            Layout.alignment: Qt.AlignVCenter
            implicitHeight: routeBadgeHeading.implicitHeight + Kirigami.Units.smallSpacing
            radius: height / 6
            color: root.topRow ? root.topRow.bgColor : Kirigami.Theme.disabledTextColor

            PlasmaComponents.Label {
                id: routeBadgeHeading
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.smallSpacing * 2)
                text: root.topRow ? (root.topRow.routeName || "?") : "-"
                color: root.topRow ? root.topRow.fgColor : Kirigami.Theme.textColor
                font.bold: true
                font.pointSize: 12
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            implicitHeight: headsignLabel.implicitHeight
            clip: true

            PlasmaComponents.Label {
                id: headsignLabel
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                text: root.topRow ? (root.topRow.headsign || "Unknown route") : "No arrivals"
                elide: Text.ElideRight
                font.bold: true
                font.pointSize: 10
            }
        }

        PlasmaComponents.Label {
            Layout.preferredWidth: root.arrivalWidth
            Layout.alignment: Qt.AlignVCenter
            horizontalAlignment: Text.AlignRight
            text: root.topRow ? (root.topRow.arrivalText || "") : ""
            font.pointSize: 10
        }
    }
}
