cmake_minimum_required(VERSION 3.16)

project(cpkchat VERSION 0.1 LANGUAGES CXX)


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 引入 Qt 组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# 项目源文件
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    res.qrc

    logindialog.h
    logindialog.cpp
    logindialog.ui

    registerdialog.h
    registerdialog.cpp
    registerdialog.ui

    global.h
    global.cpp

    singleton.h

    httpmgr.h
    httpmgr.cpp

    timerbtn.h
    timerbtn.cpp

    clickedlabel.h
    clickedlabel.cpp
)

# 等价于 qmake 里的 DISTFILES += config.ini
# 作用：让 config.ini 出现在 Qt Creator 的项目树里
set(PROJECT_DISTFILES
    config.ini
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(cpkchat
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${PROJECT_DISTFILES}
        resetdialog.h resetdialog.cpp resetdialog.ui
        tcpmgr.h tcpmgr.cpp
        chatdialog.h chatdialog.cpp chatdialog.ui
        clickedbtn.h clickedbtn.cpp
        customizeedit.h customizeedit.cpp
        chatuserlist.h chatuserlist.cpp
        listitembase.h listitembase.cpp
        chatuserswid.h chatuserswid.cpp chatuserswid.ui
        loadingdlg.h loadingdlg.cpp loadingdlg.ui
        chatpage.h chatpage.cpp chatpage.ui
        chatview.h chatview.cpp
        chatitembase.h chatitembase.cpp
        bubbleframe.h bubbleframe.cpp
        textbubble.h textbubble.cpp
        picturebubble.h picturebubble.cpp
        messagetextedit.h messagetextedit.cpp
        statewidget.h statewidget.cpp
        searchlist.h searchlist.cpp
        userdata.h userdata.cpp
        adduseritem.h adduseritem.cpp adduseritem.ui
        findsuccessdlg.h findsuccessdlg.cpp findsuccessdlg.ui
        applyfriend.h applyfriend.cpp applyfriend.ui
        clickedoncelabel.h clickedoncelabel.cpp
        friendlabel.h friendlabel.cpp friendlabel.ui
        usermgr.h usermgr.cpp
        contactuserlist.h contactuserlist.cpp
        conuseritem.h conuseritem.cpp conuseritem.ui
        grouptipitem.h grouptipitem.cpp grouptipitem.ui
        applyfriendpage.h applyfriendpage.cpp applyfriendpage.ui
        applyfrienditem.h applyfrienditem.cpp applyfrienditem.ui
        applyfriendlist.h applyfriendlist.cpp
        findfaildlg.h findfaildlg.cpp findfaildlg.ui
        authenfriend.h authenfriend.cpp authenfriend.ui
        friendinfopage.cpp friendinfopage.h friendinfopage.ui
    )
else()
    if(ANDROID)
        add_library(cpkchat SHARED
            ${PROJECT_SOURCES}
            ${PROJECT_DISTFILES}
        )
    else()
        add_executable(cpkchat
            ${PROJECT_SOURCES}
            ${PROJECT_DISTFILES}
        )
    endif()
endif()

# 链接 Qt 模块
target_link_libraries(cpkchat PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(cpkchat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 等价于 qmake 里的 win32:CONFIG(release, debug | release) + QMAKE_POST_LINK
# 作用：Windows Release 模式下，编译完成后把 config.ini 复制到 exe 所在目录
if(WIN32)
    add_custom_command(TARGET cpkchat POST_BUILD

        # 复制 config.ini
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
            "$<TARGET_FILE_DIR:cpkchat>/config.ini"

        # 只复制 static 文件夹
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/static"
            "$<TARGET_FILE_DIR:cpkchat>/static"

        COMMENT "Copy config.ini and static folder to output directory"
    )
endif()

# 其余配置保持不变
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.cpkchat)
endif()

set_target_properties(cpkchat PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

install(TARGETS cpkchat
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(cpkchat)
endif()
