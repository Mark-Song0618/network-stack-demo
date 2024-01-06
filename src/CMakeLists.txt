add_executable("boost")

# add src files
file(GLOB src 
		  app/*.cpp
		  Infrastructure/*.cpp
		  layer1/*.cpp
		  layer2/*.cpp
		  layer3/*.cpp
		  layer4/*.cpp
		  layer5/*.cpp
		  utils/*.cpp
		  )

file(GLOB headers
		  app/*.h
		  Infrastructure/*.h
		  layer1/*.h
		  layer2/*.h
		  layer3/*.h
		  layer4/*.h
		  layer5/*.h
		  utils/*.h
		  )

target_sources("boost" PRIVATE ${src})

if (WIN32)
target_link_libraries("boost" wsock32 ws2_32)
endif()