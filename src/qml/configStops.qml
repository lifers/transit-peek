pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import org.kde.kirigami.dialogs as KirigamiDialogs

KCM.ScrollViewKCM {
    id: root
    property alias cfg_includeNearbyStops: nearbySwitch.checked
    property bool cfg_includeNearbyStopsDefault
    property alias cfg_nearbyDistanceMeters: nearbySlider.value
    property int cfg_nearbyDistanceMetersDefault
    property alias cfg_presets: presetModel.jsonPresets
    property list<string> cfg_presetsDefault

    PresetModel {
        id: presetModel
    }

    PresetMaker {
        id: presetDialog
        editMode: false
        property int presetIndex: -1

        onAccepted: {
            if (presetDialog.presetIndex < 0) {
                presetModel.addPreset(presetName);
            } else {
                presetModel.renamePreset(presetDialog.presetIndex, presetName);
            }

            presetDialog.presetIndex = -1;
        }
    }

    Kirigami.PromptDialog {
        id: confirmDeleteDialog
        property int presetIndex: -1
        property string presetTitle: ""
        title: "Delete preset"
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        modal: true

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            PC.Label {
                text: confirmDeleteDialog.presetIndex >= 0 && confirmDeleteDialog.presetTitle.length > 0 ? "Delete preset '" + confirmDeleteDialog.presetTitle + "'?" : "Delete this preset?"
            }
        }

        onAccepted: {
            if (confirmDeleteDialog.presetIndex >= 0) {
                presetModel.removePreset(confirmDeleteDialog.presetIndex);
            }
            confirmDeleteDialog.presetIndex = -1;
            confirmDeleteDialog.presetTitle = "";
        }

        onRejected: {
            confirmDeleteDialog.presetIndex = -1;
            confirmDeleteDialog.presetTitle = "";
        }
    }

    Kirigami.PromptDialog {
        id: confirmDeleteAllDialog
        title: "Delete all presets"
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        modal: true

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            PC.Label {
                text: "Delete all presets? This cannot be undone."
            }
        }

        onAccepted: {
            while (presetsList.count > 0) {
                presetModel.removePreset(presetsList.count - 1);
            }
        }
    }

    SearchModel {
        id: stopSearchModel
    }

    KirigamiDialogs.SearchDialog {
        id: stopSearchDialog
        property int presetIndex: -1
        property bool isSearching: false
        emptyText: text.length === 0 ? "Type a stop name or stop ID."
                    : isSearching ? "Searching..."
                    : "No stops found."
        searchFieldPlaceholderText: "Search by stop name or stop ID"
        model: stopSearchModel

        delegate: PC.ItemDelegate {
            required property string stopCode
            required property string stopName

            text: stopName + " (" + stopCode + ")"

            onClicked: {
                if (stopSearchDialog.presetIndex >= 0) {
                    presetModel.addStop(stopSearchDialog.presetIndex, stopCode);
                }
                stopSearchDialog.accept();
            }
        }

        onAccepted: presetIndex = -1
        onRejected: presetIndex = -1
        onTextChanged: {
            isSearching = text.length > 0;
            stopSearchModel.query = text;
        }
        onCountChanged: isSearching = false
    }

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            PC.Switch {
                id: nearbySwitch
                text: "Include stops near my location"
            }

            PC.Slider {
                id: nearbySlider
                Layout.fillWidth: true
                from: 100
                to: 1000
                stepSize: 100
                snapMode: PC.Slider.SnapAlways
                enabled: nearbySwitch.checked
            }

            PC.Label {
                text: nearbySlider.value + "m"
                opacity: nearbySwitch.checked ? 1 : 0.5
            }
        }
    }

    view: ListView {
        id: presetsList
        model: presetModel

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width
            visible: presetsList.count === 0
            text: "No presets yet"
            explanation: "Use Add preset to create your first preset."
        }

        delegate: PC.ItemDelegate {
            id: presetDelegate
            required property string title
            required property list<string> stops
            required property int index

            width: ListView.view.width
            highlighted: ListView.isCurrentItem
            onClicked: {
                presetsList.forceActiveFocus();
                presetsList.currentIndex = index;
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    Layout.fillWidth: true

                    PC.Label {
                        text: presetDelegate.title
                        font.bold: true
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    PC.Button {
                        text: "Edit name"
                        icon.name: "tag-edit"
                        onClicked: {
                            presetDialog.presetIndex = presetDelegate.index;
                            presetDialog.presetName = presetDelegate.title;
                            presetDialog.editMode = true;
                            presetDialog.open();
                        }
                    }

                    PC.Button {
                        text: "Delete"
                        icon.name: "edit-delete"
                        onClicked: {
                            confirmDeleteDialog.presetIndex = presetDelegate.index;
                            confirmDeleteDialog.presetTitle = presetDelegate.title;
                            confirmDeleteDialog.open();
                        }
                    }

                    PC.Button {
                        text: "Add stop"
                        icon.name: "list-add"
                        onClicked: {
                            stopSearchDialog.presetIndex = presetDelegate.index;
                            stopSearchDialog.open();
                        }
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Repeater {
                        model: presetDelegate.stops

                        delegate: Kirigami.Chip {
                            required property string modelData
                            required property int index

                            text: modelData
                            closable: true

                            onRemoved: presetModel.removeStop(presetDelegate.index, index)
                        }
                    }
                }
            }
        }
    }

    actions: [
        Kirigami.Action {
            text: "Add preset"
            icon.name: "list-add"
            onTriggered: {
                presetDialog.presetIndex = -1;
                presetDialog.presetName = "";
                presetDialog.editMode = false;
                presetDialog.open();
            }
        },
        Kirigami.Action {
            text: "Delete all presets"
            icon.name: "edit-delete"
            onTriggered: confirmDeleteAllDialog.open()
        }
    ]
}
