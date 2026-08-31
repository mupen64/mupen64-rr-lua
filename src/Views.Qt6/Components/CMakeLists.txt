qt_add_library(Mupen64RR_Views_Qt6_Components STATIC)
qt_add_qml_module(Mupen64RR_Views_Qt6_Components
    URI "Components"
    QML_FILES
        "DialogBase.qml"
        "ToolTipLabel.qml"
)
target_link_libraries(Mupen64RR_Views_Qt6_Components PUBLIC
    Qt6::Quick Qt6::QuickControls2
)
