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

#include "settingswidget.h"
#include "ui_settingswidget.h"
#include "columnresizer.h"
#include "mainwindow.h"
#include "serverdiscovery.h"
#include "settings.h"
#include "svgicon.h"
#include <QtGui/QColor>
#include <QtGui/QFontMetrics>
#include <QtGui/QWindow>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/QAction>
#else
#include <QtWidgets/QAction>
#endif
#include <QtWidgets/QStyle>

#define REMOVE(w) \
    w->setVisible(false); \
    w->deleteLater(); \
    w=0;

static QColor constDark(20, 20, 20);
static QColor constLight(220, 220, 220);

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWidget)
    , discovery(nullptr) {
    ui->setupUi(this);
    ColumnResizer* resizer = new ColumnResizer(this);
    resizer->addWidgetsFromLayout(ui->serverGroupBox->layout(), 0);
    resizer->addWidgetsFromLayout(ui->interfaceGroupBox->layout(), 0);
#ifdef Q_OS_LINUX
    resizer->addWidgetsFromLayout(ui->miscGroupBox->layout(), 0);
#else
    REMOVE(ui->miscGroupBox);
#endif
    clearCache = false;
    connect(ui->backButton, &QPushButton::clicked, this, &SettingsWidget::backClicked);
    connect(ui->discoverServer, &QPushButton::clicked, this, &SettingsWidget::discoverClicked);
    connect(ui->clearCache, &QPushButton::clicked, this, &SettingsWidget::clearCacheClicked);
    connect(ui->zoom, &QSlider::valueChanged, this, &SettingsWidget::updateZoomPc);
    ui->backButton->setIcon(SvgIcon::icon(":back.svg", constLight, constLight));
    ui->backButton->setIconSize(QSize(24, 24));
    ui->backButton->setMaximumWidth(ui->toolbar->height());

    if (MainWindow::customWindowbar()) {
        ui->quitButton->setIcon(SvgIcon::icon(":close.svg", constLight, constLight));
        ui->quitButton->setIconSize(QSize(24, 24));
        ui->quitButton->setMaximumWidth(ui->toolbar->height());
        connect(ui->quitButton, &QPushButton::clicked, this, &SettingsWidget::quitClicked);
        ui->toolbar->installEventFilter(this);
    } else {
        REMOVE(ui->quitButton);
    }

    QFont f = font();
    f.setFixedPitch(true);
    QFontMetrics fm(f);
    ui->zoomPc->setFont(f);
    ui->zoomPc->setFixedWidth(fm.boundingRect("1000 % ").width());

    ui->zoom->setMinimum(0);
    ui->zoom->setMaximum((MainWindow::constMaxZoom-MainWindow::constMinZoom)/MainWindow::constZoomStep);
    ui->zoom->setSingleStep(1);

    QAction *closeAct = new QAction(this);
    closeAct->setShortcut(Qt::Key_Escape);
    connect(closeAct, &QAction::triggered, this, &SettingsWidget::backClicked);
    addAction(closeAct);
}

void SettingsWidget::setDark(bool dark) {
    QPalette pal(palette());
    QColor iconColor = dark ? constLight : constDark;
    pal.setColor(QPalette::Window, dark ? constDark : constLight);
    pal.setColor(QPalette::Button, pal.color(QPalette::Window));
    ui->toolbar->setPalette(pal);
    ui->toolbar->setBackgroundRole(QPalette::Window);
    ui->backButton->setPalette(pal);
    ui->backButton->setBackgroundRole(QPalette::Window);
    ui->backButton->setIcon(SvgIcon::icon(":back.svg", iconColor, iconColor));
    if (ui->quitButton) {
        ui->quitButton->setPalette(pal);
        ui->quitButton->setBackgroundRole(QPalette::Window);
        ui->quitButton->setIcon(SvgIcon::icon(":close.svg", iconColor, iconColor));
    }

    if ("fusion"==QApplication::style()->objectName()) {
        QString groupBoxStyle=QLatin1String("QGroupBox{font-weight:bold;background-color:#") +
                              QLatin1String(dark ? "282828" : "ebebeb") +
                              QLatin1String(";margin-top:3ex;padding:0px 4px 4px 4px;border-radius:6px}QGroupBox::title{subcontrol-origin:margin;padding:3px;}");
        ui->serverGroupBox->setStyleSheet(groupBoxStyle);
        ui->interfaceGroupBox->setStyleSheet(groupBoxStyle);
        ui->miscGroupBox->setStyleSheet(groupBoxStyle);
    }
}

void SettingsWidget::backClicked() {
    Settings::self()->setZoom(MainWindow::constMinZoom+(ui->zoom->value()*MainWindow::constZoomStep));
    Settings::self()->setName(ui->serverName->text().trimmed());
    Settings::self()->setAddress(ui->serverAddress->text().trimmed());
    Settings::self()->setPort(ui->serverPort->value());
    Settings::self()->setUsername(ui->userName->text().trimmed());
    Settings::self()->setPassword(ui->password->text().trimmed());
    if (ui->inhibitSuspend) {
        Settings::self()->setInhibitSuspend(ui->inhibitSuspend->isChecked());
    }
    if (ui->customTitlebar) {
        Settings::self()->setCustomTitlebar(ui->customTitlebar->isChecked());
    }
    Settings::self()->save();
    emit close(clearCache);
    clearCache = false;
}

void SettingsWidget::quitClicked() {
    emit quit();
}

void SettingsWidget::discoverClicked() {
    if (nullptr==discovery) {
        discovery = new ServerDiscovery(this);
        connect(discovery, &ServerDiscovery::server, this, &SettingsWidget::serverDiscovered);
    }
    discovery->start();
}

void SettingsWidget::clearCacheClicked() {
    clearCache = true;
}

void SettingsWidget::serverDiscovered(const QString &name, const QString &addr, quint16 port) {
    if (addr!=ui->serverAddress->text().trimmed() || port!=ui->serverPort->value()) {
        if (nullptr!=discovery) {
            discovery->stop();
        }
        ui->serverName->setText(name);
        ui->serverAddress->setText(addr);
        ui->serverPort->setValue(port);
    }
}

void SettingsWidget::updateZoomPc(int val) {
    ui->zoomPc->setText(tr("%1 %").arg(qRound((MainWindow::constMinZoom+(val*MainWindow::constZoomStep))*100.0)));
}

void SettingsWidget::update() {
    clearCache = false;
    ui->zoom->setValue((int)((Settings::self()->getZoom()-MainWindow::constMinZoom)/MainWindow::constZoomStep));
    ui->serverName->setText(Settings::self()->getName());
    ui->serverAddress->setText(Settings::self()->getAddress());
    ui->serverPort->setValue(Settings::self()->getPort());
    ui->userName->setText(Settings::self()->getUsername());
    ui->password->setText(Settings::self()->getPassword());
    updateZoomPc(ui->zoom->value());
    if (ui->inhibitSuspend) {
        ui->inhibitSuspend->setChecked(Settings::self()->getInhibitSuspend());
    }
    if (ui->customTitlebar) {
        ui->customTitlebar->setChecked(Settings::self()->getCustomTitlebar());
    }
}

bool SettingsWidget::eventFilter(QObject *watched, QEvent *e) {
    if (ui->toolbar==watched && (QEvent::MouseButtonPress==e->type() || QEvent::TouchBegin==e->type())) {
        QGuiApplication::focusWindow()->startSystemMove();
    }
    return QWidget::eventFilter(watched, e);
}
