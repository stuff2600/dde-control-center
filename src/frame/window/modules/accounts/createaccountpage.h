/*
 * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     liuhong <liuhong_cm@deepin.com>
 *
 * Maintainer: liuhong <liuhong_cm@deepin.com>
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

#pragma once

#include <DComboBox>
#include "interface/namespace.h"
#include "modules/accounts/user.h"
#include "modules/accounts/usermodel.h"
#include "modules/accounts/creationresult.h"
#include "avatarlistwidget.h"
#include "widgets/lineeditwidget.h"
#include "accountswidget.h"

#include <ddialog.h>
#include <DLineEdit>
#include <dpasswordedit.h>
#include <DSuggestButton>

#include <QWidget>
#include <QScrollArea>

#include "com_deepin_defender_hmiscreen.h"
#include "com_deepin_defender_daemonservice.h"

using Defender = com::deepin::defender::hmiscreen;
using DaemonService = com::deepin::defender::daemonservice;

enum ErrorType {
    ENUM_PASSWORD_EMPTY,
    ENUM_PASSWORD_TOOLONG,
    ENUM_PASSWORD_TOOSHORT,
    ENUM_PASSWORD_TYPE,
    ENUM_PASSWORD_SEVERAL,
    ENUM_PASSWORD_CHARACTER,
    ENUM_PASSWORD_SUCCESS
};

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

namespace DCC_NAMESPACE {
namespace accounts {

//创建账户页面
class CreateAccountPage : public QWidget
{
    Q_OBJECT
public:
    explicit CreateAccountPage(QWidget *parent = nullptr);
    ~CreateAccountPage();
    void setModel(dcc::accounts::UserModel *userModel, dcc::accounts::User *user);

private:
    void initWidgets(QVBoxLayout *layout);
    void initUsrGroup(QVBoxLayout *layout);
    void createUser();
    void showGroupList(const QString index);
    bool onPasswordEditFinished(DPasswordEdit *edit);
    int validatePassword(const QString &password);
    bool containsChar(const QString &password, const QString &validate);
    bool validateUsername(const QString &username);
    bool onNameEditFinished(DLineEdit *edit);
    bool onFullNameEidtFinished(DLineEdit *edit);
    /**
     * @brief passwordCompositionType 密码组成类型
     * @param validate 有效字符列表 （由英文（区分大小写）、数字或特殊符号（~!@#$%^&*()[]{}\|/?,.<>）组成的字符列表）
     * @param password 密码字符串
     * @return  密码字符串是否包含有效字符列表中字符
     */
    int  passwordCompositionType(const QStringList &validate, const QString &password);

Q_SIGNALS:
    void requestCreateUser(const dcc::accounts::User *user);
    void requestBack(DCC_NAMESPACE::accounts::AccountsWidget::ActionOption option = DCC_NAMESPACE::accounts::AccountsWidget::ClickCancel);

public Q_SLOTS:
    void setCreationResult(dcc::accounts::CreationResult *result);

protected:
    void resizeEvent(QResizeEvent *e) override; 
    /**
     * @brief keyPressEvent
     * @param e
     * @arg 为了捕获回车按键进行添加
     */
    void keyPressEvent(QKeyEvent *e) override;

private:
    dcc::accounts::User *m_newUser;
    dcc::accounts::UserModel *m_userModel;
    AvatarListWidget *m_avatarListWidget;
    DTK_WIDGET_NAMESPACE::DLineEdit *m_nameEdit;
    DTK_WIDGET_NAMESPACE::DLineEdit *m_fullnameEdit;
    DTK_WIDGET_NAMESPACE::DPasswordEdit *m_passwdEdit;
    DTK_WIDGET_NAMESPACE::DPasswordEdit *m_repeatpasswdEdit;
    DTK_WIDGET_NAMESPACE::DComboBox *m_accountChooser;

    DTK_WIDGET_NAMESPACE::DListView *m_groupListView;
    QStandardItemModel *m_groupItemModel;
    bool m_isServerSystem;
    QWidget *m_tw;
    QScrollArea *m_scrollArea;
    int m_passwordMinLength;
    int m_passwordMaxLength;
    int m_validate_Required;
    QLabel *m_groupTip;
};

}
}
