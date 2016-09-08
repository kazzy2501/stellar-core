// Copyright 2016 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "NtpSynchronizationChecker.h"

#include "herder/Herder.h"
#include "main/Application.h"
#include "util/IssueManager.h"
#include "util/NtpWork.h"

#include <iostream>

namespace stellar {

std::chrono::hours const NtpSynchronizationChecker::CHECK_FREQUENCY(24);

NtpSynchronizationChecker::NtpSynchronizationChecker(Application& app, std::string ntpServer)
    : WorkParent(app)
    , mCheckTimer(app)
    , mNtpServer(std::move(ntpServer))
    , mIsShutdown(false)
{
}

NtpSynchronizationChecker::~NtpSynchronizationChecker()
{
}

void
NtpSynchronizationChecker::notify(const std::string&)
{
    if (allChildrenSuccessful())
    {
        clearChildren();
        updateIssueManager();
        mNtpWork.reset();
        scheduleNextCheck();
    }
}

void
NtpSynchronizationChecker::updateIssueManager()
{
    auto &issueManager = app().getIssueManager();
    if (mNtpWork->isSynchronized())
    {
        issueManager.removeIssue(Issue::TIME_NOT_SYNCHRONIZED);
    }
    else
    {
        issueManager.addIssue(Issue::TIME_NOT_SYNCHRONIZED);
    }
}

void
NtpSynchronizationChecker::scheduleNextCheck()
{
    std::weak_ptr<NtpSynchronizationChecker> weak = std::static_pointer_cast<NtpSynchronizationChecker>(shared_from_this());
    mCheckTimer.expires_from_now(CHECK_FREQUENCY);
    mCheckTimer.async_wait([weak](){
        auto self = weak.lock();
        if (!self)
        {
            return;
        }
        self->start();
    }, &VirtualTimer::onFailureNoop);
}

void
NtpSynchronizationChecker::start()
{
    if (mIsShutdown)
    {
        return;
    }

    mNtpWork = addWork<NtpWork>(mNtpServer, Herder::MAX_TIME_SLIP_SECONDS / 2);
    advanceChildren();
}

void
NtpSynchronizationChecker::shutdown()
{
    if (mIsShutdown)
    {
        return;
    }

    mIsShutdown = true;
    clearChildren();
    mNtpWork.reset();
}

}
