/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
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

#include "shutdownwidget.h"
#include "multiuserswarningview.h"
#include "../global_util/gsettingwatcher.h"

#if 0 // storage i10n
QT_TRANSLATE_NOOP("ShutdownWidget", "Shut down"),
QT_TRANSLATE_NOOP("ShutdownWidget", "Reboot"),
QT_TRANSLATE_NOOP("ShutdownWidget", "Suspend"),
QT_TRANSLATE_NOOP("ShutdownWidget", "Hibernate")
#endif

ShutdownWidget::ShutdownWidget(QWidget *parent)
    : QFrame(parent)
    , m_index(-1)
    , m_model(nullptr)
    , m_systemMonitor(nullptr)
    , m_switchosInterface(new HuaWeiSwitchOSInterface("com.huawei", "/com/huawei/switchos", QDBusConnection::sessionBus(), this))
{
    m_frameDataBind = FrameDataBind::Instance();

    initUI();
    initConnect();

    std::function<void (QVariant)> function = std::bind(&ShutdownWidget::onOtherPageChanged, this, std::placeholders::_1);
    int index = m_frameDataBind->registerFunction("ShutdownWidget", function);

    connect(this, &ShutdownWidget::destroyed, this, [ = ] {
        m_frameDataBind->unRegisterFunction("ShutdownWidget", index);
    });
}

ShutdownWidget::~ShutdownWidget()
{
    GSettingWatcher::instance()->erase("systemSuspend");
    GSettingWatcher::instance()->erase("systemHibernate");
    GSettingWatcher::instance()->erase("systemShutdown");
}

void ShutdownWidget::initConnect()
{
    connect(m_requireRestartButton, &RoundItemButton::clicked, this, [ = ] {
        m_currentSelectedBtn = m_requireRestartButton;
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireRestart);
    });
    connect(m_requireShutdownButton, &RoundItemButton::clicked, this, [ = ] {
        m_currentSelectedBtn = m_requireShutdownButton;
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireShutdown);
    });
    connect(m_requireSuspendButton, &RoundItemButton::clicked, this, [ = ] {
        m_currentSelectedBtn = m_requireSuspendButton;
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireSuspend);
    });
    connect(m_requireHibernateButton, &RoundItemButton::clicked, this, [ = ] {
        m_currentSelectedBtn = m_requireHibernateButton;
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireHibernate);
    });
    connect(m_requireLockButton, &RoundItemButton::clicked, this, [ = ] {
        m_currentSelectedBtn = m_requireLockButton;
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireLock);
    });
    connect(m_requireSwitchUserBtn, &RoundItemButton::clicked, this, [ = ] {
        m_currentSelectedBtn = m_requireSwitchUserBtn;
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireSwitchUser);
    });
    if (m_requireSwitchSystemBtn) {
        connect(m_requireSwitchSystemBtn, &RoundItemButton::clicked, this, [ = ] {
            m_currentSelectedBtn = m_requireSwitchSystemBtn;
            onRequirePowerAction(SessionBaseModel::PowerAction::RequireSwitchSystem);
        });
    }
    connect(m_requireLogoutButton, &RoundItemButton::clicked, this, [ = ] {
        m_currentSelectedBtn = m_requireLogoutButton;
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireLogout);
    });

    connect(GSettingWatcher::instance(), &GSettingWatcher::enableChanged, this, &ShutdownWidget::onEnable);

    if (m_systemMonitor) {
        connect(m_systemMonitor, &SystemMonitor::clicked, this, &ShutdownWidget::runSystemMonitor);
    }
}

void ShutdownWidget::updateTr(RoundItemButton *widget, const QString &tr)
{
    m_trList << std::pair<std::function<void (QString)>, QString>(std::bind(&RoundItemButton::setText, widget, std::placeholders::_1), tr);
}

bool ShutdownWidget::enableState(const QString &gsettingsValue)
{
    if ("Disabled" == gsettingsValue)
        return false;
    else
        return true;
}

