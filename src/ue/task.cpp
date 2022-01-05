//
// This file is a part of UERANSIM open source project.
// Copyright (c) 2021 ALİ GÜNGÖR.
//
// The software and all associated files are licensed under GPL-3.0
// and subject to the terms and conditions defined in LICENSE file.
//

#include "task.hpp"
#include "cmd.hpp"

#include <utils/random.hpp>

struct TimerId
{
    static constexpr const int L3_MACHINE_CYCLE = 1;
    static constexpr const int L3_TIMER = 2;
    static constexpr const int RLS_ACK_CONTROL = 3;
    static constexpr const int RLS_ACK_SEND = 4;
    static constexpr const int SWITCH_OFF = 5;
};

struct TimerPeriod
{
    static constexpr const int L3_MACHINE_CYCLE = 2500;
    static constexpr const int L3_TIMER = 1000;
    static constexpr const int RLS_ACK_CONTROL = 1500;
    static constexpr const int RLS_ACK_SEND = 2250;
    static constexpr const int SWITCH_OFF = 500;
};

namespace nr::ue
{

ue::UeTask::UeTask(std::unique_ptr<UeConfig> &&config, app::IUeController *ueController,
                   app::INodeListener *nodeListener, NtsTask *cliCallbackTask)
{
    this->logBase = std::make_unique<LogBase>("logs/ue-" + config->getNodeName() + ".log");
    this->config = std::move(config);
    this->ueController = ueController;
    this->nodeListener = nodeListener;
    this->cliCallbackTask = cliCallbackTask;

    this->m_logger = logBase->makeUniqueLogger(this->config->getLoggerPrefix() + "main");
    this->shCtx.sti = Random::Mixed(this->config->getNodeName()).nextL();

    this->rlsUdp = std::make_unique<RlsUdpLayer>(this);
    this->rlsCtl = std::make_unique<RlsCtlLayer>(this);
    this->rrc = std::make_unique<UeRrcLayer>(this);
    this->nas = std::make_unique<NasLayer>(this);
    this->tun = std::make_unique<TunLayer>(this);
}

UeTask::~UeTask() = default;

void UeTask::onStart()
{
    rlsUdp->onStart();
    rrc->onStart();
    nas->onStart();

    setTimer(TimerId::L3_MACHINE_CYCLE, TimerPeriod::L3_MACHINE_CYCLE);
    setTimer(TimerId::L3_TIMER, TimerPeriod::L3_TIMER);
    setTimer(TimerId::RLS_ACK_CONTROL, TimerPeriod::RLS_ACK_CONTROL);
    setTimer(TimerId::RLS_ACK_SEND, TimerPeriod::RLS_ACK_SEND);
}

void UeTask::onLoop()
{
    rlsUdp->checkHeartbeat();

    auto msg = take();
    if (!msg)
        return;

    if (msg->msgType == NtsMessageType::UE_CYCLE_REQUIRED)
    {
        rrc->performCycle();
        nas->performCycle();
    }
    else if (msg->msgType == NtsMessageType::TIMER_EXPIRED)
    {
        auto &w = dynamic_cast<NmTimerExpired &>(*msg);
        if (w.timerId == TimerId::L3_MACHINE_CYCLE)
        {
            setTimer(TimerId::L3_MACHINE_CYCLE, TimerPeriod::L3_MACHINE_CYCLE);
            rrc->performCycle();
            nas->performCycle();
        }
        else if (w.timerId == TimerId::L3_TIMER)
        {
            setTimer(TimerId::L3_TIMER, TimerPeriod::L3_TIMER);
            rrc->performCycle();
            nas->performCycle();
        }
        else if (w.timerId == TimerId::RLS_ACK_CONTROL)
        {
            setTimer(TimerId::RLS_ACK_CONTROL, TimerPeriod::RLS_ACK_CONTROL);
            rlsCtl->onAckControlTimerExpired();
        }
        else if (w.timerId == TimerId::RLS_ACK_SEND)
        {
            setTimer(TimerId::RLS_ACK_SEND, TimerPeriod::RLS_ACK_SEND);
            rlsCtl->onAckSendTimerExpired();
        }
        else if (w.timerId == TimerId::SWITCH_OFF)
        {
            ueController->performSwitchOff(this);
        }
    }
    else if (msg->msgType == NtsMessageType::UE_TUN_TO_APP)
    {
        auto &w = dynamic_cast<NmUeTunToApp &>(*msg);
        nas->handleUplinkDataRequest(w.psi, std::move(w.data));
    }
    else if (msg->msgType == NtsMessageType::UDP_SERVER_RECEIVE)
    {
        auto &w = dynamic_cast<udp::NwUdpServerReceive &>(*msg);
        rlsUdp->receiveRlsPdu(w.fromAddress, w.packet);
    }
    else if (msg->msgType == NtsMessageType::UE_SWITCH_OFF)
    {
        setTimer(TimerId::SWITCH_OFF, TimerPeriod::SWITCH_OFF);
    }
    else if (msg->msgType == NtsMessageType::UE_CLI_COMMAND)
    {
        auto &w = dynamic_cast<NmUeCliCommand &>(*msg);
        UeCmdHandler handler{this};
        handler.handleCmd(w);
    }
    else
    {
        m_logger->unhandledNts(*msg);
    }
}

void UeTask::onQuit()
{
    rlsUdp->onQuit();
    rrc->onQuit();
    nas->onQuit();
}

} // namespace nr::ue