/*
 * Header:
 *      src/libdbproxy/value.h
 *
 * Description:
 *      Handling of a cell value to access data within the Cassandra database.
 *
 * Documentation:
 *      See the corresponding .cpp file.
 *
 * License:
 *      Copyright (c) 2011-2017 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include "libdbproxy/consistency_level.h"

#include <cassvalue/encoder.h>
#include <cassvalue/value.h>

#include <QString>
#include <QByteArray>

#include <execinfo.h>
#include <stdint.h>

#include <iostream>
#include <memory>

namespace libdbproxy
{

/*******************************************************************************
 * Light wrappers around the functions and classes in the cassvalue library
 *******************************************************************************/

inline uint64_t getBufferMaxSize()
{
    return cassvalue::getBufferMaxSize();
}


inline void checkBufferSize(const uint64_t new_size)
{
    return cassvalue::checkBufferSize(new_size);
}

// Null
inline void setNullValue(QByteArray& array)
{
    cassvalue::setNullValue(array);
}

// Bool
inline void appendBoolValue(QByteArray& array, const bool value)
{
    cassvalue::appendBoolValue(array,value);
}

inline void setBoolValue(QByteArray& array, const bool value)
{
    cassvalue::setBoolValue(array,value);
}

inline bool boolValue(const QByteArray& array, const int index = 0)
{
    return cassvalue::boolValue(array,index);
}

inline bool boolValueOrNull(const QByteArray& array, const int index = 0, const bool default_value = false)
{
    return cassvalue::boolValueOrNull(array,index,default_value);
}

// Char
inline void appendCharValue(QByteArray& array, const char value)
{
    cassvalue::appendCharValue(array,value);
}

inline void setCharValue(QByteArray& array, const char value)
{
    cassvalue::setCharValue(array,value);
}

inline char charValue(const QByteArray& array, const int index = 0)
{
    return cassvalue::charValue(array,index);
}

inline char charValueOrNull(const QByteArray& array, const int index = 0, const char default_value = 0)
{
    return cassvalue::charValueOrNull(array,index,default_value);
}

inline char safeCharValue(const QByteArray& array, const int index = 0, const char default_value = 0)
{
    return cassvalue::safeCharValue(array,index,default_value);
}

inline void appendSignedCharValue(QByteArray& array, const signed char value)
{
    cassvalue::appendCharValue(array, value);
}

inline void setSignedCharValue(QByteArray& array, const signed char value)
{
    cassvalue::setCharValue(array, value);
}

inline signed char signedCharValue(const QByteArray& array, const int index = 0)
{
    return cassvalue::signedCharValue(array,index);
}

inline signed char signedCharValueOrNull(const QByteArray& array, const int index = 0, const signed char default_value = 0)
{
    return cassvalue::signedCharValueOrNull(array,index,default_value);
}

inline signed char safeSignedCharValue(const QByteArray& array, const int index = 0, const signed char default_value = 0)
{
    return cassvalue::safeSignedCharValue(array,index,default_value);
}

inline void appendUnsignedCharValue(QByteArray& array, const unsigned char value)
{
    cassvalue::appendCharValue(array, value);
}

inline void setUnsignedCharValue(QByteArray& array, const unsigned char value)
{
    cassvalue::setCharValue(array, value);
}

inline unsigned char unsignedCharValue(const QByteArray& array, const int index = 0)
{
    return cassvalue::unsignedCharValue(array,index);
}

inline unsigned char unsignedCharValueOrNull(const QByteArray& array, const int index = 0, const unsigned char default_value = 0)
{
    return cassvalue::unsignedCharValueOrNull(array,index,default_value);
}

inline unsigned char safeUnsignedCharValue(const QByteArray& array, const int index = 0, const unsigned char default_value = 0)
{
    return cassvalue::safeUnsignedCharValue(array,index,default_value);
}

// Int16
inline void appendInt16Value(QByteArray& array, const int16_t value)
{
    cassvalue::appendInt16Value(array,value);
}

inline void setInt16Value(QByteArray& array, const int16_t value)
{
    cassvalue::setInt16Value(array,value);
}

inline int16_t int16Value(const QByteArray& array, const int index = 0)
{
    return cassvalue::int16Value(array,index);
}

inline int16_t int16ValueOrNull(const QByteArray& array, const int index = 0, const int16_t default_value = 0)
{
    return cassvalue::int16ValueOrNull(array,index,default_value);
}

