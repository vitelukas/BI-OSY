// The classes in this header are used in the example test only. You are free to
// modify these classes, add more test cases, and add more test sets.
// These classes do not exist in the Progtest's testing environment.
#ifndef SAMPLE_TESTER_H_2983745628345129345
#define SAMPLE_TESTER_H_2983745628345129345

#include "common.h"

//=============================================================================================================================================================
/**
 * An example CCompany implementation suitable for testing. This implementation does not cause
 * any delays in wait/solved calls.
 */
class CCompanyTest : public CCompany {
public:
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * A basic implementation of the waitForPack method from the base class.
     *
     * @return a new problem pack, or an empty smart pointer
     */
    AProblemPack waitForPack() override;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * A basic implementation of the solvedPack method from the base class.
     *
     * @param[in] pack        a pack to return and validate
     */
    void solvedPack(AProblemPack pack) override;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    /**
     * Check exit status:
     *  - were all packs read?
     *  - were all packs returned?
     * @return true if all packs were read and returned (does not check the results in the returned packs).
     */
    bool allProcessed() const;

private:
    size_t m_MinPos{0};
    size_t m_MinDone{0};
    size_t m_CntPos{0};
    size_t m_CntDone{0};
};
using ACompanyTest = std::shared_ptr<CCompanyTest>;
//=============================================================================================================================================================
#endif /* SAMPLE_TESTER_H_2983745628345129345 */
