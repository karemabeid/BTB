/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */
#include <vector>
#include <cmath>
#include "bp_api.h"
typedef enum{
    WNT = 1 ,SNT = 0 , WT = 2 , ST = 3
} StateOfFSM;

typedef enum {
    noShare = 0, lsbShare = 1, midShare = 2
} Share;
unsigned int PowerOfTwo(int p) {
    unsigned int result = 1;
    for (int i = 0; i < p; i++){
        result *= 2;
    }
    return result;
}

uint32_t calcEntry(uint32_t pc, int size){
    return (pc >> 2) % (size);
}
uint32_t calcTag(uint32_t pc, int btbSize, int tagSize){
    return ((pc>>2) / (btbSize)) % PowerOfTwo(tagSize)
}
int lsbShareHelper(uint32_t pc, int size){
    return (pc >> 2) % PowerOfTwo(size);
}
int midShareHelper(uint32_t pc, int size){
    return (pc >> 16) % PowerOfTwo(size);
}


class BranchPredictor{
private:
    int btbSize;
    int histRegSize;
    int tagSize;
    StateOfFSM initialFsmState;
    bool globalHistory;
    bool globalTables;
    Share shareType;
    uint32_t globalHistoryVec;
    StateOfFSM globalHistoryVecState;
    BranchPredictor();

public:
    vector <uint32_t> tags; //for tags when using local history
    vector <uint32_t> targetPCs;
    vector <uint32_t> localHistories; // history for each tag when using local history
    vector <vector<StateOfFSM>> localFSMs; // state machine for each tag when using local history
    vector <StateOfFSM> GlobalFSM;//
    vector<bool> isABTBEntry;
    SIM_stats simStats

    BranchPredictor(BranchPredictor const &) = delete; // disable copy ctor
    void operator=(BranchPredictor const &) = delete; // disable = operator
    static BranchPredictor &getInstance() // make BranchPredictor singleton
    {
        static BranchPredictor instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
////////////// setters getters //////////////
    int getBtbSize(){
        return btbSize;
    }
    void setBtbSize(int size){
        btbSize = size;
    }
    int getHistVecSize(){
        return histRegSize;
    }
    void setHistRegSize(int size){
        histRegSize = size;
    }
    int getTagSize(){
        return tagSize;
    }
    void setTagSize(int size){
        tagSize = size;
    }
    bool isGlobalHistory(){
        return globalHistory;
    }
    void setHistoryType(bool type){
        globalHistory = type;
    }
    bool isGlobalTable(){
        return globalTables;
    }
    void setTablesType(bool type){
        globalTables = type;
    }
    Share getShareType(){
        return shareType;
    }
    void setShareType(int type){
        shareType = type;
    }
    StateOfFSM getInitialFsmState() {
        return initialFsmState;
    }
    void setInitialFsmState(int state) {
        initialFsmState = state;
    }
    uint32_t getGlobalHistoryVec() {
        return globalHistoryVec
    }
    void setGlobalHistoryVec(uint32_t vec){
        globalHistoryVec = vec;
    }
    StateOfFSM getGlobalHistoryVecState(){
        retrun globalHistoryVecState;
    }
    void setGlobalHistoryVecState(int state){
        globalHistoryVecState = state;
    }
////////////// end of setters getters //////////////
};

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared){
    BranchPredictor& branchPredictor = BranchPredictor::getInstance();
    if (!((btbSize == 1) || (btbSize == 2) || (btbSize == 4) || (btbSize == 8) || (btbSize == 16) || (btbSize == 32))) return -1;
    if ((historySize > 8) || (historySize < 1))  return -1;
    if (((tagSize) < 0) || (tagSize > (30 - log2(btbSize)))) return -1;

    branchPredictor.setBtbSize(btbSize);
    branchPredictor.setHistRegSize(historySize);
    branchPredictor.setTagSize(tagSize);
    branchPredictor.setInitialFsmState(fsmState);
    branchPredictor.setHistoryType(isGlobalHist);
    branchPredictor.setTablesType(isGlobalTable);
    branchPredictor.setShareType(Share(Shared));
    branchPredictor.tags = vector<uint32_t>(btbSize, 0);
    branchPredictor.targets = vector<uint32_t>(btbSize, 0);
    branchPredictor.simStats = {0, 0, 0};
    branchPredictor.isABTBEntry = vector<bool>(btbSize, false);

    if (isGlobalTable) {
        branchPredictor.GlobalFSM = vector<StateOfFSM>(PowerOfTwo(historySize), (StateOfFSM)fsmState);
        // it is a Global table
    } else {
        branchPredictor.localFSMs = vector <vector<StateOfFSM>>(btbSize, vector<StateOfFSM>(PowerOfTwo(historySize), (StateOfFSM)fsmState));
        // each Entry has its own table
    }
    if (isGlobalHist){
        branchPredictor.setGlobalHistoryVec(0);
    } else {
        branchPredictor.localHistories = vector<uint32_t>(btbSize, 0);
    }
    return 0;
}


