#ifndef OBB_SUBDIV_HPP
#define OBB_SUBDIV_HPP

#include <vector>
#include <list>

enum class LineSide
{
    On,
    Left,
    Right,
};

// splits convex and (!) concanve polygons along a line.
// all sorts of evil configurations tested, except things
// like non-manifold vertices, crossing edges and alike.
class PolySplitter
{
public:
    struct PolyEdge
    {
        PolyEdge(const QPointF &startPos, LineSide side) :
            StartPos(startPos),
            StartSide(side),
            Next(nullptr),
            Prev(nullptr),
            DistOnLine(0.0f),
            IsSrcEdge(false),
            IsDstEdge(false),
            Visited(false)
        {
        }

        QPointF             StartPos;   // start position on edge
        LineSide            StartSide;  // start position's side of split line
        PolyEdge *          Next;       // next polygon in linked list
        PolyEdge *          Prev;       // previous polygon in linked list
        float               DistOnLine; // distance relative to first point on split line
        bool                IsSrcEdge;  // for visualization
        bool                IsDstEdge;  // for visualization
        bool                Visited;    // for collecting split polygons
    };

public:
    std::vector<QPolygonF>  Split(const QPolygonF &poly, const QLineF &line);

private:
    void                    SplitEdges(const QPolygonF &poly, const QLineF &line);
    void                    SortEdges(const QLineF &line);
    void                    SplitPolygon();
    std::vector<QPolygonF>  CollectPolys();
    void                    VerifyCycles() const;
    void                    CreateBridge(PolyEdge *srcEdge, PolyEdge *dstEdge);

public:
    std::list<PolyEdge>     SplitPoly;
    std::vector<PolyEdge *> EdgesOnLine;
};

#endif
