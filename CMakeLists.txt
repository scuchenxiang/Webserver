cmake_minimum_required(VERSION 3.15)#camke版本号
project(my_WebServer) #项目名称
set(CMAKE_CXX_STANDARD 14)#设置C++版本
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)# 指定执行程序的输出路径

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmakeModule/;${CMAKE_MODULE_PATH};")# 添加寻找FindSFML.cmake文件的路径
find_package(Threads REQUIRED) # 查找第三方库，这个应该不是module模式，Config模式
find_package(MySQL REQUIRED) # 这个应该是Module模式
aux_source_directory(./codes DIR_FILE)#查找./codes目录下的所有源文件，并将它们保存到DIR_FILE变量中
add_executable(my_WebServer ${DIR_FILE} main.cpp)#将${DIR_FILE}和 main.cpp编译成my_WebServer 
target_include_directories(my_WebServer PUBLIC ${PROJECT_SOURCE_DIR}/headers) # 添加头文件所在路径，这时候cpp中就可以直接引用，而不用管路径了
# 指明my_WebServer还需要添加Threads::Threads和${MYSQL_LIB}的链接库，添加pthread,mysql支持
target_link_libraries(my_WebServer PUBLIC Threads::Threads ${MYSQL_LIB})

include_directories(${MYSQL_INCLUDE_DIR})