/*
 * InvalidTypeException.hpp
 *
 *  Created on: 02.03.2012
 *      Author: Sergey Bulavintsev
 */

#ifndef INVALIDTYPEEXCEPTION_HPP_
#define INVALIDTYPEEXCEPTION_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class DDMLIB_API InvalidTypeException : public std::runtime_error {
public:

    /**
     * Constructs a new exception with the default detail message.
     * @see java.lang.Exception
     */
	InvalidTypeException() : std::runtime_error("Invalid Type"){};
    /**
     * Constructs a new exception with the specified detail message.
     * @param message the detail message. The detail message is saved for later retrieval
     * by the {@link Throwable#getMessage()} method.
     * @see java.lang.Exception
     */
	InvalidTypeException(const std::string& message) : std::runtime_error(message) {};

    /**
     * Constructs a new exception with the specified cause and a detail message of
     * <code>(cause==null ? null : cause.toString())</code> (which typically contains
     * the class and detail message of cause).
     * @param cause the cause (which is saved for later retrieval by the
     * {@link Throwable#getCause()} method). (A <code>null</code> value is permitted,
     * and indicates that the cause is nonexistent or unknown.)
     * @see java.lang.Exception
     */
	InvalidTypeException(std::runtime_error& cause) : std::runtime_error(cause.what()){};
    /**
     * Constructs a new exception with the specified detail message and cause.
     * @param message the detail message. The detail message is saved for later retrieval
     * by the {@link Throwable#getMessage()} method.
     * @param cause the cause (which is saved for later retrieval by the
     * {@link Throwable#getCause()} method). (A <code>null</code> value is permitted,
     * and indicates that the cause is nonexistent or unknown.)
     * @see java.lang.Exception
     */
    InvalidTypeException(const std::string& message, std::runtime_error& cause) : std::runtime_error(message) {};

private:
    /**
     * Needed by {@link Serializable}.
     */
    static long long serialVersionUID;
};

} /* namespace ddmlib */
#endif /* INVALIDTYPEEXCEPTION_HPP_ */
