
if (CONFIG_CUSTOM_RTCMCP7940)
	zephyr_include_directories(./)
	zephyr_library()
	zephyr_library_sources(RTCmcp7940.c)
endif()