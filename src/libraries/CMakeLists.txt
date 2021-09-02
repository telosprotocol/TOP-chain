add_compile_options(-static)

add_subdirectory(xapplication)
add_subdirectory(xpara_proxy)
add_subdirectory(xelect)
add_subdirectory(xvnode)
add_subdirectory(xchaininit)
add_subdirectory(xsync)
add_subdirectory(xgrpc_mgr)

if (LEAK_TRACER)
add_subdirectory(leaktracer)
endif()
