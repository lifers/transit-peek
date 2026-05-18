import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

Kirigami.CardsListView {
    id: root
    clip: true
    spacing: Kirigami.Units.largeSpacing
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick
    contentWidth: width

    delegate: PC.ItemDelegate {
        id: delegateRoot
        required property string stopName
        required property string routeName
        required property string headsign
        required property color iconFG
        required property color iconBG
        required property string arrivalText
        property int routeBadgeWidth: 48
        property int arrivalWidth: 72
        property int sideSpacing: Kirigami.Units.largeSpacing
        property real viewLeftMargin: ListView.view ? ListView.view.leftMargin : 0
        property real viewRightMargin: ListView.view ? ListView.view.rightMargin : 0

        x: viewLeftMargin
        width: ListView.view
            ? Math.max(0, ListView.view.width - viewLeftMargin - viewRightMargin)
            : 0

        implicitHeight: delegateLayout.implicitHeight

        Item {
            id: delegateLayout
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            implicitHeight: Math.max(
                routeBadge.implicitHeight,
                middleContainer.implicitHeight,
                arrivalLabel.implicitHeight
            )

            Rectangle {
                id: routeBadge
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                }
                width: delegateRoot.routeBadgeWidth
                implicitHeight: routeBadgeHeading.implicitHeight + Kirigami.Units.smallSpacing
                radius: height / 6
                color: delegateRoot.iconBG

                PC.Label {
                    id: routeBadgeHeading
                    anchors.centerIn: parent
                    width: parent.width - (Kirigami.Units.smallSpacing * 2)
                    text: delegateRoot.routeName || "?"
                    color: delegateRoot.iconFG
                    font.bold: true
                    font.pointSize: 16
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            PC.Label {
                id: arrivalLabel
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                width: delegateRoot.arrivalWidth
                horizontalAlignment: Text.AlignRight
                text: delegateRoot.arrivalText
            }

            Item {
                id: middleContainer
                anchors {
                    left: routeBadge.right
                    leftMargin: delegateRoot.sideSpacing
                    right: arrivalLabel.left
                    rightMargin: delegateRoot.sideSpacing
                    verticalCenter: parent.verticalCenter
                }
                implicitHeight: middleColumn.implicitHeight
                clip: true

                ColumnLayout {
                    id: middleColumn
                    anchors.fill: parent
                    spacing: 0

                    PC.Label {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: delegateRoot.headsign || "Unknown route"
                        elide: Text.ElideRight
                        font.bold: true
                    }

                    PC.Label {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        visible: delegateRoot.stopName.length > 0
                        text: delegateRoot.stopName
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
