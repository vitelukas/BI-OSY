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

//! -------------------------------------------------------------------------START-------------------------------------------------------------------------------------------------
struct SCompany;

struct SProbPack {
    SProbPack() = default;

    AProblemPack m_problemPack;
    shared_ptr<SCompany> m_owner = nullptr;
    mutex m_packMutex;
    size_t m_solvedProbsCounter = 0;
    size_t m_numOfProbs = 0;
    bool solved = false;
};

struct SCompany {

    SCompany(ACompany company)
        : m_company(move(company)) {}

    ACompany m_company;
    queue<shared_ptr<SProbPack>> m_problemPacks;
    mutex m_packagesMutex;
    condition_variable m_sendPacksCV;
};

struct SSolver {
    SSolver() = default;

    AProgtestSolver m_solver = nullptr;
    // Keps count how many problems from a single problem pack
    // were added into the current solver instance
    unordered_map<shared_ptr<SProbPack>, size_t> m_counts = {};
};

//| ------------------------------------------------------------------------------------------
class COptimizer {
public:
    COptimizer() {
        m_SolverMin.m_solver = createProgtestMinSolver();
        m_SolverCnt.m_solver = createProgtestCntSolver();
    }

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
    void commThrRecv(shared_ptr<SCompany> Scomp);
    void loadProbPckMin(shared_ptr<SProbPack> packPtr);
    void loadProbPckCnt(shared_ptr<SProbPack> packPtr);
    void commThrSend(shared_ptr<SCompany> Scomp);
    void workThr();
    void handleLastRecvThread();
    void handleEndingWorkThread();

    vector<shared_ptr<SCompany>> m_Companies;
    vector<thread> m_workThreads;
    vector<thread> m_commReceiveThreads;
    vector<thread> m_commSendThreads;
    queue<SSolver> m_ReadySolversQue;

    SSolver m_SolverMin;
    SSolver m_SolverCnt;

    // flag to indicate that the company is still receiving packages
    // - the flag will be set to false when the stop() method is called or when the company receives a nullptr in a package
    bool isReceiving = true;
    atomic<size_t> m_numOfInactiveRecvThreads = 0;
    // flag to indicate that the shift is over
    // - flag will be set to true when the stop() method is called
    bool endOfShift = false;
    // flag to indicate that the worker threads are still working - solving packageds
    // - the flag will be set to false when endOfShift == true && m_ReadySolversQue.empty() == true
    bool workersActive = true;
    atomic<size_t> m_numOfActiveWorkThreads = 0;

    mutex m_SolversQueMutex;
    mutex m_SolverMinMutex;
    mutex m_SolverCntMutex;
    condition_variable m_SolvePacksCV;
};
//| ------------------------------------------------------------------------------------------
// Register a new company
void COptimizer::addCompany(ACompany company) {
    m_Companies.emplace_back(make_shared<SCompany>(company));
}

// Initialize all threads
void COptimizer::start(int threadCount) {
    // for all companies make 2 communication threads
    //	 - one for receiving and one for sending packages
    for (auto &company : m_Companies) {
        m_commReceiveThreads.emplace_back(thread(&COptimizer::commThrRecv, this, company));
        m_commSendThreads.emplace_back(thread(&COptimizer::commThrSend, this, company));
    }

    // initialize worker threads - which will be shared between all companies
    for (int i = 0; i < threadCount; i++) {
        m_workThreads.emplace_back(thread(&COptimizer::workThr, this));
        m_numOfActiveWorkThreads++;
    }
}

void COptimizer::stop(void) {
    endOfShift = true;

    // Wait for all communication to receive the package they are actually waiting for
    // and then let them terminate themselves and join the main thread
    for (auto &receiverThread : m_commReceiveThreads) {
        receiverThread.join();
    }

    m_SolvePacksCV.notify_all();
    for (auto &workThread : m_workThreads) {
        workThread.join();
    }

    for (auto &company : m_Companies) {
        company->m_sendPacksCV.notify_all();
    }
    for (auto &senderThread : m_commSendThreads) {
        senderThread.join();
    }
}

//| ------------------------------------------------------custom helper functions------------------------------------------------------

void COptimizer::commThrRecv(shared_ptr<SCompany> Scomp) {
    while (true) {
        AProblemPack newPack = Scomp->m_company->waitForPack();

        if (!newPack) {
            handleLastRecvThread();
            return;
        }

        // initialize a new problem pack in the company storage
        shared_ptr<SProbPack> packPtr = make_shared<SProbPack>();
        packPtr->m_problemPack = newPack;
        packPtr->m_owner = Scomp;
        packPtr->m_numOfProbs = (newPack->m_ProblemsCnt.size() + newPack->m_ProblemsMin.size());
        Scomp->m_packagesMutex.lock();
        Scomp->m_problemPacks.push(packPtr);
        Scomp->m_packagesMutex.unlock();

        //| Also possible to create a helper thread here, which would simultaneously add problems from
        //| the CNT vector in the new pack into the CNT solver
        //? Handle all MIN problems
        unique_lock<mutex> lockMin(m_SolverMinMutex);
        loadProbPckMin(packPtr);
        lockMin.unlock();

        //? Handle all CNT problems
        unique_lock<mutex> lockCnt(m_SolverCntMutex);
        loadProbPckCnt(packPtr);
        lockCnt.unlock();
    }
}