void ShutdownWidget::onEnable(const QString &gsettingsName, bool enable)
{
    if ("systemShutdown" == gsettingsName) {
        m_requireShutdownButton->updateState(enable ? RoundItemButton::Checked : RoundItemButton::Disabled);
    } else if ("systemSuspend" == gsettingsName) {
        m_requireSuspendButton->updateState(enable ? RoundItemButton::Normal : RoundItemButton::Disabled);
    } else if ("systemHibernate" == gsettingsName) {
        m_requireHibernateButton->updateState(enable ?RoundItemButton::Normal : RoundItemButton::Disabled);
    } else if ("systemLock" == gsettingsName) {
        m_requireLockButton->updateState(enable ? RoundItemButton::Normal : RoundItemButton::Disabled);
    }
}

void ShutdownWidget::onOtherPageChanged(const QVariant &value)
{
    m_index = value.toInt();

    for (auto it = m_btnList.constBegin(); it != m_btnList.constEnd(); ++it) {
        (*it)->updateState(RoundItemButton::Normal);
    }

    m_currentSelectedBtn = m_btnList.at(m_index);
    m_currentSelectedBtn->updateState(RoundItemButton::Checked);

    onEnable("systemShutdown", enableState(GSettingWatcher::instance()->getStatus("systemShutdown")));
    onEnable("systemSuspend", enableState(GSettingWatcher::instance()->getStatus("systemSuspend")));
    onEnable("systemHibernate", enableState(GSettingWatcher::instance()->getStatus("systemHibernate")));
    onEnable("systemLock", enableState(GSettingWatcher::instance()->getStatus("systemLock")));
}

void ShutdownWidget::hideToplevelWindow()
{
    QWidgetList widgets = qApp->topLevelWidgets();
    for (QWidget *widget : widgets) {
        if (widget->isVisible()) {
            widget->hide();
        }
    }
}

void ShutdownWidget::enterKeyPushed()
{
    if (m_systemMonitor && m_systemMonitor->state() == SystemMonitor::Enter) {
        runSystemMonitor();
        return;
    }

    if (m_currentSelectedBtn->isDisabled())
        return;

    if (m_currentSelectedBtn == m_requireShutdownButton)
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireShutdown);
    else if (m_currentSelectedBtn == m_requireRestartButton)
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireRestart);
    else if (m_currentSelectedBtn == m_requireSuspendButton)
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireSuspend);
    else if (m_currentSelectedBtn == m_requireHibernateButton)
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireHibernate);
    else if (m_currentSelectedBtn == m_requireLockButton)
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireLock);
    else if (m_currentSelectedBtn == m_requireSwitchUserBtn)
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireSwitchUser);
    else if (m_currentSelectedBtn == m_requireSwitchSystemBtn)
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireSwitchSystem);
    else if (m_currentSelectedBtn == m_requireLogoutButton)
        onRequirePowerAction(SessionBaseModel::PowerAction::RequireLogout);
}

void ShutdownWidget::enableHibernateBtn(bool enable)
{
    m_requireHibernateButton->setVisible(enable && (GSettingWatcher::instance()->getStatus("systemHibernate") != "Hiden"));
}

void ShutdownWidget::enableSleepBtn(bool enable)
{
    m_requireSuspendButton->setVisible(enable && (GSettingWatcher::instance()->getStatus("systemSuspend") != "Hiden"));
}

