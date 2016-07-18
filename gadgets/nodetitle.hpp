#ifndef NODETITLE_HPP
#define NODETITLE_HPP

#include "switch.hpp"

#include "abstractnodeitem.hpp"

class Node;
class QString;

class NodeTitle : public AbstractNodeItem {

public:
    NodeTitle(Node *nd, int nest, QGraphicsItem *parent = 0);
    ~NodeTitle();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget) Q_DECL_OVERRIDE;

    QRectF boundingRect() const Q_DECL_OVERRIDE;

    void SetNest(int) Q_DECL_OVERRIDE;

    bool IsPrimary() const Q_DECL_OVERRIDE;
    bool IsHovered() const Q_DECL_OVERRIDE;

    void SetPrimary() Q_DECL_OVERRIDE;
    void SetHovered() Q_DECL_OVERRIDE;
    void SetSelectionRange() Q_DECL_OVERRIDE;

    void ClearOtherSectionSelection() Q_DECL_OVERRIDE;
    void ApplyChildrenOrder(QPointF pos) Q_DECL_OVERRIDE;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) Q_DECL_OVERRIDE;
};

#endif //ifndef NODETITLE_HPP