bool BP_predict(uint32_t pc, uint32_t *dst){
    BranchPredictor& branchPredictor = BranchPredictor::getInstance();
    uint32_t entry = calcEntry(pc, branchPredictor.getBtbSize());
    uint32_t tag = calcTag(pc, branchPredictor.getBtbSize(), branchPredictor.getTagSize());
    if(!branchPredictor.isABTBEntry[entry] || branchPredictor.tags[entry] != tag){
        *dst = pc + 4;
        return false;
    }
    // global history vector and a global table
    if(branchPredictor.isGlobalHistory() && branchPredictor.isGlobalTable()){
        int sharerHelper = 0;
        if(branchPredictor.getShareType() == lsbShare){
            sharerHelper = lsbShareHelper(pc, branchPredictor.getHistVecSize());
        }
        if(branchPredictor.getShareType() == midShare){
            sharerHelper = midShareHelper(pc, branchPredictor.getHistVecSize());
        }
        if (branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.getGlobalHistoryVec()] == ST
        || branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.getGlobalHistoryVec()] == WT){
            *dst = branchPredictor.targetPCs[entry];
            return true;
        }
    }
    // global history vector and local tables
    if(branchPredictor.isGlobalHistory() && !(branchPredictor.isGlobalTable())
        && (branchPredictor.localFSMs[entry][branchPredictor.getGlobalHistoryVec()] == ST
        || branchPredictor.localFSMs[entry][branchPredictor.getGlobalHistoryVec()] == WT)){
            *dst = branchPredictor.m_target[pcCorrected];
            return true;
        }
    // local history vector and local tables
    if(!branchPredictor.isGlobalHistory() && !(branchPredictor.isGlobalTable())
       && (branchPredictor.localFSMs[entry][branchPredictor.localHistories[entry]] == ST
           || branchPredictor.localFSMs[entry][branchPredictor.localHistories[entry]] == WT)){
        *dst = branchPredictor.m_target[pcCorrected];
        return true;
    }
    // local history vector and a global table
    if(!branchPredictor.isGlobalHistory() && branchPredictor.isGlobalTable()){
        int sharerHelper = 0;
        if(branchPredictor.getShareType() == lsbShare){
            sharerHelper = lsbShareHelper(pc, branchPredictor.getHistVecSize());
        }
        if(branchPredictor.getShareType() == midShare){
            sharerHelper = midShareHelper(pc, branchPredictor.getHistVecSize());
        }
        if (branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.localHistories[entry]] == ST
        || branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.localHistories[entry]] == WT) {
            *dst = branchPredictor.m_target[pcCorrected];
            return true;
        }
    }
    *dst = pc + 4;
    return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
    BranchPredictor& branchPredictor = BranchPredictor::getInstance();
    uint32_t entry = calcEntry(pc, branchPredictor.getBtbSize());
    uint32_t tag = calcTag(pc, branchPredictor.getBtbSize(), branchPredictor.getTagSize());
    // update the stats struct to have the correct number of branches
    branchPredictor.simStats.br_num++;
    // update the stats struct to have the correct number of false predictions (times flushed)
    if((taken && ((pred_dst == pc + 4) || (pred_dst != targetPc)))
    || (!taken && (pred_dst != pc + 4))){
        branchPredictor.simStats.flush_num++;
    }
    branchPredictor.isABTBEntry[entry] = true;
    branchPredictor.targetPCs[entry] = targetPc;
    // global history vector and a global table
    if(branchPredictor.isGlobalHistory() && branchPredictor.isGlobalTable()){
        int sharerHelper = 0;
        if(branchPredictor.getShareType() == lsbShare){
            sharerHelper = lsbShareHelper(pc, branchPredictor.getHistVecSize());
        }
        if(branchPredictor.getShareType() == midShare){
            sharerHelper = midShareHelper(pc, branchPredictor.getHistVecSize());
        }
        if (tag != branchPredictor.tags[entry]){
            branchPredictor.tags[entry] = tag;
        }
        if (taken){
            branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.getGlobalHistoryVec()] =
                    (branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.getGlobalHistoryVec()] == ST)
                    ? ST :(StateOfFSM)(((int)branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.getGlobalHistoryVec()])+1);}

        else{
            branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.getGlobalHistoryVec()] =
                    (branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.getGlobalHistoryVec()] == SNT)
                    ? SNT :(StateOfFSM)(((int)branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.getGlobalHistoryVec()])-1);}

        // shift left the history vector
        branchPredictor.setGlobalHistoryVec(branchPredictor.getGlobalHistoryVec() << 1) % PowerOfTwo(branchPredictor.getHistVecSize());
        // if taken , insert 1 from the right
        if (taken) branchPredictor.setGlobalHistoryVec(branchPredictor.getGlobalHistoryVec()++);
    }
    // global history vector and local tables
    if(branchPredictor.isGlobalHistory() && !(branchPredictor.isGlobalTable())){
        if (tag != branchPredictor.tags[entry]){
            branchPredictor.tags[entry] = tag;
            branchPredictor.localFSMs[entry] = vector<StateOfFSM>(PowerOfTwo(branchPredictor.getHistVecSize()), branchPredictor.getInitialFsmState());
        }
        if (taken){
            branchPredictor.localFSMs[entry][branchPredictor.getGlobalHistoryVec()] =
                    (branchPredictor.localFSMs[entry][branchPredictor.getGlobalHistoryVec()] == ST)
                    ? ST :(StateOfFSM)(((int)branchPredictor.localFSMs[entry][branchPredictor.getGlobalHistoryVec()])+1);}

        else{
            branchPredictor.localFSMs[entry][branchPredictor.getGlobalHistoryVec()] =
                    (branchPredictor.localFSMs[entry][branchPredictor.getGlobalHistoryVec()] == SNT)
                    ? SNT :(StateOfFSM)(((int)branchPredictor.localFSMs[entry][branchPredictor.getGlobalHistoryVec()])-1);}

        // shift left the history vector
        branchPredictor.setGlobalHistoryVec(branchPredictor.getGlobalHistoryVec() << 1) % PowerOfTwo(branchPredictor.getHistVecSize());
        // if taken , insert 1 from the right
        if (taken) branchPredictor.setGlobalHistoryVec(branchPredictor.getGlobalHistoryVec()++);
    }
    // local history vector and local tables
    if(!branchPredictor.isGlobalHistory() && !(branchPredictor.isGlobalTable())){
        if (tag != branchPredictor.tags[entry]){
            branchPredictor.localFSMs[entry] = vector<StateOfFSM>(PowerOfTwo(branchPredictor.getHistVecSize()), branchPredictor.getInitialFsmState());
            branchPredictor.localHistories[entry] = 0;
            branchPredictor.tags[entry] = tag;
        }
        if (taken){
            branchPredictor.localFSMs[entry][branchPredictor.localHistories[entry]] =
                    (branchPredictor.localFSMs[entry][branchPredictor.localHistories[entry]] == ST)
                    ? ST :(StateOfFSM)(((int)branchPredictor.localFSMs[entry][branchPredictor.localHistories[entry]])+1);}

        else{
            branchPredictor.localFSMs[entry][branchPredictor.localHistories[entry]] =
                    (branchPredictor.localFSMs[entry][branchPredictor.localHistories[entry]] == SNT)
                    ? SNT :(StateOfFSM)(((int)branchPredictor.localFSMs[entry][branchPredictor.localHistories[entry]])-1);}

        // shift left the history vector
        branchPredictor.localHistories[entry] = (branchPredictor.localHistories[entry] << 1) % PowerOfTwo(branchPredictor.getHistVecSize();
        // if taken , insert 1 from the right
        if (taken) branchPredictor.localHistories[entry]++;

    }
    // local history vector and a global table
    if(!branchPredictor.isGlobalHistory() && branchPredictor.isGlobalTable()){
        int sharerHelper = 0;
        if(branchPredictor.getShareType() == lsbShare){
            sharerHelper = lsbShareHelper(pc, branchPredictor.getHistVecSize());
        }
        if(branchPredictor.getShareType() == midShare){
            sharerHelper = midShareHelper(pc, branchPredictor.getHistVecSize());
        }

        if (tag != branchPredictor.tags[entry]){
            branchPredictor.localHistories[entry] = 0;
            branchPredictor.tags[entry] = tag;
        }
        if (taken){
            branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.localHistories[entry]] =
                    (branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.localHistories[entry]] == ST)
                    ? ST :(StateOfFSM)(((int)branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.localHistories[entry]])+1);}

        else{
            branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.localHistories[entry]] =
                    (branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.localHistories[entry]] == SNT)
                    ? SNT :(StateOfFSM)(((int)branchPredictor.GlobalFSM[sharerHelper ^ branchPredictor.localHistories[entry]])-1);}
        branchPredictor.localHistories[entry] = (branchPredictor.localHistories[entry] << 1) % PowerOfTwo(branchPredictor.getHistVecSize();
        // if taken , insert 1 from the right
        if (taken) branchPredictor.localHistories[entry]++;
    }
    return;
}

void BP_GetStats(SIM_stats *curStats){
    BranchPredictor& branchPredictor = BranchPredictor::getInstance();
    curStats->flush_num=branchPredictor.simStats.flush_num;
    curStats->br_num=branchPredictor.simStats.br_num;
    int history_size = 0, numberOfFSMs = 0;
    if (branchPredictor.isGlobalHistory()) {
        history_size = branchPredictor.getHistVecSize();
    } else {
        history_size = branchPredictor.getHistVecSize() * (branchPredictor.getBtbSize());
    }
    if (branchPredictor.isGlobalTable()) {
        numberOfFSMs = PowerOfTwo( branchPredictor.getHistVecSize());
    } else {
        numberOfFSMs = PowerOfTwo( branchPredictor.getHistVecSize()) * branchPredictor.getBtbSize();
    }
    curStats->size = branchPredictor.getBtbSize() * (branchPredictor.getTagSize() + 30 + 1) + history_size + numberOfFSMs * 2;
    return;
}

