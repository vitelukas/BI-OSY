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
    size_t m_packID = 0;
    bool solved = false;
    shared_ptr<SCompany> m_owner = nullptr;
    size_t m_solvedProbsCounter = 0;
    size_t m_numOfProbs = 0;

    bool operator<(const SProbPack &other) const {
        // Comparing based on packID
        return m_packID > other.m_packID; // Note: using '>' to create a min heap
    }
};

struct SCompany {

    SCompany(ACompany company)
        : m_company(move(company)) {}

    ACompany m_company;
    priority_queue<SProbPack> m_problemPacks;
    mutex m_PackagesMutex;
    condition_variable m_SendPacksCV;
    size_t m_nxtPackID = 0;
};

struct SSolver {
    SSolver() = default;

    AProgtestSolver m_solver;
    map<shared_ptr<SProbPack>, size_t> m_counts;
};

//| ------------------------------------------------------------------------------
class COptimizer {
public:
    COptimizer() = default;

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

    void commThrRecv(shared_ptr<SCompany> Scomp);
    void commThrSend(shared_ptr<SCompany> Scomp);
    void workThr();
    void processPack(shared_ptr<SProbPack> packPtr);

private:
    vector<shared_ptr<SCompany>> m_Companies;
    vector<thread> m_workThreads;
    vector<thread> m_commThreads;
    queue<SSolver> m_ReadySolvers;

    SSolver m_SolverMin;
    SSolver m_SolverCnt;

    // todo - solve the flags -> vyresit 2 pripady ukonceni:
    // todo     - kdyz se postupne ukonci vsechny komm vlakna, protoze jim prestanou chodit balicky
    // todo     - kdyz se zavola stop() a je potreba postupne vsechno ukoncit
    // flag to indicate that the company is still receiving packages
    // - the flag will be set to false when the stop() method is called or when the company receives a nullptr in a package
    bool isReceiving = true;
    size_t m_numOfActiveRecvThreads = 0;
    // flag to indicate that the shift is over
    // - flag will be set to true when the stop() method is called
    bool endOfShift = false;
    // flag to indicate that the worker threads are still working - solving packageds
    // - the flag will be set to false when endOfShift == true && m_ReadySolvers.empty() == true
    bool isWorking = true;

    mutex m_QueueMutex;
    condition_variable m_ProcessPacksCV;
};

// Register a new company
void COptimizer::addCompany(ACompany company) {
    m_Companies.emplace_back(make_shared<SCompany>(company));
}

// Initialize all threads
void COptimizer::start(int threadCount) {
    // for all companies make 2 communication threads
    //	 - one for receiving and one for sending
    for (auto &company : m_Companies) {
        m_commThreads.emplace_back(thread(&COptimizer::commThrRecv, this, company));
        m_numOfActiveRecvThreads++;
        m_commThreads.emplace_back(thread(&COptimizer::commThrSend, this, company));
    }

    // initialize worker threads - which will be shared between all companies
    for (int i = 0; i < threadCount; i++) {
        m_workThreads.emplace_back(thread(&COptimizer::workThr, this));
    }
}

// TODO =====
void COptimizer::stop(void) {
    endOfShift = true;

    { // thread workflow notes

        // pracovni vlakna
        // -  while loop bezi jen do ty doby, dokud je neco v bufferu && endOfShift == false
        // -> jakmile endOfShift == true, tak zpracuji zbytek bufferu s fullSolvers a pak skonci

        // komunikacni vlakna
        //	 prijimaci
        // 		-  while loop bezi jen do ty doby, dokud endOfShift == false || isReceiving == true
        //	 odesilaci
        // 		-  while loop bezi jen do ty doby, dokud isReceiving == true || priority_queue neni prazdna
        //===================================
        //? pripad kdy uz jsou aktivni jenom worker threads
        //? 	- vsechny komunikacni vlakna uz skoncila protoze jim vsem prisel nullpointer
        // notify all worker threads
        //	 - worker threads se vzbudi, ale zjisti, ze uz je endOfShift a zaroven uz neni co na praci --> ukonci se
        // wait for all worker threads to join
        //? pripad kdy jsou jeste aktivni komunikacni vlakna i pracovni vlakna
        // - while loop v komunikacnich vlaknech bezi jen do urcity doby, viz. vyse
        // - jakmiele endOfShift == true, tak se dodelaji zbyly problemy a pak komunikacni vlakna skonci
        // --> pripoji se k main vlaknu
    }

    for (auto &commThread : m_commThreads) {
        commThread.join();
    }
    for (auto &workThread : m_workThreads) {
        m_ProcessPacksCV.notify_one();
        workThread.join();
    }
}

