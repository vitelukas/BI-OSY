// The classes in this header define the common interface between your implementation and
// the testing environment. Exactly the same implementation is present in the progtest's
// testing environment. You are not supposed to modify any declaration in this file,
// any change is likely to break the compilation.
#ifndef bigint_h_22034590234652093456179823592
#define bigint_h_22034590234652093456179823592

#include <compare>
#include <cstdint>
#include <string>

//=============================================================================================================================================================
/**
 * CBigInt is a simple implementation of a big-number class. The class handles positive integers of
 * size BIGINT_BITS. Supported operations are initialization, addition, multiplication, comparison,
 * and a conversion-to-string.
 *
 * @note This interface is present in the progtest testing environment.
 * @note CBigInt class and the overloaded operators are available in all tests, even in the bonus tests.
 *
 */
class CBigInt {
public:
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Initialize an object with a 64 bit unsigned integer (zero extended in the upper bits).
     * @param[in] val        value to set
     */
    CBigInt(uint64_t val = 0);
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Initialize an object with a decimal integer.
     * @param[in] val        value to set
     * @exception std::invalid_argument if the input string is not a valid decimal integer, std::out_of_range if the input number exceeds CBigInt range
     */
    CBigInt(std::string_view val);
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Replace the contents with a 64 bit unsigned integer (zero extended in the upper bits).
     * @param[in] val        value to set
     * @return a reference to *this
     */
    CBigInt &operator=(uint64_t val);
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Convert the value into a string (decimal notation)
     * @return a string representation of *this
     */
    std::string toString() const;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Test whether the value is zero, or not
     * @return zero -> true, nonzero -> false
     */
    bool isZero() const;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Compare two big integers
     * @param[in] x          the value to compare to
     * @return one of: strong_ordering::less (*this < x), strong_ordering::equal (*this == x), or strong_ordering::greater (*this > x)
     * @note operator is not defaulted since lower order integers are stored first in m_Data
     */
    std::strong_ordering operator<=>(const CBigInt &x) const;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Compare two big integers for (non)equality. <=> is not used for == and != since <=> is not defaulted.
     * @param[in] x          the value to compare to
     * @return true for equality (identity), false otherwise
     */
    bool operator==(const CBigInt &x) const = default;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Add a CBigInt to *this
     * @param[in] x          the value to add
     * @return a reference to *this
     * @note overflow is ignored
     */
    CBigInt &operator+=(const CBigInt &x);
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Multiply *this wint another CBigInt
     * @param[in] x          the multiplier
     * @return a reference to *this
     * @note overflow is ignored
     */
    CBigInt &operator*=(const CBigInt &x);

private:
    static constexpr uint32_t BIGINT_BITS = 1024;
    static constexpr uint32_t BIGINT_INTS = BIGINT_BITS >> 5;
    uint32_t m_Data[BIGINT_INTS];
    void mulAdd(uint32_t st,
                const uint32_t v[],
                uint64_t mul);
    uint32_t divMod(uint32_t x);
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * A convenience wrapper - sum two big integers
 */
inline CBigInt operator+(CBigInt a,
                         const CBigInt &b) {
    return std::move(a += b);
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * A convenience wrapper - multiply two big integers
 */
inline CBigInt operator*(CBigInt a,
                         const CBigInt &b) {
    return std::move(a *= b);
}
//=============================================================================================================================================================
#endif /* bigint_h_22034590234652093456179823592 */
