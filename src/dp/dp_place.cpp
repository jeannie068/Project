#include "dp_data.h"
#include "dp_global.h"
#include "Timer/Timer.hpp"
#include <time.h>
#define STEP1ONLY 1
#define REFINEMENT 1

using namespace dp;
using namespace moodycamel;
clock_t startt, LG, CM, NF;
Timer timer(10*60); // set time limit to 10 minutes
void DPlacer::setupFlow(const std::string& name) {
    //PM LG if only 1st step.
    /*****modded*****/
    if (name == "iccad2017") {
        if (dp::DPModule::onlyMGL==1)
        {
            // flow for displacement-driven DP
            flow.addStage("PM", DPStage::Technique::Premove, 1);
            flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        }
        else
        {
            // flow for displacement-driven DP
            flow.addStage("PM", DPStage::Technique::Premove, 1);
            flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
            flow.addStage("CM", DPStage::Technique::ChainMove, 30);
            flow.addStage("LM", DPStage::Technique::LocalMoveForPinAccess, 1);
            //  flow.addStage("MP", DPStage::Technique::GlobalMoveForPinAccess, DPModule::MaxMPIter);
            flow.addStage("NF", DPStage::Technique::GlobalNF, 1);
            if (DPModule::GMEnable) {
                cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!hey!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
                flow.addStage("GM", DPStage::Technique::GlobalMove, DPModule::MaxGMIter);
            }
        }
    /*****modded*****/
    } else if (name == "dac2016") {
        flow.addStage("PM", DPStage::Technique::Premove, 1);
        flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        flow.addStage("NF", DPStage::Technique::GlobalNF, 1);
    } else if (name == "eval") {
        flow.addStage("EV", DPStage::Technique::Eval, 1);
    } else if (name == "WLDriven") {
        // flow for wirelength-driven DP
        flow.addStage("PM", DPStage::Technique::Premove, 1);
        flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        flow.addStage("GM", DPStage::Technique::GlobalMove, DPModule::MaxGMIter);
        flow.addStage("LM", DPStage::Technique::LocalMove, DPModule::MaxLMIter);
    } else if (name == "DispDriven") {
        // flow for displacement-driven DP
        flow.addStage("PM", DPStage::Technique::Premove, 1);
        flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        // flow.addStage("NF", DPStage::Technique::GlobalNF, 1);
        // flow.addStage("GM", DPStage::Technique::GlobalMove, DPModule::MaxGMIter);
    } else {
        // flow for DP API
        flow.addStage("PM", DPStage::Technique::Premove, 1);
        flow.addStage("LG", DPStage::Technique::Legalize, DPModule::MaxLGIter);
        flow.addStage("GM", DPStage::Technique::GlobalMove, DPModule::MaxGMIter);
        flow.addStage("LM", DPStage::Technique::LocalMove, DPModule::MaxLMIter);
    }
}

void DPlacer::place(const string& name) {
    // cout << "setupFlow: " << name <<endl;
    printlog(LOG_INFO, "setupFlow: %s", name.c_str());
    setupFlow(name);
    startt=clock();
    for (bool running = true; running;) {
        DPStage::Technique tech = flow.technique();
        int nMoved = 0;
        switch (tech) {
            case DPStage::Technique::None:
                // first stage
                break;
            case DPStage::Technique::Eval:
                nMoved = eval();
                break;
            case DPStage::Technique::Premove:
                nMoved = premove();
                break;
            case DPStage::Technique::Legalize:
                nMoved = legalize();
                break;
            case DPStage::Technique::ChainMove:
                //  nMoved = chainMove();
                nMoved = localChainMove();
                break;
            case DPStage::Technique::GlobalMove:
                nMoved = globalMove();
                break;
            case DPStage::Technique::GlobalMoveForPinAccess:
                nMoved = globalMoveForPinAccess();
                break;
            case DPStage::Technique::LocalMove:
                nMoved = localMove();
                break;
            case DPStage::Technique::LocalMoveForPinAccess:
                nMoved = localMoveForPinAccess();
                break;
            case DPStage::Technique::GlobalNF:
                nMoved = minCostFlow();
                break;
            default:
                break;
        }

        flow.update();
        if (nMoved || !flow.iter() || tech == DPStage::Technique::GlobalMoveForPinAccess) {
            /*****modded*****/
            if (dp::DPModule::refinementMode!=1) flow.report();
            /*****modded*****/
        }

        // stopping condition for each stage
        switch (tech) {
            case DPStage::Technique::Eval:
                running = flow.nextStage();
                continue;
            case DPStage::Technique::Legalize:
                if (!flow.unplaced()) {
                    running = flow.nextStage();
                    LG=clock();
                    cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!LG: " << -float(startt-LG)/CLOCKS_PER_SEC<<endl;
                    continue;
                }

                break;
            case DPStage::Technique::ChainMove:
                if (!nMoved) {
                    running = flow.nextStage();
                    CM=clock();
                    cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!CM: "<<-float(LG-CM)/CLOCKS_PER_SEC<<endl;
                    continue;
                }

                break;
            case DPStage::Technique::GlobalMove:
                if (!flow.unplaced() && flow.hpwlDiff() > -0.0005) {
                    running = flow.nextStage();
                    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!error1"<<endl;
                    continue;
                }
               
                break;
            case DPStage::Technique::GlobalMoveForPinAccess:
                if (!flow.unplaced() && !flow.overlapSum()) {
                    running = flow.nextStage();
                    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!error2"<<endl;
                    continue;
                }
                break;
            case DPStage::Technique::LocalMove:
                if (!flow.unplaced() && flow.hpwlDiff() > 0.0) {
                    running = flow.nextStage();
                    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!error3"<<endl;
                    continue;
                }
                
                break;
            case DPStage::Technique::GlobalNF:
                running = flow.nextStage();
                NF=clock();
                cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!NF: "<<-float(CM-NF)/CLOCKS_PER_SEC<<endl;
                continue;

            default:
                break;
        }
        running = flow.next();
    }
}

