#include <QtGui/QPainter>
#include <QtGui/QVector2D>

#include "polysplitter.hpp"
#include "mainwnd.hpp"

// test polygons used in the test cases defined below
static const QPolygonF TestPolys[] =
{
    QVector<QPointF>{{-50, 50}, {-50, -50}, { 50, -50}, {50,  50}},
    QVector<QPointF>{{-50, 50}, {-50, -50}, { 50, -50}, {50,  50}, { 0,  0}},
    QVector<QPointF>{{-50, 50}, {-50, -50}, { 50, -50}, {50,  50}, {15, 50}, {15, 25}, {-15, 25}, {-15, 50}},
    QVector<QPointF>{{-50, 50}, {-50, -50}, { 50, -50}, {50,  50}, {15, 50}, {15, 25}, {0, 40}, {-15, 25}, {-15, 50}},
    QVector<QPointF>{{-40, 50}, {-50,  40}, {-50, -50}, {50, -50}, {50, 50}, {15, 50}, {15, 15}, {-30, 15}, {-30, 40}},
    QVector<QPointF>{{-40, 40}, {-50,  30}, {-50, -50}, {50, -50}, {50, 50}, {15, 50}, {15, 15}, {-30, 15}, {-30, 30}},
    QVector<QPointF>{{-50, 50}, {-50, -50}, { 50, -50}, {50,  50}, { 0, 50}},
    QVector<QPointF>{{-50, 50}, {-50, -50}, { 50, -50}, {50,  50}, {25, 50}, {25, 15}, {10, 40}, {-10, 40}, {-25, 15}, {-25, 50}},
};

// test cases used to test the algorithm. i tried to
// cover as many corner cases as possible.
static struct TestCase
{
    QLineF              SplitLine;
    const QPolygonF *   Poly;
}
TestCases[] =
{
    {{{-60,   0}, { 55,   0}}, &TestPolys[1]},
    {{{ 60,   0}, {-60,   0}}, &TestPolys[1]},
    {{{-60,  20}, { 60,  20}}, &TestPolys[1]},
    {{{-60,  50}, { 55,  50}}, &TestPolys[1]},
    {{{ 60,  50}, {-60,  50}}, &TestPolys[1]},
    {{{-60,  60}, { 60, -60}}, &TestPolys[1]},
    {{{ 55, -55}, {-55,  55}}, &TestPolys[1]},
    {{{-55, -55}, { 55,  55}}, &TestPolys[1]},
    {{{ 55,  55}, {-55, -55}}, &TestPolys[1]},

    {{{-60,  25}, { 60,  25}}, &TestPolys[2]},
    {{{ 60,  25}, {-55,  25}}, &TestPolys[2]},
    {{{-60,  50}, { 60,  50}}, &TestPolys[2]},
    {{{ 60,  50}, {-55,  50}}, &TestPolys[2]},
    {{{ 15,  60}, { 15, -60}}, &TestPolys[2]},
    {{{ 15, -60}, { 15,  60}}, &TestPolys[2]},
    {{{-55, -15}, { 25,  60}}, &TestPolys[2]},
    {{{ 25,  60}, {-55, -15}}, &TestPolys[2]},

    {{{-60,   0}, { 60,   0}}, &TestPolys[0]},
    {{{-60,  50}, { 55,  50}}, &TestPolys[0]},
    {{{ 60,  50}, {-60,  50}}, &TestPolys[0]},
    {{{-60, -50}, { 55, -50}}, &TestPolys[0]},
    {{{ 60, -50}, {-60, -50}}, &TestPolys[0]},
    {{{-60,  60}, { 60, -60}}, &TestPolys[0]},
    {{{ 60, -60}, {-60,  60}}, &TestPolys[0]},
    {{{-60,  40}, {-40,  60}}, &TestPolys[0]},

    {{{ 55,  40}, {-60,  40}}, &TestPolys[7]},
    {{{-55,  40}, { 60,  40}}, &TestPolys[7]},
    {{{ 55,  45}, {-60,  45}}, &TestPolys[7]},
    {{{-55,  45}, { 60,  45}}, &TestPolys[7]},
    {{{-50,  60}, { 55, -55}}, &TestPolys[7]},
    {{{ 55, -55}, {-50,  60}}, &TestPolys[7]},

    {{{-60,  40}, { 60,  40}}, &TestPolys[3]},
    {{{ 60,  40}, {-55,  40}}, &TestPolys[3]},
    {{{-60,  25}, { 60,  25}}, &TestPolys[3]},
    {{{ 60,  25}, {-55,  25}}, &TestPolys[3]},

    {{{-60,  50}, { 55,  50}}, &TestPolys[4]},
    {{{ 60,  50}, {-55,  50}}, &TestPolys[4]},

    {{{-60,  40}, { 55,  40}}, &TestPolys[5]},
    {{{ 60,  40}, {-55,  40}}, &TestPolys[5]},

    {{{-60,  50}, { 55,  50}}, &TestPolys[6]},
    {{{ 60,  50}, {-60,  50}}, &TestPolys[6]},
};

