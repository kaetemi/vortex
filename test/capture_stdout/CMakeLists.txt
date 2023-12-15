
FILE(GLOB SRCS *.cpp)
FILE(GLOB HDRS *.h)
IF (WIN32)
  FILE(GLOB RSRC *.rc *.manifest)
ENDIF (WIN32)
SOURCE_GROUP("" FILES ${SRCS} ${HDRS} ${RSRC})

ADD_EXECUTABLE(capture_stdout
  ${SRCS}
  ${HDRS}
  ${RSRC}
)

TARGET_LINK_LIBRARIES(capture_stdout
  common
)