void ShutdownWidget::initUI()
{
    setFocusPolicy(Qt::StrongFocus);
    m_requireShutdownButton = new RoundItemButton(this);
    m_requireShutdownButton->setFocusPolicy(Qt::NoFocus);
    m_requireShutdownButton->setObjectName("RequireShutDownButton");
    m_requireShutdownButton->setAutoExclusive(true);
    updateTr(m_requireShutdownButton, "Shut down");
    GSettingWatcher::instance()->bind("systemShutdown", m_requireShutdownButton);  // GSettings配置项

    m_requireRestartButton = new RoundItemButton(tr("Reboot"), this);
    m_requireRestartButton->setFocusPolicy(Qt::NoFocus);
    m_requireRestartButton->setObjectName("RequireRestartButton");
    m_requireRestartButton->setAutoExclusive(true);
    updateTr(m_requireRestartButton, "Reboot");

    m_requireSuspendButton = new RoundItemButton(tr("Suspend"), this);
    m_requireSuspendButton->setFocusPolicy(Qt::NoFocus);
    m_requireSuspendButton->setObjectName("RequireSuspendButton");
    m_requireSuspendButton->setAutoExclusive(true);
    updateTr(m_requireSuspendButton, "Suspend");
    GSettingWatcher::instance()->bind("systemSuspend", m_requireSuspendButton);  // GSettings配置项

    m_requireHibernateButton = new RoundItemButton(tr("Hibernate"), this);
    m_requireHibernateButton->setFocusPolicy(Qt::NoFocus);
    m_requireHibernateButton->setObjectName("RequireHibernateButton");
    m_requireHibernateButton->setAutoExclusive(true);
    updateTr(m_requireHibernateButton, "Hibernate");
    GSettingWatcher::instance()->bind("systemHibernate", m_requireHibernateButton);  // GSettings配置项

    m_requireLockButton = new RoundItemButton(tr("Lock"));
    m_requireLockButton->setFocusPolicy(Qt::NoFocus);
    m_requireLockButton->setObjectName("RequireLockButton");
    m_requireLockButton->setAutoExclusive(true);
    updateTr(m_requireLockButton, "Lock");
    GSettingWatcher::instance()->bind("systemLock", m_requireLockButton);  // GSettings配置项

    m_requireLogoutButton = new RoundItemButton(tr("Log out"));
    m_requireLogoutButton->setFocusPolicy(Qt::NoFocus);
    m_requireLogoutButton->setObjectName("RequireLogoutButton");
    m_requireLogoutButton->setAutoExclusive(true);
    updateTr(m_requireLogoutButton, "Log out");

    m_requireSwitchUserBtn = new RoundItemButton(tr("Switch user"));
    m_requireSwitchUserBtn->setFocusPolicy(Qt::NoFocus);
    m_requireSwitchUserBtn->setObjectName("RequireSwitchUserButton");
    m_requireSwitchUserBtn->setAutoExclusive(true);
    updateTr(m_requireSwitchUserBtn, "Switch user");
    m_requireSwitchUserBtn->setVisible(false);

    if (m_switchosInterface->isDualOsSwitchAvail()) {
        m_requireSwitchSystemBtn = new RoundItemButton(tr("Switch system"));
        m_requireSwitchSystemBtn->setFocusPolicy(Qt::NoFocus);
        m_requireSwitchSystemBtn->setObjectName("RequireSwitchSystemButton");
        m_requireSwitchSystemBtn->setAutoExclusive(true);
        updateTr(m_requireSwitchSystemBtn, "Switch system");
    }

    m_btnList.append(m_requireShutdownButton);
    m_btnList.append(m_requireRestartButton);
    m_btnList.append(m_requireSuspendButton);
    m_btnList.append(m_requireHibernateButton);
    m_btnList.append(m_requireLockButton);
    m_btnList.append(m_requireSwitchUserBtn);
    if(m_requireSwitchSystemBtn) {
        m_btnList.append(m_requireSwitchSystemBtn);
    }
    m_btnList.append(m_requireLogoutButton);

    m_shutdownLayout = new QHBoxLayout;
    m_shutdownLayout->setMargin(0);
    m_shutdownLayout->setSpacing(10);
    m_shutdownLayout->addStretch();
    m_shutdownLayout->addWidget(m_requireShutdownButton);
    m_shutdownLayout->addWidget(m_requireRestartButton);
    m_shutdownLayout->addWidget(m_requireSuspendButton);
    m_shutdownLayout->addWidget(m_requireHibernateButton);
    m_shutdownLayout->addWidget(m_requireLockButton);
    m_shutdownLayout->addWidget(m_requireSwitchUserBtn);
    if(m_requireSwitchSystemBtn) {
        m_shutdownLayout->addWidget(m_requireSwitchSystemBtn);
    }
    m_shutdownLayout->addWidget(m_requireLogoutButton);
    m_shutdownLayout->addStretch(0);

    m_shutdownFrame = new QFrame;
    m_shutdownFrame->setLayout(m_shutdownLayout);

    m_actionLayout = new QVBoxLayout;
    m_actionLayout->setMargin(0);
    m_actionLayout->setSpacing(10);
    m_actionLayout->addStretch();
    m_actionLayout->addWidget(m_shutdownFrame);
    m_actionLayout->setAlignment(m_shutdownFrame, Qt::AlignCenter);
    m_actionLayout->addStretch();

    if (findValueByQSettings<bool>(DDESESSIONCC::session_ui_configs, "Shutdown", "enableSystemMonitor", true)) {
        QFile file("/usr/bin/deepin-system-monitor");
        if (file.exists()) {
            m_systemMonitor = new SystemMonitor;
            m_systemMonitor->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            m_systemMonitor->setFocusPolicy(Qt::NoFocus);
            setFocusPolicy(Qt::StrongFocus);
            m_actionLayout->addWidget(m_systemMonitor);
            m_actionLayout->setAlignment(m_systemMonitor, Qt::AlignHCenter);
        }
    }

    m_actionFrame = new QFrame;
    m_actionFrame->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_actionFrame->setFocusPolicy(Qt::NoFocus);
    m_actionFrame->setLayout(m_actionLayout);

    m_mainLayout = new QStackedLayout;
    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(0);
    m_mainLayout->addWidget(m_actionFrame);
    m_mainLayout->setAlignment(m_actionFrame, Qt::AlignCenter);
    setLayout(m_mainLayout);

    updateStyle(":/skin/requireshutdown.qss", this);

    // refresh language
    for (auto it = m_trList.constBegin(); it != m_trList.constEnd(); ++it) {
        it->first(qApp->translate("ShutdownWidget", it->second.toUtf8()));
    }
}

