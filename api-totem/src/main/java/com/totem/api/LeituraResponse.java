package com.totem.api;

import com.fasterxml.jackson.annotation.JsonProperty;

import java.time.Instant;

public record LeituraResponse(
        Long id,
        @JsonProperty("totem_id") String totemId,
        String uid,
        String status,
        boolean offline,
        @JsonProperty("uptime_ms") Long uptimeMs,
        @JsonProperty("recebido_em") Instant recebidoEm
) {
    public static LeituraResponse de(Leitura l) {
        return new LeituraResponse(l.getId(), l.getTotemId(), l.getUid(), l.getStatus(),
                l.isOffline(), l.getUptimeMs(), l.getRecebidoEm());
    }
}
