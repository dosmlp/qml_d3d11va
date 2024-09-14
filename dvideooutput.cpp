#include "dvideooutput.h"
#include <QQuickWindow>

DVideoOutput::DVideoOutput() {}

QSGNode *DVideoOutput::updatePaintNode(QSGNode *sgnode, UpdatePaintNodeData *)
{
    return sgnode;
}