bool DPlacer::isBookshelfSymbol(unsigned char c) {
    static char symbols[256] = {0};
    static bool inited = false;
    if (!inited) {
        symbols[(int)'('] = 1;
        symbols[(int)')'] = 1;
        // symbols[(int)'['] = 1;
        // symbols[(int)']'] = 1;
        symbols[(int)','] = 1;
        // symbols[(int)'.'] = 1;
        symbols[(int)':'] = 1;
        symbols[(int)';'] = 1;
        // symbols[(int)'/'] = 1;
        symbols[(int)'#'] = 1;
        symbols[(int)'{'] = 1;
        symbols[(int)'}'] = 1;
        symbols[(int)'*'] = 1;
        symbols[(int)'\"'] = 1;
        symbols[(int)'\\'] = 1;

        symbols[(int)' '] = 2;
        symbols[(int)'\t'] = 2;
        symbols[(int)'\n'] = 2;
        symbols[(int)'\r'] = 2;
        inited = true;
    }
    return symbols[(int)c] != 0;
}

bool DPlacer::readBSLine(istream& is, vector<string>& tokens) {
    tokens.clear();
    string line;
    while (is && tokens.empty()) {
        // read next line in
        getline(is, line);

        char token[1024] = {0};
        int tokenLen = 0;
        for (const char c : line) {
            if (c == '#') {
                break;
            }

            if (isBookshelfSymbol(c)) {
                if (tokenLen > 0) {
                    token[tokenLen] = (char)0;
                    tokens.push_back(string(token));
                    token[0] = (char)0;
                    tokenLen = 0;
                }
            } else {
                token[tokenLen++] = c;
                if (tokenLen > 1024) {
                    // TODO: unhandled error
                    tokens.clear();
                    return false;
                }
            }
        }
        // line finished, something else in token
        if (tokenLen > 0) {
            token[tokenLen] = (char)0;
            tokens.push_back(string(token));
            tokenLen = 0;
        }
    }
    return !tokens.empty();
}

bool DPlacer::readDefCell(string file, map<string, tuple<int, int, int>>& cell_list) {
    ifstream fs(file.c_str());
    if (!fs.good()) {
        printlog(LOG_ERROR, "cannot open file: %s", file.c_str());
        return false;
    }

    vector<string> tokens;
    while (DPlacer::readBSLine(fs, tokens)) {
        if (tokens[0] == "COMPONENTS") {
            break;
        }
    }
    cell_list.clear();
    while (DPlacer::readBSLine(fs, tokens)) {
        if (tokens[0] == "END") {
            break;
        } else if (tokens[0] == "SOURCE") {
            DPlacer::readBSLine(fs, tokens);
        }
        string name = tokens[1];
        readBSLine(fs, tokens);
        istringstream issx(tokens[2]);
        istringstream issy(tokens[3]);
        double x = 0;
        double y = 0;
        issx >> x;
        issy >> y;
        int orient = -1;
        if (tokens[4] == "N") {
            orient = 0;
        } else if (tokens[4] == "S") {
            orient = 2;
        } else if (tokens[4] == "FN") {
            orient = 4;
        } else if (tokens[4] == "FS") {
            orient = 6;
        }
        cell_list[name] = make_tuple((int)std::round(x), (int)std::round(y), orient);
    }

    return true;
}

