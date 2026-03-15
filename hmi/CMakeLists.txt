qt_add_executable(panel-hmi
    main.cpp
    PanelFacade.cpp
    PanelFacade.h
    logo.png Back.png README.txt TestSettingsPage.qml SchedulePage.qml pageTestEx.qml pageTest.qml pageStart.qml pagePassCh.qml pagePass.qml  pageInfo.qml pageAjasteTime.qml pageAjasteSystem.qml pageAjaste.qml OperatorKeyboard.qml main.qml LogPage.qml LineItem.qml
    pageLineEdit.qml
)
qt_add_resources(panel-hmi "qml"
    PREFIX "/"
    FILES
        main.qml
        LineItem.qml
        LogPage.qml
        OperatorKeyboard.qml
        pageAjaste.qml
        pageAjasteSystem.qml
        pageAjasteTime.qml
        pageInfo.qml
        pageLineEdit.qml
        pagePass.qml
        pagePassCh.qml
        pageStart.qml
        pageTest.qml
        pageTestEx.qml
        SchedulePage.qml
        TestSettingsPage.qml
        logo.png
        Back.png
)

target_link_libraries(panel-hmi PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    core
)
