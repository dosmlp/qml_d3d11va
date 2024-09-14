#ifndef DVIDEOOUTPUT_H
#define DVIDEOOUTPUT_H

#include <QQuickItem>

class DVideoOutput : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DVideoOutput)
public:
    DVideoOutput();
    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

signals:
};

#endif // DVIDEOOUTPUT_H