MainWnd::MainWnd(QWidget *parent) : QMainWindow(parent),
    Ui(new Ui::MainWnd)
{
    Ui->setupUi(this);
}

void MainWnd::paintEvent(QPaintEvent *pe)
{
    QMainWindow::paintEvent(pe);

    // draw info text
    QPainter p(this);
    p.setPen(Qt::green);
    p.drawText(600, 700, "Source-only vertices are green");
    p.setPen(Qt::magenta);
    p.drawText(600, 720, "Destination-only vertices are magenta");
    p.setPen(Qt::yellow);
    p.drawText(600, 740, "Source+destination vertices are yellow");
    p.setPen(Qt::red);
    p.drawText(600, 760, "All other vertices are red");

    // setup proper coordinate system
    p.translate(70, 70);
    p.scale(1.0f, -1.0f);

    // split and visualize all test cases
    const QPolygonF *lastPoly = TestCases[0].Poly;
    float transX = 0.0f;

    for (const auto &tc : TestCases)
    {
        // test polygon has changed?
        if (lastPoly != tc.Poly)
        {
            // yes => rewind back on x-axis and go one line down
            p.translate(-transX, -tc.Poly->boundingRect().height()-20);
            transX = 0.0f;
        }

        // visualize split line
        p.setPen(QPen(Qt::black, 3));
        p.drawPolygon(*tc.Poly);
        p.setPen(Qt::darkGray);
        p.drawLine(tc.SplitLine);
        p.setPen(QPen(Qt::darkGray, 3));
        p.drawPoint(tc.SplitLine.p1());

        // split test case polygon
        PolySplitter ps;
        const auto resPolys = ps.Split(*tc.Poly, tc.SplitLine);

        for (const auto &s : ps.SplitPoly)
        {
            p.setPen(QPen((s.IsSrcEdge && !s.IsDstEdge) ? Qt::green : ((s.IsDstEdge && !s.IsSrcEdge) ? Qt::magenta : (s.IsDstEdge && s.IsSrcEdge ? Qt::yellow : Qt::red)), (s.IsSrcEdge || s.IsDstEdge ? 5 : 3)));
            p.drawPoint(s.StartPos);
        }

        // offset split-off polygons inwards and draw them
        for (auto resPoly : resPolys)
        {
            for (int i=0; i<resPoly.size(); i++)
            {
                QPointF &ss = resPoly[i];
                QPointF &se = resPoly[(i+1)%resPoly.size()];
                QVector2D dir = QVector2D(se-ss).normalized();
                QVector2D nrm(dir.y(), -dir.x());
                ss -= nrm.toPointF()*3.0f;
                se -= nrm.toPointF()*3.0f;
            }

            p.setPen(QPen(Qt::cyan, 1));
            p.drawPolygon(resPoly);
        }

        // draw number of split-off polygons
        p.save();
        p.setPen(Qt::black);
        p.scale(1, -1);
        p.drawText(-10, 20, QString::number(resPolys.size()));
        p.restore();

        // move on where to draw the next polygon
        transX += tc.Poly->boundingRect().width()+20;
        p.translate(tc.Poly->boundingRect().width()+20, 0.0f);
        lastPoly = tc.Poly;
    }
}