inline int16_t safeInt16Value(const QByteArray& array, const int index = 0, const int16_t default_value = 0)
{
    return cassvalue::safeInt16Value(array,index,default_value);
}

inline void appendUInt16Value(QByteArray& array, const uint16_t value)
{
    cassvalue::appendInt16Value(array, value);
}

inline void setUInt16Value(QByteArray& array, const uint16_t value)
{
    cassvalue::setInt16Value(array, value);
}

inline uint16_t uint16Value(const QByteArray& array, const int index = 0)
{
    return cassvalue::uint16Value(array,index);
}

inline uint16_t uint16ValueOrNull(const QByteArray& array, const int index = 0, const uint16_t default_value = 0)
{
    return cassvalue::uint16ValueOrNull(array,index,default_value);
}

inline uint16_t safeUInt16Value(const QByteArray& array, const int index = 0, const uint16_t default_value = 0)
{
    return cassvalue::safeUInt16Value(array,index,default_value);
}

// Int32
inline void appendInt32Value(QByteArray& array, const int32_t value)
{
    cassvalue::appendInt32Value(array,value);
}

inline void setInt32Value(QByteArray& array, const int32_t value)
{
    cassvalue::setInt32Value(array,value);
}

inline void replaceInt32Value(QByteArray& array, const int32_t value, const int index = 0)
{
    return cassvalue::replaceInt32Value(array,value,index);
}

inline int32_t int32Value(const QByteArray& array, const int index = 0)
{
    return cassvalue::int32Value(array,index);
}

inline int32_t int32ValueOrNull(const QByteArray& array, const int index = 0, const int32_t default_value = 0)
{
    return cassvalue::int32ValueOrNull(array,index,default_value);
}

inline int32_t safeInt32Value(const QByteArray& array, const int index = 0, const int32_t default_value = 0)
{
    return cassvalue::safeInt32Value(array,index,default_value);
}

inline void appendUInt32Value(QByteArray& array, const uint32_t value)
{
    cassvalue::appendInt32Value(array, value);
}

inline void setUInt32Value(QByteArray& array, const uint32_t value)
{
    cassvalue::setInt32Value(array, value);
}

inline void replaceUInt32Value(QByteArray& array, const uint32_t value, const int index = 0)
{
    cassvalue::replaceInt32Value(array, value, index);
}

inline uint32_t uint32Value(const QByteArray& array, const int index = 0)
{
    return cassvalue::uint32Value(array,index);
}

inline uint32_t uint32ValueOrNull(const QByteArray& array, const int index = 0, const uint32_t default_value = 0)
{
    return cassvalue::uint32ValueOrNull(array,index,default_value);
}

inline uint32_t safeUInt32Value(const QByteArray& array, const int index = 0, const uint32_t default_value = 0)
{
    return cassvalue::safeUInt32Value(array,index,default_value);
}

// Int64
inline void appendInt64Value(QByteArray& array, const int64_t value)
{
    cassvalue::appendInt64Value(array,value);
}

inline void setInt64Value(QByteArray& array, const int64_t value)
{
    cassvalue::setInt64Value(array,value);
}

inline int64_t int64Value(const QByteArray& array, const int index = 0)
{
    return cassvalue::int64Value(array,index);
}

inline int64_t int64ValueOrNull(const QByteArray& array, const int index = 0, const int64_t default_value = 0)
{
    return cassvalue::int64ValueOrNull(array,index,default_value);
}

inline int64_t safeInt64Value(const QByteArray& array, const int index = 0, const int64_t default_value = 0)
{
    return cassvalue::safeInt64Value(array,index,default_value);
}

inline void appendUInt64Value(QByteArray& array, const uint64_t value)
{
    cassvalue::appendInt64Value(array, value);
}

inline void setUInt64Value(QByteArray& array, const uint64_t value)
{
    cassvalue::setInt64Value(array, value);
}

inline uint64_t uint64Value(const QByteArray& array, const int index = 0)
{
    return cassvalue::uint64Value(array,index);
}

inline uint64_t uint64ValueOrNull(const QByteArray& array, const int index = 0, const uint64_t default_value = 0)
{
    return cassvalue::uint64ValueOrNull(array,index,default_value);
}

inline uint64_t safeUInt64Value(const QByteArray& array, const int index = 0, const uint64_t default_value = 0)
{
    return cassvalue::safeUInt64Value(array,index,default_value);
}

// Float
inline void setFloatValue(QByteArray& array, const float value)
{
    cassvalue::setFloatValue(array,value);
}