int DPlacer::eval() {
    int nMoved = 0;
    map<string, tuple<int, int, int>> cell_list;
    readDefCell(DPModule::DefEval, cell_list);
    for (Cell* cell : cells) {
        tuple<int, int, int>& t = cell_list[cell->getDBCell()->name()];
        Move move(cell, get<0>(t) / siteW, get<1>(t) / siteH, get<2>(t));
        doLegalMove(move, false);
        ++nMoved;
    }
    return nMoved;
}

int DPlacer::premove() {
    timer.startTimer("PM");
    int nMoved = 0;
    int preMoveDispX = 0;
    int preMoveDispY = 0;
    for (Cell* cell : cells) {
        // modify for TYPE4 cells (quadruple height)
        const Segment* segment;
        if (cell->getDBCell()->siteHeight() == 4)
        {
            // set PPNN cell horizontal spacing with 1 site separation
            // cell->getDBCell()->setSpacingL(siteW);
            // cell->getDBCell()->setSpacingR(siteW);
            
            // set PPNN width = width + 2 site width
            // cell->getDBCell()->setCellWidth(cell->getDBCell()->width() + siteW * 2);
            
            segment = getNearestSegment(cell, cell->cx(), cell->ly() + siteH);
        }
        else
        {
            segment = getNearestSegment(cell, cell->cx(), cell->ly());
        }
        // const Segment* segment = getNearestSegment(cell, cell->cx(), cell->ly());
        
        if (!segment) {
            printlog(
                LOG_ERROR, "fail to find segment for %s with %u width", cell->getDBCellType()->name.c_str(), cell->w);
        }
        int dispX = 0;
        int dispY = abs(cell->ly() - segment->y);
        if (dispY) {
            cell->pmDy(dispY);
        } else {
            db::Region* reg = database.regions[cell->fid];
            cell->pmDy(-min(cell->ly() - ceil((reg->ly - database.coreLY) / (double)siteH),
                            floor((reg->hy - database.coreLY) / (double)siteH) - cell->hy()));
        }
        cell->ly(segment->y);
        if (cell->lx() < segment->boundL()) {
            dispX = segment->boundL() - cell->lx();
            cell->lx(segment->boundL());
        } else if (cell->hx() > segment->boundR()) {
            dispX = cell->hx() - segment->boundR();
            cell->hx(segment->boundR());
        }
        if (dispX > 0 || dispY > 0) {
            preMoveDispX += dispX;
            preMoveDispY += dispY;
            nMoved++;
        }
    }
    timer.stopTimer("PM");
    timer.printTime("PM");

    // cout << "pre move disp x = " << preMoveDispX << ", pre move disp y = " << preMoveDispY << endl;

#ifndef NDEBUG
    if (preMoveDispX > 0 || preMoveDispY > 0) {
        printlog(LOG_INFO, "pre move disp x = %d", preMoveDispX);
        printlog(LOG_INFO, "pre move disp y = %d", preMoveDispY);
    }
#endif

    return nMoved;
}

