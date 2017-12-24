#include "Node.h"

#include "Schematic.h"

using namespace AxiomModel;

Node::Node(Schematic *parent) : parent(parent) {

}

bool Node::isDragAvailable(QPoint delta) {
    return parent->positionAvailable(dragStartPos + delta, m_size, this);
}

void Node::setName(const QString &name) {
    if (name != m_name) {
        m_name = name;
        emit nameChanged(name);
    }
}

void Node::setPos(QPoint pos) {
    setPos(pos, true, true);
}

void Node::setSize(QSize size) {
    if (size != m_size) {
        parent->positionAvailable(m_pos, &size, this);
        if (size.width() < 1 || size.height() < 1) return;

        parent->freeGridRect(m_pos, m_size);
        emit beforeSizeChanged(size);
        parent->setGridRect(m_pos, size, this);
        m_size = size;
        emit sizeChanged(size);
    }
}

void Node::select(bool exclusive) {
    if (exclusive || !m_selected) {
        m_selected = true;
        emit selected(exclusive);
    }
}

void Node::deselect() {
    if (!m_selected) return;
    m_selected = false;
    emit deselected();
}

void Node::startDragging() {
    dragStartPos = m_pos;
}

void Node::dragTo(QPoint delta) {
    setPos(dragStartPos + delta, false, false);
}

void Node::finishDragging() {
}

void Node::remove() {
    emit removed();
}

void Node::setPos(QPoint pos, bool updateGrid, bool checkPositions) {
    if (pos != m_pos) {
        auto newPos = pos;
        if (checkPositions) {
            newPos = m_pos;
            if (parent->positionAvailable(QPoint(pos.x(), newPos.y()), m_size, this)) {
                newPos.setX(pos.x());
            }
            if (parent->positionAvailable(QPoint(newPos.x(), pos.y()), m_size, this)) {
                newPos.setY(pos.y());
            }
        }

        if (newPos == m_pos) return;
        if (updateGrid) parent->freeGridRect(m_pos, m_size);

        emit beforePosChanged(newPos);
        if (updateGrid) parent->setGridRect(newPos, m_size, this);
        m_pos = newPos;
        emit posChanged(newPos);
    }
}
