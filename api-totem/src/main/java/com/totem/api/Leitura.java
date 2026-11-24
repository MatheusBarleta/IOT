package com.totem.api;

import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;

import java.time.Instant;

@Entity
public class Leitura {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    private String totemId;
    private String uid;
    private String status;
    private boolean offline;
    private Long uptimeMs;
    private Instant recebidoEm;

    public Long getId() { return id; }
    public String getTotemId() { return totemId; }
    public void setTotemId(String totemId) { this.totemId = totemId; }
    public String getUid() { return uid; }
    public void setUid(String uid) { this.uid = uid; }
    public String getStatus() { return status; }
    public void setStatus(String status) { this.status = status; }
    public boolean isOffline() { return offline; }
    public void setOffline(boolean offline) { this.offline = offline; }
    public Long getUptimeMs() { return uptimeMs; }
    public void setUptimeMs(Long uptimeMs) { this.uptimeMs = uptimeMs; }
    public Instant getRecebidoEm() { return recebidoEm; }
    public void setRecebidoEm(Instant recebidoEm) { this.recebidoEm = recebidoEm; }
}