int DPlacer::legalize() {
    timer.startTimer("LG");
    const int threshold = -1;

    localRegions.resize(DPModule::CPU, LocalRegion(cells.size()));
    tQ = new ConcurrentQueue<pair<Cell*, Region>>(DPModule::CPU, 1, 0);
    rQ = new ConcurrentQueue<pair<Cell*, bool>>(DPModule::CPU, DPModule::CPU - 1, 0);
    tPtok = new ProducerToken(*tQ);
    for (unsigned i = 0; i != DPModule::CPU; ++i) {
        rPtoks.emplace_back(*rQ);
    }

    // add every type4 cell's width for 2 site width, for horizontal spacing rules
    for (auto cell: cells)
    {
        if (cell->getDBCell()->siteHeight() == 4)
        {
            cell->w += 2;
        }
    }

    // Method1: sort cells by their lower X coordinate   // for test // worse result compare with original cell order (random? by input .def file cell order?)
    // sort(cellsIter, cells.end(), [](Cell* a, Cell* b) { return a->lx() < b->lx(); });


    // Method2: If the input GP test case has TYPE4 cell grouping information, from BOYI's GP engine    // the result can be better than the original cell order a little bit in most of the testcases ~0.1%, with some testcase a little bit worse ~0.1%
    // Legalize those cells in groups first from small to large, then legalize the rest of the cells
    if (database.type4CellGroups.size() != 0)
    {
        vector<Cell*> newCells;
        newCells.reserve(cells.size());
        // first push cells without group into newCells
        std::sort(database.type4CellGroups.begin(), database.type4CellGroups.end(), [](db::Group* a, db::Group* b) { return a->members.size() < b->members.size(); });
        
        // First push cells with group into newCells, since they need to place together
        for (auto group: database.type4CellGroups)
        {
            for (auto cell: group->members)
            {
                newCells.push_back(dbCellMap[cell]);
            }
        }
        // Then push those cells without groupId 
        for (auto cell: cells)
        {
            if (cell->getDBCell()->groupId == -1)
            {
                newCells.push_back(cell);
            }
        }
        assert(newCells.size() == cells.size());

        cells.clear();
        cells = newCells;
        cells.shrink_to_fit();
        newCells.clear();
    }


    // Method3: legalize TYPE4 cell first, then legalize the rest of the TYPE1 cells    // the result is almost the same as original cell order
    // vector<Cell*> newCells;
    // newCells.reserve(cells.size());
    // for (auto cell: cells)
    // {
    //     if (cell->getDBCell()->siteHeight() == 4)
    //     {
    //         newCells.push_back(cell);
    //     }
    // }
    // for (auto cell: cells)
    // {
    //     if (cell->getDBCell()->siteHeight() == 2)
    //     {
    //         newCells.push_back(cell);
    //     }
    // }
    // assert(newCells.size() == cells.size());
    // cells.clear();
    // cells = newCells;
    // cells.shrink_to_fit();
    // newCells.clear();


    // start legalizing cells
    vector<future<unsigned>> futs;
    for (unsigned i = 1; i < DPModule::CPU; ++i) {
        futs.push_back(async(&DPlacer::legalizeCell, this, threshold, i));
    }
    int nMoved = legalizeCell(threshold, 0);
    for (future<unsigned>& fut : futs) {
        nMoved += fut.get();
    }

    // finish legalizing all the cells
    // shift all the type4 cell right 1 site
    for (auto cell: cells)
    {
        if (cell->getDBCell()->siteHeight() == 4)
        {
            // shift type4 cell right 1 site
            cell->lx(cell->lx() + 1);
        }
    }

    // flip all the cell to correct orientation
    for (auto cell: cells)
    {
        int y = (cell->getDBCell()->siteHeight() == 4) ? cell->ly() + 1 : cell->ly();
        int targetNumRow = y / 2;
        // cout << cell->getDBCell()->name() << " " << cell->ly() << " " << y << " " << siteH << " " << targetNumRow << endl;
        if (targetNumRow % 2 == 0)
        {
            // flip type4 cell to correct orientation
            cell->fy(1);
        }
    }

    delete tQ;  // give a cell with an available operating region   // pair<Cell*, Region>
    delete rQ;  // after legaling a cell, push the cell into rQ. legalized: false, not legalized: true     // pair<Cell*, bool>
    delete tPtok;
    tQ = nullptr;
    rQ = nullptr;
    tPtok = nullptr;

    timer.stopTimer("LG");
    timer.printTime("LG");

    return nMoved;
}

