/*
* Copyright (C) 2017 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     zhangdongdong <zhangdongdong@uniontech.com>
*
* Maintainer: zhangdongdong <zhangdongdong@uniontech.com>
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

#include "pwqualitymanager.h"
#include "window/utils.h"

using namespace DCC_NAMESPACE;
PwqualityManager::PwqualityManager()
    : m_passwordMinLen(0)
    , m_passwordMaxLen(0)
{
}

PwqualityManager *PwqualityManager::instance()
{
    static PwqualityManager pwquality;
    return &pwquality;
}

PwqualityManager::ERROR_TYPE PwqualityManager::verifyPassword(const QString &user, const QString &password)
{
    ERROR_TYPE error = deepin_pw_check(user.toLocal8Bit().data(), password.toLocal8Bit().data(), LEVEL_STRICT_CHECK, nullptr);

    if (error == PW_ERR_PW_REPEAT) {
        error = PW_NO_ERR;
    }

    return error;
}

QString PwqualityManager::getErrorTips(PwqualityManager::ERROR_TYPE type)
{
    m_passwordMinLen = get_pw_min_length(LEVEL_STRICT_CHECK);
    m_passwordMaxLen = get_pw_max_length(LEVEL_STRICT_CHECK);

    QMap<int, QString> PasswordFlagsStrMap = {
        {PW_ERR_PASSWORD_EMPTY, tr("Password cannot be empty")},
        {PW_ERR_LENGTH_SHORT, tr("Password must have at least %1 characters").arg(m_passwordMinLen)},
        {PW_ERR_LENGTH_LONG, tr("Password must be no more than %1 characters").arg(m_passwordMaxLen)},
        {PW_ERR_CHARACTER_INVALID, tr("Password must contain uppercase letters, lowercase letters, numbers and symbols (~!@#$%^&*-+=`|\\(){}[]:;\"'<>,.?/)")},
        {PW_ERR_PALINDROME, tr("Password must not contain more than 4 palindrome characters")},
        {PW_ERR_WORD, tr("Do not use common words and combinations in reverse order as password")},
        {PW_ERR_PW_REPEAT, tr("New password should differ from the current one")}
    };

    if (PasswordFlagsStrMap.value(type).isEmpty()) {
        PasswordFlagsStrMap[type] = tr("It does not meet password rules");
    }

    return PasswordFlagsStrMap.value(type);
}

