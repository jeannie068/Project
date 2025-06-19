#include "dp.h"
#include "dp_data.h"
#include "dp_global.h"
using namespace dp;

void DPlacer::doLegalMove(const Move& move, bool updateNet) {
    //  const Move& move = moves[targetCell];
    Cell* targetCell = move.target.cell;
    vector<Net*> updateList;
    if (updateNet) {
        if (targetCell) {
            for (const Pin* cellpin : targetCell->pins) {
                Region& netbound = cellpin->net->box;
                int px_before = cellpin->pinX();
                int py_before = cellpin->pinY();
                int px_after = move.target.x() * DPModule::dplacer->siteW + cellpin->x;
                int py_after = move.target.y() * DPModule::dplacer->siteH + cellpin->y;
                if (cellpin->net->pins.size() <= 2 || netbound.lx == px_before || netbound.hx == px_before ||
                    netbound.ly == py_before || netbound.hy == py_before) {
                    updateList.push_back(cellpin->net);
                } else if (netbound.lx > px_after || netbound.hx < px_after || netbound.ly > py_after ||
                           netbound.hy < py_after) {
                    updateList.push_back(cellpin->net);
                }
            }
        }
        for (const CellMove& cellmove : move.legals) {
            for (const Pin* cellpin : cellmove.cell->pins) {
                Region& netbound = cellpin->net->box;
                int px_before = cellpin->pinX();
                int py_before = cellpin->pinY();
                int px_after = cellmove.x() * DPModule::dplacer->siteW + cellpin->x;
                int py_after = cellmove.y() * DPModule::dplacer->siteH + cellpin->y;
                if (cellpin->net->pins.size() <= 2 || netbound.lx == px_before || netbound.hx == px_before ||
                    netbound.ly == py_before || netbound.hy == py_before) {
                    updateList.push_back(cellpin->net);
                } else if (netbound.lx > px_after || netbound.hx < px_after || netbound.ly > py_after ||
                           netbound.hy < py_after) {
                    updateList.push_back(cellpin->net);
                }
            }
        }

        sort(updateList.begin(), updateList.end());
        vector<Net*>::iterator end = unique(updateList.begin(), updateList.end());
        updateList.resize(distance(updateList.begin(), end));
    }

    for (const CellMove& cellmove : move.legals) {
        //  //  rip-up the cell and place it again
        //  moveCell(cellmove.cell, cellmove.x, cellmove.y);
        //  shift cell in the same segment
        shiftCell(cellmove.cell, cellmove.xLong(), cellmove.yLong());
    }
    if (targetCell) {
        if (move.target.hasOrient()) {
            moveCell(targetCell, move.target.xLong(), move.target.yLong(), move.target.isFlipX(), move.target.isFlipY());
        } else {
            moveCell(targetCell, move.target.xLong(), move.target.yLong());
        }
    }

    if (updateNet) {
        for (Net* net : updateList) {
            net->update();
        }
    }
}

bool DPlacer::isLegalCellMove(const CellMove& cellMove) {
    assert(!cellMove.cell->placed);
    const Cell* targetCell = cellMove.cell;
    const long targetXLong = cellMove.xLong();
    const long targetYLong = cellMove.yLong();

    char targetRow = targetCell->rowPlace();
    if (targetRow != 'a' && ((targetRow == 'e' && targetYLong % 2 == 1) || (targetRow == 'o' && targetYLong % 2 == 0))) {
        return false;
    }

    /////////////
    // if (targetCell->getDBCell()->siteHeight() == 4)
    // {
    //     cout << targetCell->getDBCell()->name() << endl;
    //     // cout << targetCell->getDBCell()->siteHeight() << endl;
    //     cout << targetYLong * 2 << endl;
    //     cout << targetCell->oy() * 2 << endl;
    //     cout << "==========================" << endl;
    // }

    // // for TYPE4 cells: Y coordinate must on specific row pattern
    // if (targetCell->getDBCell()->siteHeight() == 4 && (targetYLong*2+siteH) % 2 != 0)
    // {
    //     cout << "haha" << endl;
    //     return false;
    // }
    /////////////

    for (unsigned h = 0; h != targetCell->h; ++h) {
        Segment* seg = getSegmentAt(targetCell, targetXLong, targetYLong + h);

        // check if the cell is completely inside segments
        if (!seg) {
            return false;
        }
        // check if the cell exceeds the left/right boundary
        if (targetXLong < seg->boundL() || targetXLong + (int)targetCell->w > seg->boundR()) {
            return false;
        }
        // check if the segment has enough space
        /*
        if(seg->space < targetCell->w){
            return false;
        }
        */

        // check if any cell overlap with the intended position
        Cell* cellL = nullptr;
        Cell* cellR = nullptr;
        CellListIter iterL;
        CellListIter iterR;
        shared_lock<shared_mutex> lock(seg->mtx);
        seg->getCellAt(targetXLong, targetYLong + h, cellL, cellR, iterL, iterR);
        // check if left, right cell has enough spacing due to horizontal spacing
        if ((cellL && cellL->hx() + Cell::getSpace(cellL, targetCell) > targetXLong) || (cellR && cellR->lx() < targetXLong + (int)targetCell->w + Cell::getSpace(targetCell, cellR))) {
            return false;
        }
    }
    return true;
}