// cell -> originalRegion(waiting queue) -> operatingregions+tq(in execution)
// 
unsigned DPlacer::legalizeCell(const int threshold, const unsigned iThread) {
    unsigned nMoved = 0;
    pair<Cell*, Region> pcr;
    pair<Cell*, bool> pcb;
    static vector<Cell*>::iterator cellsIter = cells.begin();
    static list<pair<Cell*, Region>> originalRegions;
    static unordered_map<Cell*, Region> operatingRegions;
    //  static unsigned batchIdx = 0;

    // if (!iThread)
    // {
    //     int i=0;
    //     for (auto x: cells)
    //     {
    //         if (++i>100000) break;
    //         if (i%500==0)
    //         {
    //             cout << x->_dbCell->name() <<": " <<x->w<<" "<<x->_dbCell->width() <<" / " <<x->h <<" "<< x->_dbCell->height()<<endl;
    //         }
    //     }
    // }

    while (true) {
        if (!iThread) {
            oHist = hist;
            oMaxDisp = maxDisp;
            list<pair<Cell*, Region>>::iterator oriRegIter = originalRegions.begin();
            // emplace those (cells, region) into operatingRegions first , since they cannot be legalized in the previous iteration
            while (oriRegIter != originalRegions.end()) {
                if (oriRegIter->second.independent(operatingRegions)) {
                    Cell* cell = oriRegIter->first;
                    operatingRegions[cell] = oriRegIter->second;
                    tQ->enqueue(*tPtok, *oriRegIter);
                    // remove from originalRegions, since we have place them into operatingRegions
                    oriRegIter = originalRegions.erase(oriRegIter);
                    if (operatingRegions.size() == DPModule::LGOperRegSize) { //確保operatingRegions.size()<=256
                        break;
                    }
                } else {
                    ++oriRegIter;
                }
            }


            // assign a region for a cell
            for (; operatingRegions.size() != DPModule::LGOperRegSize && cellsIter != cells.end(); ++cellsIter) {
                Cell* cell = *cellsIter;
                // assign a region: (originX - MLLLocalRegionW), (originX + cell width + MLLLocalRegionW), (originY - MLLLocalRegionH), (originY + cell height + MLLLocalRegionH)
                Region oRegion = cell->getOriginalRegion(MLLLocalRegionW, MLLLocalRegionH);

                // cout << cell->lx() << " " << cell->hx() << " " << cell->ly() << " " << cell->hy() << endl;
                // cout << oRegion.lx << " " << oRegion.hx << " " << oRegion.ly << " " << oRegion.hy << endl;
                // cout << "---------------" << endl;

                assert(oRegion.area());
            #ifdef LEGALIZE_REGION_SHIFT_TOWARDS_OPTIMAL_REGION // not enabled
                Region optimalRegion = cell->getOptimalRegion();
                optimalRegion.hx += cell->w;
                optimalRegion.hy += cell->h;

                int targetX = optimalRegion.cx();
                int targetY = optimalRegion.cy();
                targetX = max(oRegion.lx, min(targetX, oRegion.hx));
                targetY = max(oRegion.ly, min(targetY, oRegion.hy));
                oRegion.shift(targetX - oRegion.cx(), targetY - oRegion.cy());
            #endif
                if (oRegion.independent(operatingRegions)) { // if oRegion沒被包在operatingRegions裡面任一區域內，即為True, 有可能cell一開始被放在MACRO內, 代表cell 一開始不被包在operatingRegions裡面
                    operatingRegions.emplace(cell, oRegion);
                    tQ->enqueue(*tPtok, make_pair(cell, oRegion));
                    if (operatingRegions.size() == DPModule::LGOperRegSize) {
                        ++cellsIter;
                        break;
                    }
                } else {
                    originalRegions.emplace_back(cell, oRegion);
                }
            }

            // finish legalizing all cells
            if (operatingRegions.empty() && originalRegions.empty()) {
                for (unsigned termThrIdx = 1; termThrIdx < DPModule::CPU; ++termThrIdx) {
                    tQ->enqueue(*tPtok, make_pair(nullptr, Region()));
                }
                return nMoved;
            }
        }

        // dequeue tQ will get (cell, oRegion), oRegion will be a region that cell will be placed(legal at the begining but may overlap with other cell)
        while (tQ->try_dequeue_from_producer(*tPtok, pcr)) {
            Cell* cell = pcr.first;
            if (!cell) {
                return nMoved;
            }
            /*
            if (cell->getDBCell()->name() == "g213448_u1" || cell->getDBCell()->name() == "FE_OFC4844_n_11951") {
                printlog(LOG_INFO, "cell %s start %u", cell->getDBCell()->name().c_str(), batchIdx);
            }
            */
           // set cell's target position as input original position
            cell->tx(cell->ox());
            cell->ty(cell->oy());
            
            // set TYPE4 cell target y on the nearest legal row
            if (cell->getDBCell()->siteHeight() == 4)
            {
                // cout << cell->getDBCell()->name() << endl;
                double realY = cell->oy() * 2;
                cell->ty((round((realY - 2.0) / 8.0) * 8.0 + 2.0) / 2);
                // cout << realY << " " << cell->ty() * 2 << endl;
                // cout << "====================" << endl;
            }

            Move move(cell, cell->tx(), cell->ty());

            const Region& oRegion = pcr.second;
            assert(oRegion.lx != INT_MAX && oRegion.ly != INT_MAX && oRegion.hx != INT_MIN && oRegion.hy != INT_MIN);
            if (isLegalMove(move, oRegion, threshold, 1, iThread)) {
                doLegalMove(move, false);
                ++nMoved;
                rQ->enqueue(make_pair(cell, false));
                /*
                if (cell->getDBCell()->name() == "g213448_u1" || cell->getDBCell()->name() == "FE_OFC4844_n_11951") {
                    printlog(LOG_INFO, "cell %s succeeded", cell->getDBCell()->name().c_str());
                }
                */
                continue;
            }
            rQ->enqueue(make_pair(cell, true));
            /*
            if (cell->getDBCell()->name() == "g213448_u1" || cell->getDBCell()->name() == "FE_OFC4844_n_11951") {
                printlog(LOG_INFO, "cell %s failed", cell->getDBCell()->name().c_str());
            }
            */
        }

        if (iThread) {
            continue;
        }
        //  ++batchIdx;
        while (operatingRegions.size()) {
            if (!rQ->try_dequeue(pcb)) {
                continue;
            }
            Cell* cell = pcb.first;
            // the oRegion cannot legalize the cell, need to extend the operating region for this cell
            if (pcb.second) {
                Region& region = operatingRegions[cell];
                // extend the oRegion again
                region.expand(
                    MLLLocalRegionW, MLLLocalRegionW, MLLLocalRegionH, MLLLocalRegionH, database.regions[cell->fid]);
                // no enough region to legalize the cell
                if (region.isMax()) {
                    printlog(LOG_WARN,
                             "local region %s covers whole fence %s",
                             cell->getDBCell()->name().c_str(),
                             database.regions[cell->fid]->name().c_str());
                }
                bool is_not_emplaced = true;
                for (list<pair<Cell*, Region>>::iterator iter = originalRegions.begin(); iter != originalRegions.end();
                     ++iter) {
                    if (iter->first->i > cell->i) {
                        originalRegions.emplace(iter, cell, region);
                        is_not_emplaced = false;
                        break;
                    }
                }
                if (is_not_emplaced) {
                    originalRegions.emplace_back(cell, region);
                }
            }
            // finish legalizing this cell, remove from operatingRegions
            operatingRegions.erase(cell);
        }
    }
}

