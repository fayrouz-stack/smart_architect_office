cmake_minimum_required(VERSION 3.21)

project(MaterialManagementSystem VERSION 1.0 LANGUAGES CXX)

# Set Qt path
set(QT_PATH "C:/Qt/6.9.0/mingw_64")
set(CMAKE_PREFIX_PATH "${QT_PATH}/lib/cmake/Qt6")

# Enable automatic moc, uic and rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt components
find_package(Qt6 6.9.0 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Charts
    Network
    PrintSupport
    Svg
)

# Add executable with WIN32 flag (no console window)
# Group headers and sources
set(HEADERS
    mainwindow.h
    databaseconnection.h
)

# Add resources
set(RESOURCES
    resources.qrc
)

set(SOURCES
    main.cpp
    mainwindow.cpp
    databaseconnection.cpp
)

set(FORMS
    mainwindow.ui
)

add_executable(MaterialManagementSystem WIN32
    ${RESOURCES}
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
)

# Link Qt libraries
target_link_libraries(MaterialManagementSystem PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
    Qt6::Network
    Qt6::PrintSupport
    Qt6::Svg
)

# Set C++ standard
set_target_properties(MaterialManagementSystem PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Windows-specific deployment
if(WIN32)
    # Copy Qt DLLs
    add_custom_command(TARGET MaterialManagementSystem POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QT_PATH}/bin"
            $<TARGET_FILE_DIR:MaterialManagementSystem>
    )

    # Copy SQLite driver
    add_custom_command(TARGET MaterialManagementSystem POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:MaterialManagementSystem>/sqldrivers"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${QT_PATH}/plugins/sqldrivers/qsqlite.dll"
            "$<TARGET_FILE_DIR:MaterialManagementSystem>/sqldrivers/"
    )

    # Copy platform plugin
    add_custom_command(TARGET MaterialManagementSystem POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:MaterialManagementSystem>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy
            "${QT_PATH}/plugins/platforms/qwindows.dll"
            "$<TARGET_FILE_DIR:MaterialManagementSystem>/platforms/"
    )
endif()
