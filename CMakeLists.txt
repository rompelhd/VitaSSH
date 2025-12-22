cmake_minimum_required(VERSION 3.16)

# --- Toolchain Vita ---
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(ssh_client_vita)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

# --- Metadata ---
set(VITA_APP_NAME "VitaSSH")
set(VITA_TITLEID "ROM000069")
set(VITA_VERSION  "01.00")

# --- Flags ---
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# --- Includes ---
include_directories(
  ./include
  ./src
  ../common
  $ENV{VITASDK}/arm-vita-eabi/include
)

# --- Create include directory if it doesn't exist ---
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Sources ---
add_executable(${PROJECT_NAME}
  src/main.c
  src/input.c
  src/network.c
  src/ssh_client.c
  src/shell.c
  src/text.c
)

target_link_libraries(${PROJECT_NAME}
  vita2d
  ssh2
  ssl
  crypto
  z
  SceDisplay_stub
  SceCtrl_stub
  SceSysmodule_stub
  SceNet_stub
  SceNetCtl_stub
  SceLibKernel_stub
  SceGxm_stub
  SceIme_stub
  SceAppUtil_stub
  SceSysmem_stub
  SceCommonDialog_stub
  SceAppMgr_stub
  ScePgf_stub
  png
  bz2
  SceTouch_stub
  m
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/fonts/HackNerdFontMono-Regular.ttf sce_sys/fonts/HackNerdFontMono-Regular.ttf
  FILE ./sce_sys/icon0.png sce_sys/icon0.png
  FILE ./sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE ./sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