unsigned DPlacer::chainMove() {
    unsigned nMoves = 0;
    for (unsigned i = 0; i != nRegions; ++i) {
        for (unsigned j = 0; j != nDPTypes; ++j) {
            nMoves += chainMoveCell(i, j);
        }
    }
    return nMoves;
}

unsigned DPlacer::localChainMove() {
    Cell* targetCell = cells[0];
    for (Cell* cell : cells) {
        if (abs(cell->lx() - cell->ox()) * siteW + abs(cell->ly() - cell->oy()) * siteH >
            abs(targetCell->lx() - targetCell->ox()) * siteW + abs(targetCell->ly() - targetCell->oy()) * siteH) {
            targetCell = cell;
        }
    }
    return localChainMoveCell(targetCell);
}

unsigned DPlacer::globalMove() {
    unsigned nMoves = 0;
    const unsigned threshold = DPModule::GMThresholdBase + flow.iter() * DPModule::GMThresholdStep;
    for (Cell* cell : cells) {
        nMoves += globalMoveCell(cell, threshold, 20, 3);
    }
    return nMoves;
}

unsigned DPlacer::globalMoveCell(Cell* cell, int threshold, int Rx, int Ry) {
    oHist = hist;
    for (int trial = 0; trial < 2; trial++) {
        Region trialRegion;
        Region oRegion = cell->getOptimalRegion();
        if (trial == 0) {
#ifdef OPTIMAL_REGION_COVER_FIX
            int dx = max(0, max(oRegion.lx - cell->lx(), cell->lx() - oRegion.hx));
            int dy = max(0, max(oRegion.ly - cell->ly(), cell->ly() - oRegion.hy));
            int distance = dx * siteW + dy * siteH;
            if (cell->placed && distance <= DISTANCE_THRESHOLD) {
                return 0;
            }
#else
            if (cell->placed && oRegion.contains(cell->lx(), cell->ly(), cell->w, cell->h)) {
                // if(cell->placed && oRegion.contains(cell->lx(), cell->ly())){
                return 0;
            }
#endif
#ifdef OPTIMAL_REGION_SIZE_LIMIT
            if (oRegion.width() > OPTIMAL_REGION_SIZE_X_LIMIT) {
                if (cell->lx() < oRegion.lx + OPTIMAL_REGION_SIZE_X_LIMIT / 2) {
                    oRegion.hx = oRegion.lx + OPTIMAL_REGION_SIZE_X_LIMIT;
                } else if (cell->lx() > oRegion.hx - OPTIMAL_REGION_SIZE_X_LIMIT / 2) {
                    oRegion.lx = oRegion.hx - OPTIMAL_REGION_SIZE_X_LIMIT;
                } else {
                    oRegion.lx = cell->lx() - OPTIMAL_REGION_SIZE_X_LIMIT / 2;
                    oRegion.hx = oRegion.lx + OPTIMAL_REGION_SIZE_X_LIMIT;
                }
            }
            if (oRegion.height() > OPTIMAL_REGION_SIZE_Y_LIMIT) {
                if (cell->ly() < oRegion.ly + OPTIMAL_REGION_SIZE_Y_LIMIT / 2) {
                    oRegion.hy = oRegion.ly + OPTIMAL_REGION_SIZE_Y_LIMIT;
                } else if (cell->ly() > oRegion.hy - OPTIMAL_REGION_SIZE_Y_LIMIT / 2) {
                    oRegion.ly = oRegion.hy - OPTIMAL_REGION_SIZE_Y_LIMIT;
                } else {
                    oRegion.ly = cell->ly() - OPTIMAL_REGION_SIZE_Y_LIMIT / 2;
                    oRegion.hy = oRegion.ly + OPTIMAL_REGION_SIZE_Y_LIMIT;
                }
            }
#endif

            trialRegion = oRegion;
        } else {
            trialRegion = cell->getImproveRegion(oRegion);
        }
        cell->tx(getrand(trialRegion.lx, trialRegion.hx));
        cell->ty(getrand(trialRegion.ly, trialRegion.hy));

        Move move(cell, cell->tx(), cell->ty());

        Region localRegion;
        localRegion.lx = max<long>(coreLX, cell->txLong() - Rx);
        localRegion.ly = max<long>(coreLY, cell->tyLong() - Ry);
        localRegion.hx = min<long>(coreHX - cell->w, cell->txLong() + Rx + cell->w);
        localRegion.hy = min<long>(coreHY - cell->h, cell->tyLong() + Ry + cell->h);
        if (isLegalMove(move, localRegion, threshold)) {
            int hpwlDiff = getHPWLDiff(move);
            if (!cell->placed || hpwlDiff < 0) {
                doLegalMove(move, true);
                return 1;
            }
        }
    }
    return 0;
}

