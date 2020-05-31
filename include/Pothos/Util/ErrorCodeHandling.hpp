///
/// \file Util/ErrorCodeHandling.hpp
///
/// Mapping sets of error codes to Pothos exceptions
///
/// \copyright
/// Copyright (c) 2020 Nicholas Corgan
/// SPDX-License-Identifier: BSL-1.0
///

#pragma once
#include <Pothos/Config.hpp>
#include <Pothos/Exception.hpp>

#include <cerrno>
#include <cmath>
#include <system_error>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#endif

namespace Pothos {
namespace Util {

    /*!
     * Throw a specified exception type for the given error code, passing in a
     * std::error_category instance to access the associated error message. Error
     * codes for OS functions can be queried using std::generic_category or
     * std::system_category. Alternatively, a custom std::error_category subclass
     * can be passed in to convert arbitrary error codes to exceptions.
     *
     * \param errorCode The error code to convert into an exception
     * \param category A std::error_category mapping the error code to an error string
     * \throws An exception with given error code and associated error string
     */
    template <typename ExceptionType>
    inline void throwExceptionForErrorCode(int errorCode, const std::error_category& category);

    namespace Errno
    {
        /*!
         * If the given error code is nonzero, throw a Pothos::SystemException. The error
         * code will be checked against the errno codes supported by the current OS.
         *
         * Note that this function accounts for some functions returning the negative of
         * the given code.
         *
         * \param errorCode The errno code to convert into an exception
         * \throws Pothos::SystemException when the given errno code is non-zero
         */
        inline void throwExceptionForReturnCode(int errorCode);

        /*!
         * If <b>errno</b> is non-zero, throw a Pothos::SystemException with the corresponding
         * error string.
         *
         * Note that this function accounts for some functions setting errno to the negative of
         * the code's value.
         *
         * \throws Pothos::SystemException when errno is non-zero
         */
        inline void throwExceptionForErrno();
    }

#if defined(_WIN32) || defined(_WIN64)
    namespace WinError
    {
        /*!
         * If the given error code is nonzero, throw a Pothos::SystemException. The error
         * code will be checked against the set of WinAPI error code and HRESULT values
         * supported by the given version of Windows.
         *
         * \param errorCode The error code to convert into an exception
         * \throws Pothos::SystemException when the given errno code is non-zero
         */
        inline void throwExceptionForReturnCode(int errorCode);

        /*!
         * If the last WinAPI error (queried with GetLastError()) is non-zero,
         * throw a Pothos::SystemException. The error code will be checked against
         * the set of WinAPI error code and HRESULT values supported by the given
         * version of Windows.
         *
         * \throws Pothos::SystemException when the last WinAPI error is non-zero
         */
        inline void throwExceptionForLastError();
    }
#endif

} //namespace Util
} //namespace Pothos

//
// Implementations
//

template <typename ExceptionType>
inline void Pothos::Util::throwExceptionForErrorCode(int errorCode, const std::error_category& category)
{
    throw ExceptionType("Error code " + std::to_string(errorCode) + ": " + category.message(errorCode));
}

inline void Pothos::Util::Errno::throwExceptionForReturnCode(int errorCode)
{
    // On POSIX systems, std::generic_category() and std::system_category()
    // return the same set of errors. On Windows systems, std::generic_category()
    // returns errno's error set.
    if (errorCode) throwExceptionForErrorCode<Pothos::SystemException>(
        std::abs(errorCode),
        std::generic_category());
}

inline void Pothos::Util::Errno::throwExceptionForErrno()
{
    throwExceptionForReturnCode(errno);
}

#if defined(_WIN32) || defined(_WIN64)
inline void Pothos::Util::WinError::throwExceptionForReturnCode(int errorCode)
{
    if (errorCode) throwExceptionForErrorCode<Pothos::SystemException>(
        errorCode,
        std::system_category());
}

inline void Pothos::Util::WinError::throwExceptionForLastError()
{
    throwExceptionForReturnCode(GetLastError());
}
#endif