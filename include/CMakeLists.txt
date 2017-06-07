# Add install directive for our includes:
if (JEVOIS_PLATFORM)
  # do not install and do not pack include files
else (JEVOIS_PLATFORM)
  install(DIRECTORY .
    COMPONENT include
    DESTINATION include
    FILES_MATCHING PATTERN "*.[Hh]"
    PATTERN "*.hpp"
    PATTERN ".svn" EXCLUDE
    )

  set(CPACK_COMPONENTS_ALL include)
endif (JEVOIS_PLATFORM)
