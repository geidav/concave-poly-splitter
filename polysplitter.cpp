#include <iterator>
#include <cassert>
#include <queue>

#include <QLineF>
#include <QPointF>
#include <QPolygonF>
#include <QVector2D>

#include "polysplitter.hpp"

static LineSide GetSideOfLine(const QLineF &line, const QPointF &pt)
{
    const float d = (pt.x()-line.p1().x())*(line.p2().y()-line.p1().y())-(pt.y()-line.p1().y())*(line.p2().x()-line.p1().x());
    return (d > 0.1f ? LineSide::Right : (d < -0.1f ? LineSide::Left : LineSide::On));
}

static float PointDistance(const QPointF &pt0, const QPointF &pt1)
{
    return QVector2D(pt0-pt1).length();
}

static double CalcSignedDistance(const QLineF &line, const QPointF &p)
{
    // scalar projection on line. in case of co-linear
    // vectors this is equal to the signed distance.
    return (p.x()-line.p1().x())*(line.p2().x()-line.p1().x())+(p.y()-line.p1().y())*(line.p2().y()-line.p1().y());
}

// -----------------------------------------------------------------------

std::vector<QPolygonF> PolySplitter::Split(const QPolygonF &poly, const QLineF &line)
{
    SplitEdges(poly, line);
    SortEdges(line);
    SplitPolygon();
    return CollectPolys();
}

void PolySplitter::SplitEdges(const QPolygonF &poly, const QLineF &line)
{
    SplitPoly.clear();
    EdgesOnLine.clear();

    for (int i=0; i<poly.count(); i++)
    {
        const QLineF edge(poly[i], poly[(i+1)%poly.count()]);
        const LineSide edgeStartSide = GetSideOfLine(line, edge.p1());
        const LineSide edgeEndSide = GetSideOfLine(line, edge.p2());
        SplitPoly.push_back(PolyEdge{poly[i], edgeStartSide});

        if (edgeStartSide == LineSide::On)
        {
            EdgesOnLine.push_back(&SplitPoly.back());
        }
        else if (edgeStartSide != edgeEndSide && edgeEndSide != LineSide::On)
        {
            QPointF ip;
            auto res = edge.intersect(line, &ip);
            assert(res != QLineF::NoIntersection);
            SplitPoly.push_back(PolyEdge{ip, LineSide::On});
            EdgesOnLine.push_back(&SplitPoly.back());
        }
    }

    // connect doubly linked list, except
    // first->prev and last->next
    for (auto iter=SplitPoly.begin(); iter!=std::prev(SplitPoly.end()); iter++)
    {
        auto nextIter = std::next(iter);
        iter->Next = &(*nextIter);
        nextIter->Prev = &(*iter);
    }

    // connect first->prev and last->next
    SplitPoly.back().Next = &SplitPoly.front();
    SplitPoly.front().Prev = &SplitPoly.back();
}

void PolySplitter::SortEdges(const QLineF &line)
{
    // sort edges by start position relative to
    // the start position of the split line
    std::sort(EdgesOnLine.begin(), EdgesOnLine.end(), [&](PolyEdge *e0, PolyEdge *e1)
    {
        // it's important to take the signed distance here,
        // because it can happen that the split line starts/ends
        // inside the polygon. in that case intersection points
        // can fall on both sides of the split line and taking
        // an unsigned distance metric will result in wrongly
        // ordered points in EdgesOnLine.
        return CalcSignedDistance(line, e0->StartPos) < CalcSignedDistance(line, e1->StartPos);
    });

    // compute distance between each edge's start
    // position and the first edge's start position
    for (size_t i=1; i<EdgesOnLine.size(); i++)
        EdgesOnLine[i]->DistOnLine = PointDistance(EdgesOnLine[i]->StartPos, EdgesOnLine[0]->StartPos);
}

