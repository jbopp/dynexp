// This file is part of DynExp.

/**
 * @file stdafx.h
 * @brief Accumulates include statements to provide a precompiled header.
*/

// Std-lib
#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cmath>
#include <compare>
#include <complex>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iterator>
#include <latch>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <numeric>
#include <queue>
#include <random>
#include <ranges>
#include <regex>
#include <source_location>
#include <stacktrace>
#include <string>
#include <string_view>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// g++ compiler does not come with support for std::stacktrace by default.
// So, disable this feature if it is not available.
#if (defined(_GLIBCXX_HAVE_STACKTRACE) || !defined(__GNUC__))
#define DYNEXP_HAS_STACKTRACE
#endif

// Qt
#include <Q3DSurface>
#include <QBarSeries>
#include <QBarSet>
#include <QByteArray>
#include <QChartView>
#include <QImage>
#include <QLineSeries>
#include <QLogValueAxis>
#include <QScatterSeries>
#include <QThread>
#include <QValueAxis>
#include <QtWidgets>
#include <QtXml>
#include <QXYSeries>

// GNU Scientific Library (GSL)
#include <gsl/gsl_fft_complex.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_siman.h>
#include <gsl/gsl_version.h>

// gRPC
#include <grpcpp/grpcpp.h>

// pybind11
// Temporarily undef Qt's "slots" macro to avoid a name conflict with pybind11.
#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#include <pybind11/chrono.h>
#include <pybind11/stl_bind.h>
#pragma pop_macro("slots")

// DynExp
#include "DynExpDefinitions.h"
#include "Util.h"
#include "circularbuf.h"
#include "QtUtil.h"
#include "PyUtil.h"