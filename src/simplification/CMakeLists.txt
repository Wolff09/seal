set(SOURCES
	rmCAS.cpp
	rmConditionals.cpp
	rmJumps.cpp
	rmScopedVars.cpp
	rmUselessScopes.cpp
	simplifyExpr.cpp
)

add_library(CoLaDesugar ${SOURCES})
target_link_libraries(CoLaDesugar CoLaUtil)