void ShutdownWidget::leftKeySwitch()
{
    m_btnList.at(m_index)->updateState(RoundItemButton::Normal);
    if (m_index <= 0) {
        m_index = m_btnList.length() - 1;
    } else {
        m_index  -= 1;
    }

    for (int i = m_btnList.size(); i != 0; --i) {
        int index = (m_index + i) % m_btnList.size();

        if (m_btnList[index]->isVisible()) {
            m_index = index;
            break;
        }
    }

    m_currentSelectedBtn = m_btnList.at(m_index);
    m_currentSelectedBtn->updateState(RoundItemButton::Checked);

    m_frameDataBind->updateValue("ShutdownWidget", m_index);
}

void ShutdownWidget::rightKeySwitch()
{
    m_btnList.at(m_index)->updateState(RoundItemButton::Normal);

    if (m_index == m_btnList.size() - 1) {
        m_index = 0;
    } else {
        ++m_index;
    }

    for (int i = 0; i < m_btnList.size(); ++i) {
        int index = (m_index + i) % m_btnList.size();

        if (m_btnList[index]->isVisible()) {
            m_index = index;
            break;
        }
    }

    m_currentSelectedBtn = m_btnList.at(m_index);
    m_currentSelectedBtn->updateState(RoundItemButton::Checked);

    m_frameDataBind->updateValue("ShutdownWidget", m_index);
}

void ShutdownWidget::onStatusChanged(SessionBaseModel::ModeStatus status)
{
    //根据当前是锁屏还是关机,设置按钮可见状态,同时需要判官切换用户按钮是否允许可见
    RoundItemButton * roundItemButton = m_requireShutdownButton;
    if (m_model->currentModeState() == SessionBaseModel::ModeStatus::ShutDownMode) {
        m_requireLockButton->setVisible(true && (GSettingWatcher::instance()->getStatus("systemLock") != "Hiden"));
        m_requireSwitchUserBtn->setVisible(m_switchUserEnable);
        if (m_requireSwitchSystemBtn) {
            m_requireSwitchSystemBtn->setVisible(true);
        }
        m_requireLogoutButton->setVisible(true);
        roundItemButton = m_requireLockButton;
    } else {
        m_requireLockButton->setVisible(false);
        m_requireSwitchUserBtn->setVisible(false);
        if (m_requireSwitchSystemBtn) {
            m_requireSwitchSystemBtn->setVisible(false);
        }
        m_requireLogoutButton->setVisible(false);
        roundItemButton = m_requireShutdownButton;
    }

    int index = m_btnList.indexOf(roundItemButton);
    roundItemButton->updateState(RoundItemButton::Checked);
    m_frameDataBind->updateValue("ShutdownWidget", index);

    if (m_systemMonitor) {
        m_systemMonitor->setVisible(status == SessionBaseModel::ModeStatus::ShutDownMode);
    }
}

void ShutdownWidget::runSystemMonitor()
{
    QProcess::startDetached("/usr/bin/deepin-system-monitor");

    if (m_systemMonitor) {
        m_systemMonitor->clearFocus();
        m_systemMonitor->setState(SystemMonitor::Leave);
    }

    hideToplevelWindow();
}