inline void appendFloatValue(QByteArray& array, const float value)
{
    cassvalue::appendFloatValue(array,value);
}

inline float floatValue(const QByteArray& array, const int index = 0)
{
    return cassvalue::floatValue(array,index);
}

inline float floatValueOrNull(const QByteArray& array, const int index = 0, const float default_value = 0)
{
    return cassvalue::floatValueOrNull(array,index,default_value);
}

inline float safeFloatValue(const QByteArray& array, const int index = 0, const float default_value = 0)
{
    return cassvalue::safeFloatValue(array,index,default_value);
}

// Double
inline void setDoubleValue(QByteArray& array, const double value)
{
    cassvalue::setDoubleValue(array,value);
}

inline void appendDoubleValue(QByteArray& array, const double value)
{
    cassvalue::appendDoubleValue(array,value);
}

inline double doubleValue(const QByteArray& array, const int index = 0)
{
    return cassvalue::doubleValue(array,index);
}

inline double doubleValueOrNull(const QByteArray& array, const int index = 0, const double default_value = 0)
{
    return cassvalue::doubleValueOrNull(array,index,default_value);
}

inline double safeDoubleValue(const QByteArray& array, const int index = 0, const double default_value = 0.0)
{
    return cassvalue::safeDoubleValue(array,index,default_value);
}

// String
inline void setStringValue(QByteArray& array, const QString& value)
{
    cassvalue::setStringValue(array,value);
}

inline void appendStringValue(QByteArray& array, const QString& value)
{
    cassvalue::appendStringValue(array,value);
}

inline QString stringValue(const QByteArray& array, const int index = 0, int size = -1)
{
    return cassvalue::stringValue(array,index,size);
}

// Binary
inline void setBinaryValue(QByteArray& array, const QByteArray& value)
{
    cassvalue::setBinaryValue(array,value);
}

inline void appendBinaryValue(QByteArray& array, const QByteArray& value)
{
    cassvalue::appendBinaryValue(array,value);
}

inline QByteArray binaryValue(const QByteArray& array, const int index = 0, int size = -1)
{
    return cassvalue::binaryValue(array,index,size);
}




class QCassandraEncoder : public cassvalue::Encoder
{
public:
    QCassandraEncoder(int reserve_size) : cassvalue::Encoder(reserve_size) {}
};


class QCassandraDecoder : public cassvalue::Decoder
{
public:
    QCassandraDecoder(QByteArray const & encoded) : cassvalue::Decoder(encoded) {}
};



/*******************************************************************************
 * value
 *******************************************************************************/


class value : public cassvalue::Value
{
public:
    static const int32_t    TTL_PERMANENT = 0;

    // TTL must be positive, although Cassandra allows 0 as "permanent"
    typedef int32_t         cassandra_ttl_t;

    enum timestamp_mode_t {
        TIMESTAMP_MODE_CASSANDRA,
        TIMESTAMP_MODE_AUTO,
        TIMESTAMP_MODE_DEFINED
    };

    // CASSANDRA_VALUE_TYPE_BINARY (empty buffer)
    value();

    // CASSANDRA_VALUE_TYPE_INTEGER
    value(bool value);
    value(char value);
    value(signed char value);
    value(unsigned char value);
    value(int16_t value);
    value(uint16_t value);
    value(int32_t value);
    value(uint32_t value);
    value(int64_t value);
    value(uint64_t value);

    // CASSANDRA_VALUE_TYPE_FLOAT
    value(float value);
    value(double value);

    // CASSANDRA_VALUE_TYPE_STRING
    value(const QString& value);

    // CASSANDRA_VALUE_TYPE_BINARY
    value(const QByteArray& value);
    value(const char *data, int size);

    bool operator == (const value& rhs);
    bool operator != (const value& rhs);

    int32_t ttl() const;
    void setTtl(int32_t ttl = TTL_PERMANENT);

    consistency_level_t consistencyLevel() const;
    void setConsistencyLevel(consistency_level_t level);

private:
    cassandra_ttl_t             f_ttl = TTL_PERMANENT;
    consistency_level_t         f_consistency_level = CONSISTENCY_LEVEL_DEFAULT;
    timestamp_mode_t            f_timestamp_mode = TIMESTAMP_MODE_AUTO;
    int64_t                     f_timestamp = 0;
};



} // namespace libdbproxy
// vim: ts=4 sw=4 et