bool DPlacer::isLegalMove(Move& move, Region region, int threshold, double targetWeight, unsigned iThread) {
    bool isLegalizable = false;
    bool alreadyLegal = true;
    //  Move& move = moves[targetCell];
    Cell* targetCell = move.target.cell;
    unsigned targetCellType = targetCell->type;
    const long targetXLong = move.target.xLong();
    const long targetYLong = move.target.yLong();
    bool originallyPlaced = targetCell->placed;

    if (originallyPlaced) {
        removeCell(targetCell);
    }

    if (targetYLong < region.ly || targetYLong + targetCell->h > region.hy || targetXLong < region.lx || targetXLong + targetCell->w > region.hx) {
        // some cells in our cases
        alreadyLegal = false;
    } else if (cellTypeMaps[targetCellType].hScore(targetYLong) || cellTypeMaps[targetCellType].vScore(targetXLong)) {
        // do not exist in our cases
        alreadyLegal = false;
    } else if (!isLegalCellMove(move.target)) {
        // lots of the cells in out cases
        // this function can check if the insert point's left, right cell has enough horizontal spacing
        alreadyLegal = false;
    } else if (targetCell->getDBCell()->siteHeight() == 4 && (targetYLong*2+siteH) % (siteH*2) != 0) {
        // for TYPE4 cell row pattern
        // cout << targetCell->getDBCell()->name() << endl;
        alreadyLegal = false;
    }
    // lots of cells in our cases has alreadyLegal=true

    // check if the cells in 1 site region are same type of cell (exclude the case of type1 v.s. large macro)
    // if not, we need a site separation
    // stupiest way: check all the cells if any of them is in the 1 site separation region
    // int leftBoundary = targetXLong - siteW;
    // int rightBoundary = targetXLong + targetCell->w + siteW;
    // for (auto& cell : cells) {
    //     if (!(targetCell->getDBCell()->height() == 2 && cell->getDBCell()->height() > 4))
    //     {
    //         if ((cell->getDBCell()->height() != targetCell->getDBCell()->height()) && cell->placed) 
    //         {
    //             if (!(cell->ly() >= (targetYLong + targetCell->h) || cell->hy() <= targetYLong)) 
    //             {
    //                 if (cell->lx() < rightBoundary || cell->hx() > leftBoundary) 
    //                 {
    //                     // cout << cell->getDBCell()->name() << " " << cell->lx() << " " << cell->hx() << " " << cell->ly() << " " << cell->hy() << endl;
    //                     // cout << targetCell->getDBCell()->name() << " " << leftBoundary << " " << rightBoundary << " " << targetYLong << " " << targetYLong + targetCell->h << endl;
    //                     // cout << siteW << endl;
    //                     // cout << "---------------------" << endl;
    //                     alreadyLegal = false;
    //                     break;
    //                 }
    //             }
    //         }
    //     }
        
    // }
    

    move.legals.clear();
    if (alreadyLegal) {
        // can be placed without legalization
        isLegalizable = true;
    } else {
        if (threshold == 0) {
            // displacement = 0 is impossible at this point
            isLegalizable = false;
        } else if (!multiRowMode) {
            isLegalizable = isLegalMoveSR(move, region, targetWeight);
        } else {
            // if (targetCell->getDBCell()->siteHeight() == 4 && targetCell->getDBCell()->name() == "g557860")
            // {
            //     cout << move.target.y() << endl;
            // }
            isLegalizable = isLegalMoveMLL(move, region, targetWeight, iThread);
            // if (targetCell->getDBCell()->siteHeight() == 4 && targetCell->getDBCell()->name() == "g557860")
            // {
            //     cout << move.target.y() << endl;
            //     move.target.y((round(((move.target.y() * 2)-2) / 8) * 8 + 2) / 2);
            //     cout << move.target.y() << endl;
            //     cout << "---------------------" << endl;
            // }
            // if (targetCell->getDBCell()->siteHeight() == 4)
            // {
            //     cout << targetCell->getDBCell()->name() << endl;
            //     cout << move.target.y() << endl;
            //     move.target.y((round(((move.target.y() * 2)-2) / 8) * 8 + 2) / 2);
            //     cout << move.target.y() << endl;
            //     cout << "-----------------" << endl;
            // }
        }
        if (isLegalizable && threshold >= 0 && move.displacement(targetWeight) > threshold * siteW) {
            isLegalizable = false;
        }
    }

    if (originallyPlaced) {
        insertCell(targetCell);
    }

    return isLegalizable;
}