void ShutdownWidget::recoveryLayout()
{
    //关机或重启确认前会隐藏所有按钮,取消重启或关机后隐藏界面时重置按钮可见状态
    //同时需要判断切换用户按钮是否允许可见
    m_requireShutdownButton->setVisible(true && (GSettingWatcher::instance()->getStatus("systemShutdown") != "Hiden"));
    m_requireRestartButton->setVisible(true);
    enableHibernateBtn(m_model->canSleep());
    enableSleepBtn(m_model->hasSwap());
    m_requireLockButton->setVisible(true && (GSettingWatcher::instance()->getStatus("systemLock") != "Hiden"));
    m_requireSwitchUserBtn->setVisible(m_switchUserEnable);

    if (m_systemMonitor) {
        m_systemMonitor->setVisible(false);
    }

    m_mainLayout->setCurrentWidget(m_actionFrame);
    setFocusPolicy(Qt::StrongFocus);
}

void ShutdownWidget::onRequirePowerAction(SessionBaseModel::PowerAction powerAction)
{
    //锁屏或关机模式时，需要确认是否关机或检查是否有阻止关机
    if (m_model->currentType() == SessionBaseModel::AuthType::LockType ||
        m_model->currentType() == SessionBaseModel::AuthType::UnknowAuthType) {
        switch (powerAction) {
        case SessionBaseModel::PowerAction::RequireShutdown:
        case SessionBaseModel::PowerAction::RequireRestart:
        case SessionBaseModel::PowerAction::RequireSwitchSystem:
        case SessionBaseModel::PowerAction::RequireLogout:
        case SessionBaseModel::PowerAction::RequireSuspend:
        case SessionBaseModel::PowerAction::RequireHibernate:
            m_model->setIsCheckedInhibit(false);
            emit m_model->shutdownInhibit(powerAction);
            break;
        default:
            m_model->setPowerAction(powerAction);
            break;
        }
    } else {
        //登录模式直接操作
        m_model->setPowerAction(powerAction);
    }
}

void ShutdownWidget::setUserSwitchEnable(bool enable)
{
    //接收到用户列表变更信号号,记录切换用户是否允许可见,再根据当前是锁屏还是关机设置切换按钮可见状态
    if (m_switchUserEnable == enable) {
        return;
    }

    m_switchUserEnable = enable;
    m_requireSwitchUserBtn->setVisible(m_switchUserEnable && m_model->currentModeState() == SessionBaseModel::ModeStatus::ShutDownMode);
}

void ShutdownWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        leftKeySwitch();
        break;
    case Qt::Key_Right:
        rightKeySwitch();
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        enterKeyPushed();
        break;
    case Qt::Key_Tab: {
        if (m_systemMonitor && m_currentSelectedBtn->isVisible() && m_systemMonitor->isVisible()) {
            if (m_currentSelectedBtn && m_currentSelectedBtn->isChecked()) {
                m_currentSelectedBtn->setChecked(false);
                m_systemMonitor->setState(SystemMonitor::Enter);
            } else if (m_systemMonitor->state() == SystemMonitor::Enter) {
                m_systemMonitor->setState(SystemMonitor::Leave);
                m_currentSelectedBtn->setChecked(true);
            }
        }
        break;
    }
    default:
        break;
    }

    QFrame::keyReleaseEvent(event);
}

bool ShutdownWidget::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        // refresh language
        for (auto it = m_trList.constBegin(); it != m_trList.constEnd(); ++it) {
            it->first(qApp->translate("ShutdownWidget", it->second.toUtf8()));
        }
    }

    if (e->type() == QEvent::FocusIn) {
        if (m_index < 0) {
            m_index = 0;
        }
        m_frameDataBind->updateValue("ShutdownWidget", m_index);
    }

    return QFrame::event(e);
}

void ShutdownWidget::showEvent(QShowEvent *event)
{
    setFocus();
    QFrame::showEvent(event);
}

void ShutdownWidget::setModel(SessionBaseModel *const model)
{
    m_model = model;

    connect(model, &SessionBaseModel::onRequirePowerAction, this, &ShutdownWidget::onRequirePowerAction);

    connect(model, &SessionBaseModel::onHasSwapChanged, this, &ShutdownWidget::enableHibernateBtn);
    enableHibernateBtn(model->hasSwap());

    connect(model, &SessionBaseModel::canSleepChanged, this, &ShutdownWidget::enableSleepBtn);
    enableSleepBtn(model->canSleep());
}
