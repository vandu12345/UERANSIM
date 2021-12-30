#pragma once

#include <unordered_map>
#include <vector>

#include <lib/rrc/rrc.hpp>
#include <ue/nts.hpp>
#include <ue/types.hpp>
#include <utils/nts.hpp>

namespace nr::ue
{

class RlsCtlLayer
{
  private:
    TaskBase *m_base;
    std::unique_ptr<Logger> m_logger;
    RlsSharedContext *m_shCtx;
    int m_servingCell;
    std::unordered_map<uint32_t, rls::PduInfo> m_pduMap;
    std::unordered_map<int, std::vector<uint32_t>> m_pendingAck;

  public:
    explicit RlsCtlLayer(TaskBase *base, RlsSharedContext *shCtx);
    ~RlsCtlLayer() = default;

  private:
    void declareRadioLinkFailure(rls::ERlfCause cause);

  public:
    void onStart();
    void onQuit();
    void onAckControlTimerExpired();
    void onAckSendTimerExpired();
    void handleRlsMessage(int cellId, rls::RlsMessage &msg);
    void assignCurrentCell(int cellId);
    void handleUplinkRrcDelivery(int cellId, uint32_t pduId, rrc::RrcChannel channel, OctetString &&data);
    void handleUplinkDataDelivery(int psi, OctetString &&data);

};

} // namespace nr::ue