//| ------------------------------------------------------custom helper functions------------------------------------------------------

// TODO ====
void COptimizer::commThrRecv(shared_ptr<SCompany> Scomp) {
    while (true) {
        AProblemPack newPack = Scomp->m_company->waitForPack();
        if (!newPack || endOfShift) {
            m_numOfActiveRecvThreads--;
            m_numOfActiveRecvThreads == 0 ? isReceiving = false : isReceiving = true;
            // todo - if(isReceiving == false)
            // -> pokud nejsou prazdne, tak poslat oba dva solvery do bufferu, a notify worker threads
            return;
        }
        // initialize a new problem pack in the company
        SProbPack pack;
        pack.m_problemPack = newPack;
        pack.m_packID = Scomp->m_nxtPackID++;
        pack.m_owner = Scomp;
        pack.m_numOfProbs = newPack->m_ProblemsCnt.size() + newPack->m_ProblemsMin.size();
        Scomp->m_problemPacks.push(pack);
        {
            lock_guard<mutex> lock(m_QueueMutex);
            // todo - vysypat problemy z packu do solveru
            // - pokud je solver plny, poslat ho do bufferu, vytvorit novy a notify worker threads
            // - do noveho solveru vysypat zbytek problemu z packu a znova to same
            m_ProcessPacksCV.notify_one();
        }
    }
}

// TODO ====
void COptimizer::commThrSend(shared_ptr<SCompany> Scomp) {
    while (true) {
        {
            unique_lock<mutex> lock(Scomp->m_PackagesMutex);
            Scomp->m_SendPacksCV.wait(lock, [this, &Scomp] { return !Scomp->m_problemPacks.empty() || isWorking; });
            if (Scomp->m_problemPacks.empty() && isWorking)
                continue;
            else if (Scomp->m_problemPacks.empty() && !isWorking)
                // The company stopped sending packages and there are no more packages to be sent -> terminate the tread
                return;
            else {
                SProbPack topPack = Scomp->m_problemPacks.top();
                while (topPack.solved) {
                    Scomp->m_problemPacks.pop();
                    Scomp->m_company->solvedPack(topPack.m_problemPack);
                    topPack = Scomp->m_problemPacks.top();
                }
            }
        }
    }
}

// TODO ====
void COptimizer::workThr(void) {
    // wait for a notification from the comm threads
    // 	- the comm thread will send a notification when a new full solver is added to the buffer
    // if the queue is not empty:
    // 		- nejdriv se pusti solve() -> vyresej se vsechny problemy v solveru
    //      - pak projizdim for loopem celej vektor counts, nejdriv incrementuju solvedProblemsCounter v SProblemPack
    //      - pak zkotroluju jestli solvedProblemsCounter == numOfProbs, kdyz jo => solved = true
    //      - POTOM pingnu ownercompany
    // if the queue is empty but the shift is not over yet or the packages are still being received (endOfShift == false || isReceiving == true), go to sleep and wait for a notification
    // if the queue is empty and the shift is over and no packages are being received (endOfShift == true && isReceiving == false),
    //~ terminate the thread  ~~~~ AND SET isWorking = false ~~~~
    while (true) {
        // todo
        // m_ProcessPacksCV.wait(lock, [this] { return !m_PacksQueue.empty() || !isReceiving; });
    }
}

// Process a whole problem pack which will be sent from a worker thread
void COptimizer::processPack(shared_ptr<SProbPack> packPtr) {
}

//! ---------------------------------------------------------------------------END-------------------------------------------------------------------------------------------------
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
