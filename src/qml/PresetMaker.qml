import QtQuick
import QtQuick.Layouts
import org.kde.plasma.components as PC
import org.kde.kirigami as Kirigami

Kirigami.PromptDialog {
    id: root
    property bool editMode: false
    property alias presetName: nameField.text

    title: editMode ? "Edit preset" : "Add preset"
    standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
    modal: true

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        PC.Label {
            text: root.editMode ? "Rename the preset" : "Name the preset"
        }

        PC.TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: "Preset name"
            onAccepted: root.accept()
        }
    }

    onVisibleChanged: {
        if (visible) {
            nameField.forceActiveFocus()
            nameField.selectAll()
        }
    }
}
