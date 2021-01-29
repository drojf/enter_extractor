#pragma once

#ifndef ENABLE_MULTITHREADED
#  define ENABLE_MULTITHREADED 0
#endif

#include <boost/filesystem.hpp>

extern const char* currentFileName;
extern bool SHOULD_WRITE_DEBUG_IMAGES;
extern boost::filesystem::path debugImagePath;
