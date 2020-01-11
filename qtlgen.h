#ifndef _QTLGEN_H_
#define _QTLGEN_H_

#pragma once

#include "qtlconfig.h"

#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif //_WIN32
void generate(const qtlconfig& config, const std::string& template_path, const std::string& output_path);

#endif //_QTLGEN_H_
