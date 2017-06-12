# Add install directive for our includes: On host, we install to /usr/include, while on platform we install into
# buildroot (note: not to the microSD):


if (JEVOIS_PLATFORM)
  install(DIRECTORY .
    COMPONENT include
    DESTINATION ${JEVOIS_BULDROOT_BASE}/target/usr/include
    FILES_MATCHING PATTERN "*.[Hh]"
    PATTERN "*.hpp"
    PATTERN ".svn" EXCLUDE
    )

else (JEVOIS_PLATFORM)
  install(DIRECTORY .
    COMPONENT include
    DESTINATION include
    FILES_MATCHING PATTERN "*.[Hh]"
    PATTERN "*.hpp"
    PATTERN ".svn" EXCLUDE
    )
endif (JEVOIS_PLATFORM)

set(CPACK_COMPONENTS_ALL include)
