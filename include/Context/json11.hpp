/* json11
 *
 * json11 is a tiny JSON library for C++11, providing JSON parsing and serialization.
 *
 * The core object provided by the library is json11::Json. A Json object represents any JSON
 * value: null, bool, number (int or double), string (std::string), array (std::vector), or
 * object (std::map).
 *
 * Json objects act like values: they can be assigned, copied, moved, compared for equality or
 * order, etc. There are also helper methods Json::dump, to serialize a Json to a string, and
 * Json::parse (static) to parse a std::string as a Json object.
 *
 * Internally, the various types of Json object are represented by the JsonValue class
 * hierarchy.
 *
 * A note on numbers - JSON specifies the syntax of number formatting but not its semantics,
 * so some JSON implementations distinguish between integers and floating-point numbers, while
 * some don't. In json11, we choose the latter. Because some JSON implementations (namely
 * Javascript itself) treat all numbers as the same type, distinguishing the two leads
 * to JSON that will be *silently* changed by a round-trip through those implementations.
 * Dangerous! To avoid that risk, json11 stores all numbers as double internally, but also
 * provides integer helpers.
 *
 * Fortunately, double-precision IEEE754 ('double') can precisely store any integer in the
 * range +/-2^53, which includes every 'int' on most systems. (Timestamps often use int64
 * or long long to avoid the Y2038K problem; a double storing microseconds since some epoch
 * will be exact for +/- 275 years.)
 */

/* Copyright (c) 2013 Dropbox, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

#ifdef _MSC_VER
#define JSON11_NOEXCEPT
#else
#define JSON11_NOEXCEPT noexcept
#endif

namespace json11
{
	class JsonValue;

	class Json final
	{
	public:
		// Types
		enum Type
		{
			NUL, NUMBER, BOOL, STRING, ARRAY, OBJECT
		};

		// Array and object typedefs
		typedef std::vector<Json> array;
		typedef std::map<std::string, Json> object;

		// Constructors for the various types of JSON value.
		inline Json() JSON11_NOEXCEPT;                // NUL
		inline Json(std::nullptr_t) JSON11_NOEXCEPT;  // NUL
		inline Json(double value);                    // NUMBER
		inline Json(int value);                       // NUMBER
		inline Json(bool value);                      // BOOL
		inline Json(const std::string &value);        // STRING
		inline Json(std::string &&value);             // STRING
		inline Json(const char * value);              // STRING
		inline Json(const array &values);             // ARRAY
		inline Json(array &&values);                  // ARRAY
		inline Json(const object &values);            // OBJECT
		inline Json(object &&values);                 // OBJECT

		// Implicit constructor: anything with a to_json() function.
		template <class T, class = decltype(&T::to_json)>
		Json(const T & t) : Json(t.to_json()) {}

		// Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
		template <class M, typename std::enable_if<
			std::is_constructible<std::string, typename M::key_type>::value
			&& std::is_constructible<Json, typename M::mapped_type>::value,
			int>::type = 0>
			Json(const M & m) : Json(object(m.begin(), m.end())) {}

		// Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
		template <class V, typename std::enable_if<
			std::is_constructible<Json, typename V::value_type>::value,
			int>::type = 0>
			Json(const V & v) : Json(array(v.begin(), v.end())) {}

		// This prevents Json(some_pointer) from accidentally producing a bool. Use
		// Json(bool(some_pointer)) if that behavior is desired.
		Json(void *) = delete;

		// Accessors
		inline Type type() const;

		inline bool is_null()   const { return type() == NUL; }
		inline bool is_number() const { return type() == NUMBER; }
		inline bool is_bool()   const { return type() == BOOL; }
		inline bool is_string() const { return type() == STRING; }
		inline bool is_array()  const { return type() == ARRAY; }
		inline bool is_object() const { return type() == OBJECT; }

		// Return the enclosed value if this is a number, 0 otherwise. Note that json11 does not
		// distinguish between integer and non-integer numbers - number_value() and int_value()
		// can both be applied to a NUMBER-typed object.
		inline double number_value() const;
		inline int int_value() const;

		// Return the enclosed value if this is a boolean, false otherwise.
		inline bool bool_value() const;
		// Return the enclosed string if this is a string, "" otherwise.
		inline const std::string &string_value() const;
		// Return the enclosed std::vector if this is an array, or an empty vector otherwise.
		inline const array &array_items() const;
		// Return the enclosed std::map if this is an object, or an empty map otherwise.
		inline const object &object_items() const;

		// Return a reference to arr[i] if this is an array, Json() otherwise.
		inline const Json & operator[](size_t i) const;
		// Return a reference to obj[key] if this is an object, Json() otherwise.
		inline const Json & operator[](const std::string &key) const;

		// Serialize.
		inline void dump(std::string &out) const;
		std::string dump() const
		{
			std::string out;
			dump(out);
			return out;
		}

		// Parse. If parse fails, return Json() and assign an error message to err.
		inline static Json parse(const std::string & in, std::string & err);
		static Json parse(const char * in, std::string & err)
		{
			if (in)
			{
				return parse(std::string(in), err);
			}
			else
			{
				err = "null input";
				return nullptr;
			}
		}
		// Parse multiple objects, concatenated or separated by whitespace
		inline static std::vector<Json> parse_multi(const std::string & in, std::string & err);

		inline bool operator== (const Json &rhs) const;
		inline bool operator<  (const Json &rhs) const;
		bool operator!= (const Json &rhs) const { return !(*this == rhs); }
		bool operator<= (const Json &rhs) const { return !(rhs < *this); }
		bool operator>(const Json &rhs) const { return  (rhs < *this); }
		bool operator>= (const Json &rhs) const { return !(*this < rhs); }

		/* has_shape(types, err)
		 *
		 * Return true if this is a JSON object and, for each item in types, has a field of
		 * the given type. If not, return false and set err to a descriptive message.
		 */
		typedef std::initializer_list<std::pair<std::string, Type>> shape;
		inline bool has_shape(const shape & types, std::string & err) const;

	private:
		std::shared_ptr<JsonValue> m_ptr;
	};

	// Internal class hierarchy - JsonValue objects are not exposed to users of this API.
	class JsonValue
	{
	protected:
		friend class Json;
		friend class JsonInt;
		friend class JsonDouble;
		virtual Json::Type type() const = 0;
		virtual bool equals(const JsonValue * other) const = 0;
		virtual bool less(const JsonValue * other) const = 0;
		virtual void dump(std::string &out) const = 0;
		inline virtual double number_value() const;
		inline virtual int int_value() const;
		inline virtual bool bool_value() const;
		inline virtual const std::string &string_value() const;
		inline virtual const Json::array &array_items() const;
		inline virtual const Json &operator[](size_t i) const;
		inline virtual const Json::object &object_items() const;
		inline virtual const Json &operator[](const std::string &key) const;
		virtual ~JsonValue() {}
	};

} // namespace json11

#include <Context/json11.ipp>