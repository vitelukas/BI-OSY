// The classes in this header define the common interface between your implementation and
// the testing environment. Exactly the same implementation is present in the progtest's
// testing environment. You are not supposed to modify any declaration in this file,
// any change is likely to break the compilation.
#ifndef COMMON_H_98273465234756283645234625356
#define COMMON_H_98273465234756283645234625356

#include "bigint.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================================================================
/**
 * A simlpe representation of a 2D point
 */
class CPoint {
public:
    CPoint(int x, int y)
        : m_X(x), m_Y(y) {}

    int m_X;
    int m_Y;
    std::strong_ordering operator<=>(const CPoint &other) const = default;
    friend std::ostream &operator<<(std::ostream &os, const CPoint &x) {
        return os << '[' << x.m_X << ", " << x.m_Y << ']';
    }
};
//=============================================================================================================================================================
/**
 * An encapsulation of a polygon. The problem is defined by a set of points (m_Points). We are interested in the cheapest
 * triangulation (TriangMin problem) or we want to compute the total number of various triangulations (TriangCnt problem).
 * The computation stores the wanted result into the corresponding member variable.
 *
 */
class CPolygon {
public:
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    CPolygon() = default;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    CPolygon(std::vector<CPoint> points)
        : m_Points(std::move(points)) {}
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    CPolygon &add(CPoint p) {
        m_Points.push_back(p);
        return *this;
    }
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    std::vector<CPoint> m_Points;
    double m_TriangMin = 0;
    CBigInt m_TriangCnt;
};
using APolygon = std::shared_ptr<CPolygon>;
//=============================================================================================================================================================
/**
 * A batch of problems to solve. The solver is expected to iterate over the stored polygons and compute the expected
 * results. TriangMin is required for polygons in m_ProblemsMin, TriangCnt is required for polygons in m_ProblemsCnt.
 */
class CProblemPack {
public:
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    virtual ~CProblemPack(void) noexcept = default;
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    void addMin(APolygon polygon) {
        m_ProblemsMin.push_back(std::move(polygon));
    }
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    void addCnt(APolygon polygon) {
        m_ProblemsCnt.push_back(std::move(polygon));
    }
    //---------------------------------------------------------------------------------------------------------------------------------------------------------
    std::vector<APolygon> m_ProblemsMin;
    std::vector<APolygon> m_ProblemsCnt;
};
using AProblemPack = std::shared_ptr<CProblemPack>;
//=============================================================================================================================================================
/**
 * CCompany: deliver problem packs to solve & receive solved problem packs.
 *
 */
class CCompany {
public:
    virtual ~CCompany(void) noexcept = default;
    virtual AProblemPack waitForPack(void) = 0;
    virtual void solvedPack(AProblemPack pack) = 0;
};
using ACompany = std::shared_ptr<CCompany>;
//=============================================================================================================================================================
#endif /* COMMON_H_98273465234756283645234625356 */
