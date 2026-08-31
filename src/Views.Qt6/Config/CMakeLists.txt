qt_add_library(Mupen64RR_Views_Qt6_Config STATIC)
set(qml_sources
    "Bool.qml"
    "Choices.qml"
    "Page.qml"
    "Row.qml"
)
set(qml_singleton_sources
    # "Dummy.qml"
)
# set_source_files_properties(
#     ${qml_singleton_sources}
#     PROPERTIES
#     QT_QML_SINGLETON_TYPE TRUE
# )
qt_add_qml_module(Mupen64RR_Views_Qt6_Config
    URI "Config"
    IMPORTS
        TARGET Mupen64RR_Views_Qt6_Components
    QML_FILES ${qml_sources} ${qml_singleton_sources}
)
target_link_libraries(Mupen64RR_Views_Qt6_Config PUBLIC
    Qt6::Quick Qt6::QuickControls2
)
