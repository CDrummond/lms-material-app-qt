/*
 * Melodeon - LMS Controller
 *
 * Copyright (c) 2022-2023 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef EDGE_H
#define EDGE_H

#include <QtWidgets/QWidget>

class Edge : public QWidget {
public:
    Edge(Qt::Edges e, int sz, QWidget *p);

    static void setDark(bool d);

    bool event(QEvent *ev) override;
    void update();
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void resizeOrMoveWindow(const QPointF &p);
    void changeCursorShape(const QPointF &p);

private:
    Qt::Edges edge;
    int size;
};

#endif