void COptimizer::loadProbPckMin(shared_ptr<SProbPack> packPtr) {
    for (auto &polygon : packPtr->m_problemPack->m_ProblemsMin) {
        m_SolverMin.m_solver->addPolygon(polygon);
        m_SolverMin.m_counts[packPtr]++;

        if (m_SolverMin.m_solver->hasFreeCapacity())
            continue;

        // if the current solver is full -> push it into the buffer and create a new one
        unique_lock<mutex> queLock(m_SolversQueMutex);
        m_ReadySolversQue.push(m_SolverMin);
        queLock.unlock();
        m_SolvePacksCV.notify_one();
        m_SolverMin.m_solver = createProgtestMinSolver();
        m_SolverMin.m_counts.clear();
    }
}

void COptimizer::loadProbPckCnt(shared_ptr<SProbPack> packPtr) {
    for (auto &polygon : packPtr->m_problemPack->m_ProblemsCnt) {
        m_SolverCnt.m_solver->addPolygon(polygon);
        m_SolverCnt.m_counts[packPtr]++;

        if (m_SolverCnt.m_solver->hasFreeCapacity())
            continue;

        // if the current solver is full -> push it into the buffer and create a new one
        unique_lock<mutex> queLock(m_SolversQueMutex);
        m_ReadySolversQue.push(m_SolverCnt);
        queLock.unlock();
        m_SolvePacksCV.notify_one();
        m_SolverCnt.m_solver = createProgtestCntSolver();
        m_SolverCnt.m_counts.clear();
    }
}

void COptimizer::handleLastRecvThread() {
    m_numOfInactiveRecvThreads++;
    m_numOfInactiveRecvThreads.load() == m_Companies.size() ? isReceiving = false : 0;
    if (isReceiving == false) {
        unique_lock<mutex> queLock(m_SolversQueMutex);
        m_ReadySolversQue.push(m_SolverMin);
        m_ReadySolversQue.push(m_SolverCnt);
        m_SolvePacksCV.notify_all();
    }
}

void COptimizer::workThr(void) {
    while (true) {
        unique_lock<mutex> queLock(m_SolversQueMutex);
        //? Stay awake until there are some ready solvers in the buffer to be picked up
        m_SolvePacksCV.wait(queLock, [this] { return m_ReadySolversQue.empty() == false || isReceiving == false; });
        if (m_ReadySolversQue.empty() && !isReceiving && !endOfShift) {
            //? The queue is empty and the communication threads are not receiving any next packages,
            //? but the shift is not over yet -> go to sleep and wait for a notification
            m_SolvePacksCV.wait(queLock, [this] { return endOfShift; });
            //? Stop() hase been called -> terminating the thread
            handleEndingWorkThread();
            return;
        } else if (m_ReadySolversQue.empty() && endOfShift) {
            //? The shift is over and all packs from the solvers in the buffer were solved
            //? -> terminated the thread
            handleEndingWorkThread();
            return;
        }

        SSolver readySolverStruct = m_ReadySolversQue.front();
        m_ReadySolversQue.pop();
        queLock.unlock();
        readySolverStruct.m_solver->solve();
        for (auto &pack : readySolverStruct.m_counts) {
            unique_lock<mutex> packLock(pack.first->m_packMutex);
            pack.first->m_solvedProbsCounter += pack.second;
            if (pack.first->m_numOfProbs == pack.first->m_solvedProbsCounter) {
                pack.first->solved = true;
                pack.first->m_owner->m_sendPacksCV.notify_one();
            }
        }
    }
}

void COptimizer::handleEndingWorkThread() {
    m_numOfActiveWorkThreads--;
    if (m_numOfActiveWorkThreads.load() == 0)
        workersActive = false;
}

void COptimizer::commThrSend(shared_ptr<SCompany> Scomp) {
    while (true) {
        unique_lock<mutex> lock(Scomp->m_packagesMutex);
        Scomp->m_sendPacksCV.wait(lock, [this, &Scomp] { return (!Scomp->m_problemPacks.empty() && Scomp->m_problemPacks.front()->solved) || !workersActive; });
        if (Scomp->m_problemPacks.empty() && !workersActive) {
            //? The company stopped sending packages or stop() has been called
            //? and there are no more packages in the storage to be sent -> terminate the sender thread
            return;
        }
        //? Until there are some solved packages in the minimal heap, send them
        shared_ptr<SProbPack> topPack = Scomp->m_problemPacks.front();
        Scomp->m_company->solvedPack(topPack->m_problemPack);
        Scomp->m_problemPacks.pop();
    }
}

//! ---------------------------------------------------------------------------END-------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__
int main(void) {
    COptimizer optimizer;
    ACompanyTest company = std::make_shared<CCompanyTest>();
    for (size_t i = 0; i < 2; i++) {
        company = std::make_shared<CCompanyTest>();
        optimizer.addCompany(company);
    }
    optimizer.start(4);
    optimizer.stop();
    if (!company->allProcessed())
        throw std::logic_error("(some) problems were not correctly processsed");
    return 0;
}
#endif /* __PROGTEST__ */
