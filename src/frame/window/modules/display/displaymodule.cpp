/*
 * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     lq <longqi_cm@deepin.com>
 *
 * Maintainer: lq <longqi_cm@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "displaymodule.h"
#include "displaywidget.h"
#include "brightnesswidget.h"
#include "scalingwidget.h"
#include "resolutionwidget.h"
#include "refreshratewidget.h"
#include "rotatewidget.h"
#include "secondaryscreendialog.h"
#include "multiscreenwidget.h"
#include "touchscreenpage.h"
#include "window/utils.h"
#include "widgets/timeoutdialog.h"
#include "modules/display/displaymodel.h"
#include "modules/display/displayworker.h"
#include "modules/display/recognizewidget.h"

#include <QApplication>

using namespace dcc::display;
using namespace DCC_NAMESPACE::display;

DisplayModule::DisplayModule(FrameProxyInterface *frame, QObject *parent)
    : QObject(parent)
    , ModuleInterface(frame)
    , m_displayModel(nullptr)
    , m_displayWorker(nullptr)
    , m_displayWidget(nullptr)
{
    // 用于传入MultiScreenWidget销毁副屏窗口
    m_pMainWindow = dynamic_cast<MainWindow *>(m_frameProxy);
}

DisplayModule::~DisplayModule()
{
    m_displayModel->deleteLater();
    m_displayWorker->deleteLater();
}

void DisplayModule::windowUpdate()
{
    if (m_pMainWindow)
        m_pMainWindow->updateWinsize();
}

void DisplayModule::initialize()
{
}

const QString DisplayModule::name() const
{
    return QStringLiteral("display");
}

const QString DisplayModule::displayName() const
{
    return tr("Display");
}

void DisplayModule::active()
{
    m_displayWidget = new DisplayWidget;
    m_displayWidget->setVisible(false);
    pushScreenWidget();

    connect(m_displayWidget, &DisplayWidget::requestShowTouchscreenPage, this, &DisplayModule::showTouchScreenPage);
    connect(m_displayWidget, &DisplayWidget::requestShowTouchscreenPage, this, &DisplayModule::showTouchRecognize);

    m_frameProxy->pushWidget(this, m_displayWidget);
    m_displayWidget->setVisible(true);
}

int DisplayModule::load(const QString &path)
{
    if (!m_displayWidget) {
        active();
    }

    return m_displayWidget->showPath(path);
}

void DisplayModule::preInitialize(bool sync, FrameProxyInterface::PushType pushtype)
{
    Q_UNUSED(pushtype);
    if (m_displayModel) {
        delete m_displayModel;
    }
    m_displayModel = new DisplayModel;
    m_displayWorker = new DisplayWorker(m_displayModel, nullptr, sync);

    m_displayModel->moveToThread(qApp->thread());
    m_displayWorker->moveToThread(qApp->thread());

    connect(m_displayModel, &DisplayModel::monitorListChanged, this, [=] {
        if (m_displayWidget != nullptr) {
            pushScreenWidget();
        }
    });

    QTimer::singleShot(0, m_displayWorker, [=] {
        m_displayWorker->active();
    });
}

QStringList DisplayModule::availPage() const
{
    QStringList sl;
    sl << "Resolution"
       << "Refresh Rate"
       << "Brightness";

    if (m_displayModel && m_displayModel->monitorList().size() > 1) {
        sl << "Multiple Displays";
    }

    if (IsServerSystem) {
        sl << "Display Scaling";
    }

    if (m_displayModel && !m_displayModel->touchscreenList().isEmpty()) {
        sl << "Touch Screen";
    }

    return sl;
}

void DisplayModule::showSingleScreenWidget()
{
    QVBoxLayout *contentLayout = new QVBoxLayout;

    BrightnessWidget *brightnessWidget = new BrightnessWidget;
    brightnessWidget->setMode(m_displayModel);
    contentLayout->addWidget(brightnessWidget);
    brightnessWidget->setVisible(m_displayModel->brightnessEnable());
    connect(brightnessWidget, &BrightnessWidget::requestSetColorTemperature, m_displayWorker, &DisplayWorker::setColorTemperature);
    connect(brightnessWidget, &BrightnessWidget::requestSetMonitorBrightness, m_displayWorker, &DisplayWorker::setMonitorBrightness);
    connect(brightnessWidget, &BrightnessWidget::requestAmbientLightAdjustBrightness, m_displayWorker, &DisplayWorker::setAmbientLightAdjustBrightness);
    connect(brightnessWidget, &BrightnessWidget::requestSetMethodAdjustCCT, m_displayWorker, &DisplayWorker::SetMethodAdjustCCT);
    connect(m_displayModel, &DisplayModel::brightnessEnableChanged, brightnessWidget, &BrightnessWidget::setVisible);

    ScalingWidget *scalingWidget = new ScalingWidget;
    scalingWidget->setModel(m_displayModel);
    contentLayout->addWidget(scalingWidget);
    connect(scalingWidget, &ScalingWidget::requestUiScaleChange, m_displayWorker, &DisplayWorker::setUiScale);
    connect(scalingWidget, &ScalingWidget::requestIndividualScaling, m_displayWorker, &DisplayWorker::setIndividualScaling);

    ResolutionWidget *resolutionWidget = new ResolutionWidget;
    resolutionWidget->setModel(m_displayModel, m_displayModel->monitorList().first());
    contentLayout->addWidget(resolutionWidget);
    connect(resolutionWidget, &ResolutionWidget::requestSetResolution, this, &DisplayModule::onRequestSetResolution, Qt::QueuedConnection);

    RefreshRateWidget *refreshRateWidget = new RefreshRateWidget;
    refreshRateWidget->setModel(m_displayModel, m_displayModel->monitorList().first());
    contentLayout->addWidget(refreshRateWidget);
    connect(refreshRateWidget, &RefreshRateWidget::requestSetResolution, this, &DisplayModule::onRequestSetResolution, Qt::QueuedConnection);

    RotateWidget *rotateWidget = new RotateWidget;
    rotateWidget->setModel(m_displayModel, m_displayModel->monitorList().first());
    contentLayout->addWidget(rotateWidget);
    connect(rotateWidget, &RotateWidget::requestSetRotate, this, &DisplayModule::onRequestSetRotate, Qt::QueuedConnection);

    contentLayout->addStretch();
    contentLayout->setContentsMargins(56, 0, 56, 0);
    QWidget *singleScreenWidget = new QWidget;
    singleScreenWidget->setLayout(contentLayout);
    m_displayWidget->setContent(singleScreenWidget);
}

void DisplayModule::showMultiScreenWidget()
{
    MultiScreenWidget *multiScreenWidget = new MultiScreenWidget(m_pMainWindow);
    multiScreenWidget->setModel(m_displayModel);
    connect(multiScreenWidget, &MultiScreenWidget::requestRecognize, this, &DisplayModule::showDisplayRecognize);
    connect(multiScreenWidget, &MultiScreenWidget::requestSwitchMode, m_displayWorker, &DisplayWorker::switchMode);
    connect(multiScreenWidget, &MultiScreenWidget::requestSetMonitorPosition, m_displayWorker, &DisplayWorker::setMonitorPosition);
    connect(multiScreenWidget, &MultiScreenWidget::requestSetPrimary, m_displayWorker, &DisplayWorker::setPrimary);
    connect(multiScreenWidget, &MultiScreenWidget::requestSetColorTemperature, m_displayWorker, &DisplayWorker::setColorTemperature);
    connect(multiScreenWidget, &MultiScreenWidget::requestSetMonitorBrightness, m_displayWorker, &DisplayWorker::setMonitorBrightness);
    connect(multiScreenWidget, &MultiScreenWidget::requestAmbientLightAdjustBrightness, m_displayWorker, &DisplayWorker::setAmbientLightAdjustBrightness);
    connect(multiScreenWidget, &MultiScreenWidget::requestSetMethodAdjustCCT, m_displayWorker, &DisplayWorker::SetMethodAdjustCCT);
    connect(multiScreenWidget, &MultiScreenWidget::requestUiScaleChange, m_displayWorker, &DisplayWorker::setUiScale);
    connect(multiScreenWidget, &MultiScreenWidget::requestIndividualScaling, m_displayWorker, &DisplayWorker::setIndividualScaling);
    connect(multiScreenWidget, &MultiScreenWidget::requestSetResolution, this, &DisplayModule::onRequestSetResolution, Qt::QueuedConnection);
    connect(multiScreenWidget, &MultiScreenWidget::requestSetRotate, this, &DisplayModule::onRequestSetRotate, Qt::QueuedConnection);
    connect(multiScreenWidget, &MultiScreenWidget::requestSetMainwindowRect, this, [=](Monitor *moi) {
        bool stateChanged = false;
        if (m_pMainWindow->windowState() == Qt::WindowState::WindowMaximized) {
            m_pMainWindow->setWindowState(Qt::WindowState::WindowNoState);
            stateChanged = true;
        }
        m_pMainWindow->move(moi->x() + moi->w() / 2 - m_pMainWindow->width() / 2, moi->y() + moi->h() / 2 - m_pMainWindow->height() / 2);
        if (stateChanged) {
            m_pMainWindow->setWindowState(Qt::WindowState::WindowMaximized);
        }
    });

    m_displayWidget->setContent(multiScreenWidget);
}

void DisplayModule::showTouchScreenPage()
{
    auto page = new TouchscreenPage();
    page->setVisible(false);
    page->setModel(m_displayModel);

    connect(page, &TouchscreenPage::requestAssociateTouch, m_displayWorker, &DisplayWorker::setTouchScreenAssociation);

    m_frameProxy->pushWidget(this, page);
    page->setVisible(true);
}

void DisplayModule::onRequestSetResolution(Monitor *monitor, const int mode)
{
    Resolution lastRes = monitor->currentMode();
    Resolution firstRes;

    for (auto res : monitor->modeList()) {
        if (res.id() == mode) {
            firstRes = res;
            break;
        }
    }

    auto tfunc = [this](Monitor *tmon, Resolution tmode) {
        if (m_displayModel->displayMode() == MERGE_MODE) {
            for (auto monitor : m_displayModel->monitorList()) {
                bool bFind = false;
                for (auto res : monitor->modeList()) {
                    if (res == tmode) {
                        m_displayWorker->setMonitorResolution(monitor, res.id());
                        bFind = true;
                        break;
                    }
                }
                if (!bFind) {
                    m_displayWorker->setMonitorResolutionBySize(monitor, tmode.width(), tmode.height());
                }
            }
        } else {
            m_displayWorker->setMonitorResolution(tmon, tmode.id());
        }
        m_displayWorker->applyChanges();
    };

    tfunc(monitor, firstRes);

    if (showTimeoutDialog(monitor) == QDialog::Accepted) {
        m_displayWorker->saveChanges();
    } else {
        tfunc(monitor, lastRes);
    }
}

void DisplayModule::onRequestSetRotate(Monitor *monitor, const int rotate)
{
    auto lastRotate = monitor->rotate();
    m_displayWorker->setMonitorRotate(monitor, rotate);
    m_displayWorker->applyChanges();

    if (showTimeoutDialog(monitor) == QDialog::Accepted) {
        m_displayWorker->saveChanges();
    } else {
        m_displayWorker->setMonitorRotate(monitor, lastRotate);
        m_displayWorker->applyChanges();
    }
}

void DisplayModule::pushScreenWidget()
{
    if (m_displayModel->monitorList().size() > 1) {
        showMultiScreenWidget();
    } else {
        showSingleScreenWidget();
    }
}

int DisplayModule::showTimeoutDialog(Monitor *monitor)
{
    TimeoutDialog *timeoutDialog = new TimeoutDialog(15);
    qreal radio = qApp->devicePixelRatio();
    connect(monitor, &Monitor::geometryChanged, timeoutDialog, [=] {
        if (timeoutDialog) {
            QRectF rt(monitor->x(), monitor->y(), monitor->w() / radio, monitor->h() / radio);
            timeoutDialog->moveToCenterByRect(rt.toRect());
        }
    },
            Qt::QueuedConnection);
    connect(m_displayModel, &DisplayModel::monitorListChanged, timeoutDialog, &TimeoutDialog::deleteLater);

    return timeoutDialog->exec();
}

void DisplayModule::showDisplayRecognize()
{
    // 复制模式
    if (m_displayModel->displayMode() == MERGE_MODE) {
        QString text = m_displayModel->monitorList().first()->name();
        for (int idx = 1; idx < m_displayModel->monitorList().size(); idx++) {
            text += QString(" = %1").arg(m_displayModel->monitorList()[idx]->name());
        }

        // 所在显示器不存在显示框
        if (m_recognizeWidget.value(text) == nullptr) {
            RecognizeWidget *widget = new RecognizeWidget(m_displayModel->monitorList()[0], text);
            QTimer::singleShot(5000, this, [=] {
                widget->deleteLater();
                m_recognizeWidget.remove(text);
            });
            m_recognizeWidget[text] = widget;
        }
    } else { // 扩展模式
        for (auto monitor : m_displayModel->monitorList()) {
            // 所在显示器不存在显示框
            if (m_recognizeWidget.value(monitor->name()) == nullptr) {
                RecognizeWidget *widget = new RecognizeWidget(monitor, monitor->name());
                m_recognizeWidget[monitor->name()] = widget;
                QTimer::singleShot(5000, this, [=] {
                    widget->deleteLater();
                    m_recognizeWidget.remove(monitor->name());
                });
                m_recognizeWidget[monitor->name()] = widget;
            }
        }
    }
}

void DisplayModule::showTouchRecognize()
{
    // 只有一块屏幕不显示触摸提示
    if (m_displayModel->monitorList().size() < 2) {
        return;
    }

    // 复制模式
    if (m_displayModel->displayMode() == MERGE_MODE) {
        QString text = m_displayModel->monitorList().first()->name();
        for (int i = 1; i < m_displayModel->monitorList().size(); i++) {
            text += QString(" = %1").arg(m_displayModel->monitorList()[i]->name());
        }

        // 所在显示器不存在显示框
        if (m_recognizeWidget.value(text) == nullptr) {
            RecognizeWidget *widget = new RecognizeWidget(m_displayModel->monitorList()[0], text);
            QTimer::singleShot(5000, this, [=] {
                widget->deleteLater();
                m_recognizeWidget.remove(text);
            });
            m_recognizeWidget[text] = widget;
        }
    } else { // 扩展模式
        for (auto monitor : m_displayModel->monitorList()) {
            // 所在显示器不存在显示框
            if (m_recognizeWidget.value(monitor->name()) == nullptr) {
                RecognizeWidget *widget = new RecognizeWidget(monitor, monitor->name());
                m_recognizeWidget[monitor->name()] = widget;
                QTimer::singleShot(5000, this, [=] {
                    widget->deleteLater();
                    m_recognizeWidget.remove(monitor->name());
                });
                m_recognizeWidget[monitor->name()] = widget;
            }
        }
    }
}
