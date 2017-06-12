# Add install directive for our includes:
install(DIRECTORY .
  COMPONENT include
  DESTINATION include
  FILES_MATCHING PATTERN "*.[Hh]"
  PATTERN "*.hpp"
  PATTERN ".svn" EXCLUDE
  )

set(CPACK_COMPONENTS_ALL include)