void PolySplitter::SplitPolygon()
{
    PolyEdge *useSrc = nullptr;

    for (size_t i=0; i<EdgesOnLine.size(); i++)
    {
        // find source
        PolyEdge *srcEdge = useSrc;
        useSrc = nullptr;

        for (; !srcEdge && i<EdgesOnLine.size(); i++)
        {
            PolyEdge *curEdge = EdgesOnLine[i];
            const auto curSide = curEdge->StartSide;
            const auto prevSide = curEdge->Prev->StartSide;
            const auto nextSide = curEdge->Next->StartSide;
            assert(curSide == LineSide::On);

            if ((prevSide == LineSide::Left && nextSide == LineSide::Right) ||
                (prevSide == LineSide::Left && nextSide == LineSide::On && curEdge->Next->DistOnLine < curEdge->DistOnLine) ||
                (prevSide == LineSide::On && nextSide == LineSide::Right && curEdge->Prev->DistOnLine < curEdge->DistOnLine))
            {
                srcEdge = curEdge;
                srcEdge->IsSrcEdge = true;
            }
        }

        // find destination
        PolyEdge *dstEdge = nullptr;

        for (; !dstEdge && i<EdgesOnLine.size(); )
        {
            PolyEdge *curEdge = EdgesOnLine[i];
            const auto curSide = curEdge->StartSide;
            const auto prevSide = curEdge->Prev->StartSide;
            const auto nextSide = curEdge->Next->StartSide;
            assert(curSide == LineSide::On);

            if ((prevSide == LineSide::Right && nextSide == LineSide::Left)  ||
                (prevSide == LineSide::On && nextSide == LineSide::Left)     ||
                (prevSide == LineSide::Right && nextSide == LineSide::On)    ||
                (prevSide == LineSide::Right && nextSide == LineSide::Right) ||
                (prevSide == LineSide::Left && nextSide == LineSide::Left))
            {
                dstEdge = curEdge;
                dstEdge->IsDstEdge = true;
            }
            else
                i++;
        }

        // bridge source and destination
        if (srcEdge && dstEdge)
        {
            CreateBridge(srcEdge, dstEdge);
            VerifyCycles();

            // is it a configuration in which a vertex
            // needs to be reused as source vertex?
            if (srcEdge->Prev->Prev->StartSide == LineSide::Left)
            {
                useSrc = srcEdge->Prev;
                useSrc->IsSrcEdge = true;
            }
            else if (dstEdge->Next->StartSide == LineSide::Right)
            {
                useSrc = dstEdge;
                useSrc->IsSrcEdge = true;
            }
        }
    }
}

std::vector<QPolygonF> PolySplitter::CollectPolys()
{
    std::vector<QPolygonF> resPolys;

    for (auto &e : SplitPoly)
    {
        if (!e.Visited)
        {
            QPolygonF splitPoly;
            auto *curSide = &e;

            do
            {
                curSide->Visited = true;
                splitPoly.append(curSide->StartPos);
                curSide = curSide->Next;
            }
            while (curSide != &e);

            resPolys.push_back(splitPoly);
        }
    }

    return resPolys;
}

void PolySplitter::VerifyCycles() const
{
    for (auto &edge : SplitPoly)
    {
        const auto *curSide = &edge;
        size_t count = 0;

        do
        {
            assert(count < SplitPoly.size());
            curSide = curSide->Next;
            count++;
        }
        while (curSide != &edge);
    }
}

void PolySplitter::CreateBridge(PolyEdge *srcEdge, PolyEdge *dstEdge)
{
    SplitPoly.push_back(*srcEdge);
    PolyEdge *a = &SplitPoly.back();
    SplitPoly.push_back(*dstEdge);
    PolyEdge *b = &SplitPoly.back();
    a->Next = dstEdge;
    a->Prev = srcEdge->Prev;
    b->Next = srcEdge;
    b->Prev = dstEdge->Prev;
    srcEdge->Prev->Next = a;
    srcEdge->Prev = b;
    dstEdge->Prev->Next = b;
    dstEdge->Prev = a;
}