int DPlacer::globalMoveForPinAccess() {
    int nMoves = 0;

    int stageIter = flow.iter() + 1;
    int base = DPModule::GMThresholdBase;
    int step = DPModule::GMThresholdStep;
    int threshold = base + (stageIter - 1) * step;

    int rangeX = 20 * stageIter;  // 30;
    int rangeY = 3 * stageIter;   // 5;
    int nCells = cells.size();
    // printlog(LOG_INFO, "threshold = %d, rangeX = %d, rangeY = %d", threshold*siteW, rangeX, rangeY);
    for (int i = 0; i < nCells; i++) {
        if (cellTypeMaps[cells[i]->type].score(cells[i]->ly(), cells[i]->lx()) != 0.0)
            nMoves += globalMoveForPinAccessCell(cells[i], threshold, rangeX, rangeY);
    }
    return nMoves;
}

int DPlacer::globalMoveForPinAccessCell(Cell* cell, int threshold, int Rx, int Ry) {
    cell->tx(cell->ox());
    cell->ty(cell->oy());

    Move move(cell, cell->tx(), cell->ty());

    Region localRegion;
    localRegion.lx = max<long>(coreLX, cell->txLong() - Rx);
    localRegion.ly = max<long>(coreLY, cell->tyLong() - Ry);
    localRegion.hx = min<long>(coreHX, cell->txLong() + Rx + cell->w);
    localRegion.hy = min<long>(coreHY, cell->tyLong() + Ry + cell->h);
    if (isLegalMove(move, localRegion, threshold, 0)) {
        if (!cell->placed || getScoreDiff(move) < 0.0) {
            doLegalMove(move, true);
            return 1;
        }
    }

    return 0;
}

int DPlacer::localMove() {
    int nMoves = 0;
    int stageIter = flow.iter() + 1;
    int base = DPModule::LMThresholdBase;
    int step = DPModule::LMThresholdStep;
    int threshold = base + stageIter * step;
    int rangeX = 10;
    int rangeY = 2;
    for (Cell* cell : cells) {
        nMoves += localVerticalMove(cell, threshold, rangeX, rangeY);
    }
    return nMoves;
}

int DPlacer::localVerticalMove(Cell* cell, int threshold, int Rx, int Ry) {
    Region region = cell->getOriginalRegion(Rx, Ry);
    int offset[6] = {1, -1, 2, -2, 4, -4};
    for (int i = 0; i < 6; i++) {
        Move move(cell, cell->lx(), cell->ly() + offset[i]);
        // TODO: check if move improve wirelength
        if (isLegalMove(move, region, threshold) && getHPWLDiff(move) < 0) {
            doLegalMove(move, true);
            // updateNet(move);
            return 1;
        }
    }
    return 0;
}

