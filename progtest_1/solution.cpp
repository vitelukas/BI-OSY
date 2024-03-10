#ifndef __PROGTEST__
#include "progtest_solver.h"
#include "sample_tester.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cfloat>
#include <chrono>
#include <climits>
#include <cmath>
#include <compare>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <set>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace std;
#endif /* __PROGTEST__ */

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
class COptimizer {
public:
    static bool usingProgtestSolver(void) {
        return true;
    }
    static void checkAlgorithmMin(APolygon p) {
        // dummy implementation if usingProgtestSolver() returns true
    }
    static void checkAlgorithmCnt(APolygon p) {
        // dummy implementation if usingProgtestSolver() returns true
    }
    void start(int threadCount);
    void stop(void);
    void addCompany(ACompany company);

private:
};
// TODO: COptimizer implementation goes here

void COptimizer::start(int threadCount) {
}

void COptimizer::stop(void) {
}

void COptimizer::addCompany(ACompany company) {
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__
int main(void) {
    COptimizer optimizer;
    ACompanyTest company = std::make_shared<CCompanyTest>();
    optimizer.addCompany(company);
    optimizer.start(4);
    optimizer.stop();
    if (!company->allProcessed())
        throw std::logic_error("(some) problems were not correctly processsed");
    return 0;
}
#endif /* __PROGTEST__ */