int DPlacer::localMoveForPinAccess() {
    int nMoves = 0;
    for (Cell* cell : cells) {
        nMoves += localShiftForPinAccess(cell, 16);
    }
    for (auto& row : segments) {
        for (auto& seg : row) {
            nMoves += localSwapForPinAccess(seg, 8);
        }
    }
    return nMoves;
}

MLLCost DPlacer::getPinAccessAndDispCost(Cell* cell, const int x, const int y) const {
    return cellTypeMaps[cell->type].score(y, x) * 1000 +                                // primary: pin access
           (abs(x - cell->ox()) + abs(y - cell->oy())) / (MLLCost)counts[cell->h - 1];  // secondary: disp
}

int DPlacer::localShiftForPinAccess(Cell* cell, int maxDisp) {
    // get all candidate moves
    vector<CellMove> moves;
    int boundL = max(coreLX, cell->lx() - maxDisp);
    int boundR = min(coreHX - (int)cell->w, cell->lx() + maxDisp);
    for (int newX = boundL; newX <= boundR; ++newX) {
        if (newX != cell->lx()) {
            moves.emplace_back(cell, newX, cell->ly());
        }
    }

    // find the best one
    MLLCost bestCost = getPinAccessAndDispCost(cell);
    CellMove* bestMove = nullptr;
    removeCell(cell);
    for (auto& move : moves) {
        if (!isLegalCellMove(move)) continue;
        MLLCost newCost = getPinAccessAndDispCost(move);
        if (newCost < bestCost) {
            bestCost = newCost;
            bestMove = &move;
        }
    }

    // commit
    if (bestMove != nullptr) {
        Move move(*bestMove);  // work around for CellMove -> Move
        doLegalMove(move, true);
        return 1;
    } else {
        insertCell(cell);
        return 0;
    }
}

int DPlacer::localSwapForPinAccess(Segment& seg, int maxDisp) {
    int nMoves = 0;

    //  note that seg may be changed within for loop
    for (int i = 0; i < (int)seg.cells.size() - 1; ++i) {
        // fast pruning
        Cell* cellL = seg.cells.get(i);
        Cell* cellR = seg.cells.get(i + 1);
        if (cellR->lx() - cellL->lx() > maxDisp) {
            continue;
        }
        int oldSpace = cellR->lx() - cellL->hx();  // keep the old spacing for simplicity
        if (Cell::getSpace(cellR, cellL) > oldSpace) {
            continue;
        }

        // get all candidate moves
        vector<pair<CellMove, CellMove>> moves;
        int newRX = cellL->lx();                        // new x for cellR
        int newLX = cellL->lx() + cellR->w + oldSpace;  // new x for cellL
        moves.emplace_back(CellMove(cellR, newRX, cellR->ly()),
                           CellMove(cellL, newLX, cellL->ly()));  // keep themselves's
        if (cellR->ly() != cellL->ly()) {
            moves.emplace_back(CellMove(cellR, newRX, cellL->ly()),
                               CellMove(cellL, newLX, cellR->ly()));  // use the opposite's
            moves.emplace_back(CellMove(cellR, newRX, cellL->ly()),
                               CellMove(cellL, newLX, cellL->ly()));  // both use cellL->ly()
            moves.emplace_back(CellMove(cellR, newRX, cellR->ly()),
                               CellMove(cellL, newLX, cellR->ly()));  // both use cellR->ly()
        }

        // find the best one
        removeCell(cellL);
        removeCell(cellR);
        MLLCost bestCost = getPinAccessAndDispCost(cellL) + getPinAccessAndDispCost(cellR);
        const pair<CellMove, CellMove>* bestMove = nullptr;
        for (const auto& move : moves) {
            const auto& [moveR, moveL] = move;
            if (!isLegalCellMove(moveR) || !isLegalCellMove(moveL)) {
                continue;
            }
            MLLCost newCost = getPinAccessAndDispCost(moveR) + getPinAccessAndDispCost(moveL);
            if (newCost < bestCost) {
                bestCost = newCost;
                bestMove = &move;
            }
        }

        // commit
        if (bestMove) {
            Move moveR(bestMove->first), moveL(bestMove->second);  // work around for CellMove -> Move
            doLegalMove(moveR, true);
            doLegalMove(moveL, true);
            ++nMoves;
        } else {
            insertCell(cellL);
            insertCell(cellR);
        }
    }
    return nMoves;
}
